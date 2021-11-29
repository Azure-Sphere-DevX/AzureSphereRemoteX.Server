/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "main.h"

/// <summary>
///     Called when the TCP server stops processing messages from clients.
/// </summary>
static void ServerStoppedHandler(EchoServer_StopReason reason)
{
    const char *reasonText;
    switch (reason)
    {
    case EchoServer_StopReason_ClientClosed:
        reasonText = "client closed the connection.";

        // stop existing TCP server
        EchoServer_ShutDown(serverState);
        // Start the TCP server.
        serverState = EchoServer_Start(dx_timerGetEventLoop(), localServerIpAddress.s_addr, LocalTcpServerPort,
                                       serverBacklogSize, ServerStoppedHandler);

        break;

    case EchoServer_StopReason_Error:
        reasonText = "an error occurred. See previous log output for more information.";
        Log_Debug("INFO: TCP server stopped: %s\n", reasonText);
        dx_terminate(ExitCode_StoppedHandler_Stopped);
        break;

    default:
        reasonText = "unknown reason.";
        Log_Debug("INFO: TCP server stopped: %s\n", reasonText);
        dx_terminate(ExitCode_StoppedHandler_Stopped);
        break;
    }
}

/// <summary>
///     Configure the specified network interface.
/// </summary>
/// <param name="interfaceName">
///     The name of the network interface to be configured.
/// </param>
/// <returns>0 on success, or -1 on failure</returns>
static int ConfigureNetworkInterface(const char *interfaceName)
{
    Networking_IpConfig ipConfig;
    Networking_IpConfig_Init(&ipConfig);
    Networking_IpConfig_EnableDynamicIp(&ipConfig);

    int result = Networking_IpConfig_Apply(interfaceName, &ipConfig);
    Networking_IpConfig_Destroy(&ipConfig);
    if (result != 0)
    {
        Log_Debug("ERROR: Networking_IpConfig_Apply: %d (%s)\n", errno, strerror(errno));
        return -1;
    }
    Log_Debug("INFO: Set dynamic IP address on network interface: %s.\n", interfaceName);

    return 0;
}

/// <summary>
///     The timer event handler.
/// </summary>
static void CheckStatusTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0)
    {
        dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
        return;
    }

    if (dx_isNetworkConnected(NetworkInterface))
    {
        dx_timerStop(&checkStatusTimer);

        if (ConfigureNetworkInterface(NetworkInterface) != 0)
        {
            dx_terminate(99);
            return;
        }

        // Start the TCP server.
        if ((serverState = EchoServer_Start(dx_timerGetEventLoop(), localServerIpAddress.s_addr, LocalTcpServerPort,
                                            serverBacklogSize, ServerStoppedHandler)) == NULL)
        {
            dx_terminate(100);
            return;
        }

        dx_gpioOff(&gpio_status_led);
        dx_gpioClose(&gpio_status_led);
    }
}

static ExitCode InitializeAndLaunchServers(void)
{
    dx_gpioOpen(&gpio_status_led);
    dx_gpioOn(&gpio_status_led);
    dx_timerSetStart(timer_bindings, NELEMS(timer_bindings));
    return ExitCode_Success;
}

/// <summary>
///     Shut down TCP server and close event handler.
/// </summary>
static void ShutDownServerAndCleanup(void)
{
    dx_timerSetStop(timer_bindings, NELEMS(timer_bindings));
    dx_timerEventLoopStop();
}

/// <summary>
///     Main entry point for this application.
/// </summary>
int main(int argc, char *argv[])
{
    dx_registerTerminationHandler();
    PowerManagement_SetSystemPowerProfile(PowerManagement_HighPerformance);
    InitializeAndLaunchServers();

    // Run the main event loop. This call blocks until termination is required.
    dx_eventLoopRun();

    ShutDownServerAndCleanup();
    return dx_getTerminationExitCode();
}
