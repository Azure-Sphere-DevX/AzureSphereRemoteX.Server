#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "contract.h"
#include <applibs/adc.h>
#include <applibs/gpio.h>
#include <applibs/i2c.h>
#include <applibs/log.h>
#include <applibs/pwm.h>
#include <applibs/spi.h>
#include <applibs/storage.h>
#include <applibs/uart.h>
#include <errno.h>
#include <unistd.h>

#define DEFINE_CMD(command, data, length)           \
    int command##_cmd(uint8_t *buf, ssize_t length) \
    {                                               \
        command##_t *data = (command##_t *)buf;

#define END_CMD           \
    data->header.err_no = errno; \
    return 0;             \
    }

#define DECLARE_CMD(command) \
    int command##_cmd(uint8_t *buf, ssize_t nread)

#define ADD_CMD(command) command##_cmd
#define CORE_BLOCK_SIZE(name) (int)(sizeof(name##_t) - sizeof(((name##_t *)0)->data_block))
#define VARIABLE_BLOCK_SIZE(name, length) (int)(sizeof(name##_t) - sizeof(((name##_t *)0)->data_block.data) + length)

#define NELEMS(x) (sizeof(x) / sizeof((x)[0]))

#define LEDGE_SIZE 128
int file_descriptor_ledger[LEDGE_SIZE];

void ledger_initialize(void);
void ledger_close(void);

DECLARE_CMD(GPIO_OpenAsOutput);
DECLARE_CMD(GPIO_OpenAsInput);
DECLARE_CMD(GPIO_SetValue);
DECLARE_CMD(GPIO_GetValue);

DECLARE_CMD(I2CMaster_Open);
DECLARE_CMD(I2CMaster_SetBusSpeed);
DECLARE_CMD(I2CMaster_SetTimeout);
DECLARE_CMD(I2CMaster_Write);
DECLARE_CMD(I2CMaster_WriteThenRead);
DECLARE_CMD(I2CMaster_Read);
DECLARE_CMD(I2CMaster_SetDefaultTargetAddress);

DECLARE_CMD(SPIMaster_Open);
DECLARE_CMD(SPIMaster_InitConfig);
DECLARE_CMD(SPIMaster_SetBusSpeed);
DECLARE_CMD(SPIMaster_SetMode);
DECLARE_CMD(SPIMaster_SetBitOrder);
DECLARE_CMD(SPIMaster_WriteThenRead);
DECLARE_CMD(SPIMaster_InitTransfers);
DECLARE_CMD(SPIMaster_TransferSequential);

DECLARE_CMD(PWM_Open);
DECLARE_CMD(PWM_Apply);

DECLARE_CMD(ADC_Open);
DECLARE_CMD(ADC_GetSampleBitCount);
DECLARE_CMD(ADC_SetReferenceVoltage);
DECLARE_CMD(ADC_Poll);

DECLARE_CMD(Storage_OpenMutableFile);
DECLARE_CMD(Storage_DeleteMutableFile);

DECLARE_CMD(RemoteX_Write);
DECLARE_CMD(RemoteX_Read);
DECLARE_CMD(RemoteX_Lseek);
DECLARE_CMD(RemoteX_Close);
DECLARE_CMD(RemoteX_PlatformInformation);

DECLARE_CMD(UART_InitConfig);
DECLARE_CMD(UART_Open);