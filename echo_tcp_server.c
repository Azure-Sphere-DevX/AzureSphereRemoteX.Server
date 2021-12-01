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

static uint8_t buffer[1024 * 5];

// Support functions.
static void HandleListenEvent(EventLoop *el, int fd, EventLoop_IoEvents events, void *context);
static void LaunchRead(EchoServer_ServerState *serverState);
static void HandleClientEvent(EventLoop *el, int fd, EventLoop_IoEvents events, void *context);
static void HandleClientReadEvent(EchoServer_ServerState *serverState);
static void LaunchWrite(EchoServer_ServerState *serverState);
static void HandleClientWriteEvent(EchoServer_ServerState *serverState);
static int OpenIpV4Socket(in_addr_t ipAddr, uint16_t port, int sockType);
static void ReportError(const char *desc);
static void StopServer(EchoServer_ServerState *serverState, EchoServer_StopReason reason);

static int (*cmd_functions[])(uint8_t *buf, ssize_t nread) = {
    ADD_CMD(GPIO_OpenAsOutput),
    ADD_CMD(GPIO_OpenAsInput),
    ADD_CMD(GPIO_SetValue),
    ADD_CMD(GPIO_GetValue),

    ADD_CMD(I2CMaster_Open),
    ADD_CMD(I2CMaster_SetBusSpeed),
    ADD_CMD(I2CMaster_SetTimeout),
    ADD_CMD(I2CMaster_Write),
    ADD_CMD(I2CMaster_WriteThenRead),
    ADD_CMD(I2CMaster_Read),
    ADD_CMD(I2CMaster_SetDefaultTargetAddress),

    ADD_CMD(SPIMaster_Open),
    ADD_CMD(SPIMaster_InitConfig),
    ADD_CMD(SPIMaster_SetBusSpeed),
    ADD_CMD(SPIMaster_SetMode),
    ADD_CMD(SPIMaster_SetBitOrder),
    ADD_CMD(SPIMaster_WriteThenRead),
    ADD_CMD(SPIMaster_TransferSequential),

    ADD_CMD(PWM_Open),
    ADD_CMD(PWM_Apply),

    ADD_CMD(ADC_Open),
    ADD_CMD(ADC_GetSampleBitCount),
    ADD_CMD(ADC_SetReferenceVoltage),
    ADD_CMD(ADC_Poll),

    ADD_CMD(Storage_OpenMutableFile),
    ADD_CMD(Storage_DeleteMutableFile),

    ADD_CMD(RemoteX_Write),
    ADD_CMD(RemoteX_Read),
    ADD_CMD(RemoteX_Lseek),
    ADD_CMD(RemoteX_Close),
    ADD_CMD(RemoteX_PlatformInformation),

    ADD_CMD(UART_InitConfig),
    ADD_CMD(UART_Open)

};

EchoServer_ServerState *EchoServer_Start(EventLoop *eventLoopInstance, in_addr_t ipAddr,
                                         uint16_t port, int backlogSize,
                                         void (*shutdownCallback)(EchoServer_StopReason))
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
    serverState->listenFd = OpenIpV4Socket(ipAddr, port, sockType);
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
        dx_terminate(ExitCode_EchoStart_Listen);
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

void process_command(EchoServer_ServerState *serverState, const uint8_t *buf, ssize_t nread)
{
    CTX_HEADER *header = (CTX_HEADER *)buf;

    // Validate incoming command and contract version
    if (header->cmd < NELEMS(cmd_functions) && header->contract_version <= REMOTEX_CONTRACT_VERSION)
    {
        cmd_functions[header->cmd]((uint8_t *)buf, nread);
    }
    else
    {
        Log_Debug("Error: Request uses newer contract version. Rebuild RemoteX service with latest contact.\n");
        header->contract_version = REMOTEX_CONTRACT_VERSION;
    }

    if (!header->respond)
    {
        LaunchRead(serverState);
    }
    else
    {
        memcpy(serverState->input, buf, (size_t)header->response_length);
        serverState->inLineSize = (size_t)header->response_length;
        LaunchWrite(serverState);
    }
}

static void HandleClientReadEvent(EchoServer_ServerState *serverState)
{
    int state_machine = 0;
    ssize_t bytes_returned;
    uint16_t block_length;
    uint8_t byte_0;
    uint8_t byte_1;

    EventLoop_ModifyIoEvents(serverState->eventLoop, serverState->clientEventReg, EventLoop_None);

    while (state_machine < 3)
    {
        switch (state_machine)
        {
        case 0:
            bytes_returned = recv(serverState->clientFd, &byte_0, 1, 0);
            break;
        case 1:
            bytes_returned = recv(serverState->clientFd, &byte_1, 1, 0);
            break;
        case 2:
            // subtract 2 as already read in two bytes
            block_length = (uint16_t)((byte_1 << 8 | byte_0) - 2);
            buffer[0] = byte_0;
            buffer[1] = byte_1;

            ssize_t bytes_read = 0;
            while (bytes_read < block_length)
            {
                ssize_t byte_count = recv(serverState->clientFd, buffer + 2 + bytes_read, (size_t)(block_length - bytes_read), 0);
                if (byte_count > 0)
                {
                    bytes_read += byte_count;
                }

                if (byte_count < 0 && errno != 11)
                {
                    bytes_read = byte_count;
                    Log_Debug("Read error number: %d\n", errno);
                    break;
                }
            }

            bytes_returned = bytes_read;

            if (bytes_returned != block_length)
            {
                Log_Debug("Block length not returned\n");
            }
            bytes_returned += 2; // add the first two bytes to bytes returned
            break;
        default:
            break;
        }

        if (bytes_returned == -1 || bytes_returned == 0)
        {
            Log_Debug("Connection closed\n");

            close(serverState->clientFd);
            serverState->clientFd = -1;

            ledger_close();
            break;
        }

        state_machine++;
    }
    if (serverState->clientFd != -1)
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
    LaunchRead(serverState);
}

static int OpenIpV4Socket(in_addr_t ipAddr, uint16_t port, int sockType)
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
            dx_terminate(ExitCode_OpenIpV4_Socket);
            break;
        }

        // Enable rebinding soon after a socket has been closed.
        int enableReuseAddr = 1;
        int r = setsockopt(localFd, SOL_SOCKET, SO_REUSEADDR, &enableReuseAddr,
                           sizeof(enableReuseAddr));
        if (r != 0)
        {
            ReportError("setsockopt/SO_REUSEADDR");
            dx_terminate(ExitCode_OpenIpV4_SetSockOpt);
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
            dx_terminate(ExitCode_OpenIpV4_Bind);
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
