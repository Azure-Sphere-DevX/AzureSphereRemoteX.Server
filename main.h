#pragma once

#include "hw/azure_sphere_remotex.h" // Hardware definition

#include "dx_gpio.h"
#include "dx_terminate.h"
#include "dx_timer.h"
#include "dx_utilities.h"
#include "echo_tcp_server.h"
#include "exitcode_privnetserv.h"
#include <applibs/log.h>
#include <applibs/networking.h>
#include <applibs/powermanagement.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static ExitCode InitializeAndLaunchServers(void);
static void CheckStatusTimerEventHandler(EventLoopTimer *timer);
static void ServerStoppedHandler(EchoServer_StopReason reason);
static void ShutDownServerAndCleanup(void);

EchoServer_ServerState *serverState = NULL;

// Ethernet / TCP server settings.
static struct in_addr localServerIpAddress;
static const uint16_t LocalTcpServerPort = 8888;
static int serverBacklogSize = 3;
static const char NetworkInterface[] = "wlan0";

static DX_GPIO_BINDING gpio_status_led = {.pin = STATUS_LED, .name = "gpio_status_led", .direction = DX_OUTPUT, .initialState = GPIO_Value_Low, .invertPin = true};

static DX_TIMER_BINDING checkStatusTimer = {.period = {1, 0}, .name = "checkStatusTimer", .handler = CheckStatusTimerEventHandler};
static DX_TIMER_BINDING *timer_bindings[] = {&checkStatusTimer};
