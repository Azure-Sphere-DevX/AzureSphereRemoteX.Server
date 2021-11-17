/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#define _GNU_SOURCE // required for asprintf
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stddef.h>

#include <sys/socket.h>

#include <applibs/log.h>

#include "echo_tcp_server.h"

#define LEDGE_SIZE 128
int file_descriptor_ledger[LEDGE_SIZE];

// Support functions.
static void HandleListenEvent(EventLoop *el, int fd, EventLoop_IoEvents events, void *context);
static void LaunchRead(EchoServer_ServerState *serverState);
static void HandleClientEvent(EventLoop *el, int fd, EventLoop_IoEvents events, void *context);
static void HandleClientReadEvent(EchoServer_ServerState *serverState);
static void LaunchWrite(EchoServer_ServerState *serverState);
static void HandleClientWriteEvent(EchoServer_ServerState *serverState);
static int OpenIpV4Socket(in_addr_t ipAddr, uint16_t port, int sockType, ExitCode *callerExitCode);
static void ReportError(const char *desc);
static void StopServer(EchoServer_ServerState *serverState, EchoServer_StopReason reason);

void ledger_initialize(void) {
    for (size_t i = 0; i < LEDGE_SIZE; i++)
    {
        file_descriptor_ledger[i] = -1;
    }
}

void ledger_add_file_descriptor(int fd) {
    for (size_t i = 0; i < LEDGE_SIZE; i++)
    {
        if (file_descriptor_ledger[i] == -1) {
            file_descriptor_ledger[i] = fd;
            break;
        }
    }
}

void ledger_close(void) {
    for (size_t i = 0; i < LEDGE_SIZE; i++)
    {
        if (file_descriptor_ledger[i] != -1) {
            close(file_descriptor_ledger[i]);
            file_descriptor_ledger[i] = -1;
        }
    }
}

EchoServer_ServerState *EchoServer_Start(EventLoop *eventLoopInstance, in_addr_t ipAddr,
                                         uint16_t port, int backlogSize,
                                         void (*shutdownCallback)(EchoServer_StopReason),
                                         ExitCode *callerExitCode)
{
    EchoServer_ServerState *serverState = malloc(sizeof(*serverState));
    if (!serverState)
    {
        abort();
    }

    ledger_initialize();

    // Set EchoServer_ServerState state to unused values so it can be safely cleaned up if only a
    // subset of the resources are successfully allocated.
    serverState->eventLoop = eventLoopInstance;
    serverState->listenFd = -1;
    serverState->listenEventReg = NULL;
    serverState->clientFd = -1;
    serverState->clientEventReg = NULL;
    serverState->txPayload = NULL;
    serverState->shutdownCallback = shutdownCallback;

    int sockType = SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK;
    serverState->listenFd = OpenIpV4Socket(ipAddr, port, sockType, callerExitCode);
    if (serverState->listenFd == -1)
    {
        ReportError("open socket");
        goto fail;
    }

    // Be notified asynchronously when a client connects.
    serverState->listenEventReg = EventLoop_RegisterIo(eventLoopInstance, serverState->listenFd, EventLoop_Input, HandleListenEvent, serverState);
    if (serverState->listenEventReg == NULL)
    {
        ReportError("register listen event");
        goto fail;
    }

    int result = listen(serverState->listenFd, backlogSize);
    if (result != 0)
    {
        ReportError("listen");
        *callerExitCode = ExitCode_EchoStart_Listen;
        goto fail;
    }

    Log_Debug("INFO: TCP server: Listening for client connection (fd %d).\n",
              serverState->listenFd);

    return serverState;

fail:
    EchoServer_ShutDown(serverState);
    return NULL;
}

static void CloseFdAndPrintError(int fd, const char *fdName)
{
    if (fd >= 0)
    {
        int result = close(fd);
        if (result != 0)
        {
            Log_Debug("ERROR: Could not close fd %s: %s (%d).\n", fdName, strerror(errno), errno);
        }
    }
}

void EchoServer_ShutDown(EchoServer_ServerState *serverState)
{
    if (!serverState)
    {
        return;
    }

    EventLoop_UnregisterIo(serverState->eventLoop, serverState->clientEventReg);
    CloseFdAndPrintError(serverState->clientFd, "clientFd");

    EventLoop_UnregisterIo(serverState->eventLoop, serverState->listenEventReg);
    CloseFdAndPrintError(serverState->listenFd, "listenFd");

    // free(serverState->txPayload);

    free(serverState);
}

static void HandleListenEvent(EventLoop *el, int fd, EventLoop_IoEvents events, void *context)
{
    EchoServer_ServerState *serverState = (EchoServer_ServerState *)context;
    int localFd = -1;

    do
    {
        // Create a new accepted socket to connect to the client.
        // The newly-accepted sockets should be opened in non-blocking mode.
        struct sockaddr in_addr;
        socklen_t sockLen = sizeof(in_addr);
        localFd = accept4(serverState->listenFd, &in_addr, &sockLen, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (localFd == -1)
        {
            ReportError("accept");
            break;
        }

        Log_Debug("INFO: TCP server: Accepted client connection (fd %d).\n", localFd);

        // If already have a client, then close the newly-accepted socket.
        if (serverState->clientFd >= 0)
        {
            Log_Debug(
                "INFO: TCP server: Closing incoming client connection: only one client supported "
                "at a time.\n");
            break;
        }

        serverState->clientEventReg = EventLoop_RegisterIo(serverState->eventLoop, localFd, 0x0,
                                                           HandleClientEvent, serverState);
        if (serverState->clientEventReg == NULL)
        {
            ReportError("register client event");
            break;
        }

        // Socket opened successfully, so transfer ownership to EchoServer_ServerState object.
        serverState->clientFd = localFd;
        localFd = -1;

        LaunchRead(serverState);
    } while (0);

    CloseFdAndPrintError(localFd, "localClientFd");
}

static void LaunchRead(EchoServer_ServerState *serverState)
{
    serverState->inLineSize = 0;

    EventLoop_ModifyIoEvents(serverState->eventLoop, serverState->clientEventReg, EventLoop_Input);
}

static void HandleClientEvent(EventLoop *el, int fd, EventLoop_IoEvents events, void *context)
{
    EchoServer_ServerState *serverState = context;

    if (events & EventLoop_Input)
    {
        HandleClientReadEvent(serverState);
    }

    if (events & EventLoop_Output)
    {
        HandleClientWriteEvent(serverState);
    }
}

bool GPIO_OpenAsOutput_cmd(uint8_t *buf, ssize_t nread)
{
    if (buf[0] != GPIO_OpenAsOutput_c)
    {
        return false;
    }

    GPIO_OpenAsOutput_t *data = (GPIO_OpenAsOutput_t *)buf;

    data->returns = GPIO_OpenAsOutput(data->gpioId, data->outputMode, data->initialValue);
    data->err_no = errno;

    ledger_add_file_descriptor(data->returns);

    // Log_Debug("%s\n", __func__);
    return true;
}

bool GPIO_OpenAsInput_cmd(uint8_t *buf, ssize_t nread)
{
    if (buf[0] != GPIO_OpenAsInput_c)
    {
        return false;
    }

    GPIO_OpenAsInput_t *data = (GPIO_OpenAsInput_t *)buf;

    data->returns = GPIO_OpenAsInput(data->gpioId);
    data->err_no = errno;

    ledger_add_file_descriptor(data->returns);

    // Log_Debug("%s\n", __func__);
    return true;
}

bool GPIO_SetValue_cmd(uint8_t *buf, ssize_t nread)
{
    if (buf[0] != GPIO_SetValue_c)
    {
        return false;
    }

    GPIO_SetValue_t *data = (GPIO_SetValue_t *)buf;
    data->returns = GPIO_SetValue(data->gpioFd, data->value);
    data->err_no = errno;

    // Log_Debug("%s\n", __func__);
    return true;
}

bool GPIO_GetValue_cmd(uint8_t *buf, ssize_t nread)
{
    GPIO_Value_Type outValue;

    if (buf[0] != GPIO_GetValue_c)
    {
        return false;
    }

    GPIO_GetValue_t *data = (GPIO_GetValue_t *)buf;
    data->returns = GPIO_GetValue(data->gpioFd, &outValue);
    data->outValue = outValue;
    data->err_no = errno;

    // Log_Debug("%s\n", __func__);
    return true;
}

bool I2CMaster_Open_cmd(uint8_t *buf, ssize_t nread)
{
    if (buf[0] != I2CMaster_Open_c)
    {
        return false;
    }

    I2CMaster_Open_t *data = (I2CMaster_Open_t *)buf;

    data->returns = I2CMaster_Open(data->I2C_InterfaceId);
    data->err_no = errno;

    ledger_add_file_descriptor(data->returns);

    // Log_Debug("%s\n", __func__);
    return true;
}

bool I2CMaster_SetBusSpeed_cmd(uint8_t *buf, ssize_t nread)
{
    if (buf[0] != I2CMaster_SetBusSpeed_c)
    {
        return false;
    }

    I2CMaster_SetBusSpeed_t *data = (I2CMaster_SetBusSpeed_t *)buf;

    data->returns = I2CMaster_SetBusSpeed(data->fd, data->speedInHz);
    data->err_no = errno;

    // Log_Debug("%s\n", __func__);
    return true;
}

bool I2CMaster_SetTimeout_cmd(uint8_t *buf, ssize_t nread)
{
    if (buf[0] != I2CMaster_SetTimeout_c)
    {
        return false;
    }

    I2CMaster_SetTimeout_t *data = (I2CMaster_SetTimeout_t *)buf;

    data->returns = I2CMaster_SetTimeout(data->fd, data->timeoutInMs);
    data->err_no = errno;

    // Log_Debug("%s\n", __func__);
    return true;
}

bool I2CMaster_Write_cmd(uint8_t *buf, ssize_t nread)
{
    if (buf[0] != I2CMaster_Write_c)
    {
        return false;
    }

    I2CMaster_Write_t *data = (I2CMaster_Write_t *)buf;

    data->returns = I2CMaster_Write(data->fd, data->address, (const uint8_t *)data->data, data->length);
    data->err_no = errno;

    // Log_Debug("%s\n", __func__);
    return true;
}

bool I2CMaster_WriteThenRead_cmd(uint8_t *buf, ssize_t nread)
{    
    if (buf[0] != I2CMaster_WriteThenRead_c)
    {
        return false;
    }

    I2CMaster_WriteThenRead_t *data = (I2CMaster_WriteThenRead_t *)buf;

    uint8_t read_data[data->lenReadData];

    data->returns = I2CMaster_WriteThenRead(data->fd, data->address, (const uint8_t *)data->data, data->lenWriteData, read_data, data->lenReadData);
    data->err_no = errno;

    memset(data->data, 0x00, sizeof(data->data));
    memcpy(data->data, read_data, data->lenReadData);

    // Log_Debug("%s\n", __func__);
    return true;
}

bool I2CMaster_Read_cmd(uint8_t *buf, ssize_t nread)
{    
    if (buf[0] != I2CMaster_Read_c)
    {
        return false;
    }

    I2CMaster_Read_t *data = (I2CMaster_Read_t *)buf;

    data->returns = I2CMaster_Read(data->fd, data->address, data->data, data->maxLength);
    data->err_no = errno;

    // Log_Debug("%s\n", __func__);
    return true;
}

bool I2CMaster_SetDefaultTargetAddress_cmd(uint8_t *buf, ssize_t nread)
{    
    if (buf[0] != I2CMaster_Read_c)
    {
        return false;
    }

    I2CMaster_SetDefaultTargetAddress_t *data = (I2CMaster_SetDefaultTargetAddress_t *)buf;

    data->returns = I2CMaster_SetDefaultTargetAddress(data->fd, data->address);
    data->err_no = errno;

    // Log_Debug("%s\n", __func__);
    return true;
}

bool SPIMaster_Open_cmd(uint8_t *buf, ssize_t nread)
{
    if (buf[0] != SPIMaster_Open_c)
    {
        return false;
    }

    // SPIMaster_Open_t *data = (SPIMaster_Open_t *)buf;

    // if (ledger_spi[data->interfaceId] != -1)
    // {
    //     if (close(ledger_spi[data->interfaceId]) != 0)
    //     {
    //         Log_Debug("Failed to close I2C Ledger item %d\n", data->interfaceId);
    //     }
    //     ledger_spi[data->interfaceId] = -1;
    // }

    // SPIMaster_Config config;
    // memcpy(&config, &data->config, sizeof(config));

    // data->returns = SPIMaster_Open(data->interfaceId, data->chipSelectId, &config);
    // data->err_no = errno;

    // ledger_spi[data->interfaceId] = data->returns;

    // // Log_Debug("%s\n", __func__);
    return true;
}

bool PWM_Open_cmd(uint8_t *buf, ssize_t nread)
{
    if (buf[0] != PWM_Open_c)
    {
        return false;
    }

    PWM_Open_t *data = (PWM_Open_t *)buf;

    data->returns = PWM_Open(data->pwm);
    data->err_no = errno;

    ledger_add_file_descriptor(data->returns);

    // Log_Debug("%s\n", __func__);
    return true;
}

bool PWM_Apply_cmd(uint8_t *buf, ssize_t nread)
{
    if (buf[0] != PWM_Apply_c)
    {
        return false;
    }

    PWM_Apply_t *data = (PWM_Apply_t *)buf;

    PwmState newState;
    newState.period_nsec = data->period_nsec;
    newState.dutyCycle_nsec = data->dutyCycle_nsec;
    newState.polarity = data->polarity;
    newState.enabled = data->enabled;

    data->returns = PWM_Apply(data->pwmFd, data->pwmChannel, &newState);
    data->err_no = errno;

    Log_Debug("%s\n", __func__);
    return true;
}

bool (*cmd_functions[])(uint8_t *buf, ssize_t nread) = {GPIO_OpenAsOutput_cmd,
                                                        GPIO_OpenAsInput_cmd,
                                                        GPIO_SetValue_cmd,
                                                        GPIO_GetValue_cmd,
                                                        I2CMaster_Open_cmd,
                                                        I2CMaster_SetBusSpeed_cmd,
                                                        I2CMaster_SetTimeout_cmd,
                                                        I2CMaster_Write_cmd,
                                                        I2CMaster_WriteThenRead_cmd,
                                                        I2CMaster_SetDefaultTargetAddress_cmd,
                                                        SPIMaster_Open_cmd,
                                                        PWM_Open_cmd,
                                                        PWM_Apply_cmd
                                                        };

void process_command(EchoServer_ServerState *serverState, const uint8_t *buf, ssize_t nread)
{
    bool cmd_found = false;

    for (size_t i = 0; i < NELEMS(cmd_functions) && !cmd_found; i++)
    {
        cmd_found = cmd_functions[i]((uint8_t *)buf, nread);
    }

    if (!cmd_found)
    {
        Log_Debug("recv: %s\n", buf);
    }

    memcpy(serverState->input, buf, (size_t)nread);

    serverState->inLineSize = (size_t)nread;
    LaunchWrite(serverState);
}

static void HandleClientReadEvent(EchoServer_ServerState *serverState)
{
    static uint8_t buffer[1024];
    memset(buffer, 0x00, sizeof(buffer));

    EventLoop_ModifyIoEvents(serverState->eventLoop, serverState->clientEventReg, EventLoop_None);

    ssize_t bytes_returned = read(serverState->clientFd, buffer, sizeof(buffer));

    if (bytes_returned == -1 || bytes_returned == 0)
    {
        Log_Debug("Error receiving bytes\n");

        close(serverState->clientFd);
        serverState->clientFd = -1;

        ledger_close();
    }
    else
    {
        process_command(serverState, buffer, bytes_returned);
    }
}

static void LaunchWrite(EchoServer_ServerState *serverState)
{
    // Start to send the response.
    serverState->txPayloadSize = serverState->inLineSize;
    serverState->txPayload = serverState->input;
    serverState->txBytesSent = 0;
    HandleClientWriteEvent(serverState);
}

/// <summary>
///     <para>
///         Called to launch a new write operation, or to continue an existing
///         write operation when the client socket receives a write event.
///     </para>
///     <param name="serverState">
///         The server whose client should be sent the message.
///     </param>
/// </summary>
static void HandleClientWriteEvent(EchoServer_ServerState *serverState)
{
    EventLoop_ModifyIoEvents(serverState->eventLoop, serverState->clientEventReg, EventLoop_None);

    // Continue until have written entire response, error occurs, or OS TX buffer is full.
    while (serverState->txBytesSent < serverState->txPayloadSize)
    {
        size_t remainingBytes = serverState->txPayloadSize - serverState->txBytesSent;
        const uint8_t *data = &serverState->txPayload[serverState->txBytesSent];
        ssize_t bytesSentOneSysCall =
            send(serverState->clientFd, data, remainingBytes, /* flags */ 0);

        // If successfully sent data then stay in loop and try to send more data.
        if (bytesSentOneSysCall > 0)
        {
            serverState->txBytesSent += (size_t)bytesSentOneSysCall;
        }

        // If OS TX buffer is full then wait for next EventLoop_Output.
        else if (bytesSentOneSysCall < 0 && errno == EAGAIN)
        {
            EventLoop_ModifyIoEvents(serverState->eventLoop, serverState->clientEventReg,
                                     EventLoop_Output);
            return;
        }

        // Another error occurred so terminate the program.
        else
        {
            ReportError("send");
            StopServer(serverState, EchoServer_StopReason_Error);
            return;
        }
    }

    // If reached here then successfully sent entire payload so clean up and read next line from
    // client.
    // free(serverState->txPayload);
    // serverState->txPayload = NULL;

    LaunchRead(serverState);
}

static int OpenIpV4Socket(in_addr_t ipAddr, uint16_t port, int sockType, ExitCode *callerExitCode)
{
    int localFd = -1;
    int retFd = -1;

    do
    {
        // Create a TCP / IPv4 socket. This will form the listen socket.
        localFd = socket(AF_INET, sockType, /* protocol */ 0);
        if (localFd == -1)
        {
            ReportError("socket");
            *callerExitCode = ExitCode_OpenIpV4_Socket;
            break;
        }

        // Enable rebinding soon after a socket has been closed.
        int enableReuseAddr = 1;
        int r = setsockopt(localFd, SOL_SOCKET, SO_REUSEADDR, &enableReuseAddr,
                           sizeof(enableReuseAddr));
        if (r != 0)
        {
            ReportError("setsockopt/SO_REUSEADDR");
            *callerExitCode = ExitCode_OpenIpV4_SetSockOpt;
            break;
        }

        // Bind to a well-known IP address.
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = ipAddr;
        addr.sin_port = htons(port);

        r = bind(localFd, (const struct sockaddr *)&addr, sizeof(addr));
        if (r != 0)
        {
            ReportError("bind");
            *callerExitCode = ExitCode_OpenIpV4_Bind;
            break;
        }

        // Port opened successfully.
        retFd = localFd;
        localFd = -1;
    } while (0);

    CloseFdAndPrintError(localFd, "localListenFd");

    return retFd;
}

static void ReportError(const char *desc)
{
    Log_Debug("ERROR: TCP server: \"%s\", errno=%d (%s)\n", desc, errno, strerror(errno));
}

static void StopServer(EchoServer_ServerState *serverState, EchoServer_StopReason reason)
{
    if (serverState->clientEventReg != NULL)
    {
        EventLoop_ModifyIoEvents(serverState->eventLoop, serverState->clientEventReg,
                                 EventLoop_None);
    }

    if (serverState->listenEventReg != NULL)
    {
        EventLoop_ModifyIoEvents(serverState->eventLoop, serverState->listenEventReg,
                                 EventLoop_None);
    }

    serverState->shutdownCallback(reason);
}
