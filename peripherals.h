#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "contract.h"
#include <applibs/gpio.h>
#include <applibs/i2c.h>
#include <applibs/spi.h>
#include <applibs/pwm.h>
#include <applibs/adc.h>
#include <errno.h>
#include <applibs/log.h>
#include <unistd.h>

#define NELEMS(x) (sizeof(x) / sizeof((x)[0]))

#define LEDGE_SIZE 128
int file_descriptor_ledger[LEDGE_SIZE];

void ledger_initialize(void);
void ledger_close(void);

int GPIO_OpenAsOutput_cmd(uint8_t *buf, ssize_t nread);
int GPIO_OpenAsInput_cmd(uint8_t *buf, ssize_t nread);
int GPIO_SetValue_cmd(uint8_t *buf, ssize_t nread);
int GPIO_GetValue_cmd(uint8_t *buf, ssize_t nread);

int I2CMaster_Open_cmd(uint8_t *buf, ssize_t nread);
int I2CMaster_SetBusSpeed_cmd(uint8_t *buf, ssize_t nread);
int I2CMaster_SetTimeout_cmd(uint8_t *buf, ssize_t nread);
int I2CMaster_Write_cmd(uint8_t *buf, ssize_t nread);
int I2CMaster_WriteThenRead_cmd(uint8_t *buf, ssize_t nread);
int I2CMaster_Read_cmd(uint8_t *buf, ssize_t nread);
int I2CMaster_SetDefaultTargetAddress_cmd(uint8_t *buf, ssize_t nread);

int SPIMaster_Open_cmd(uint8_t *buf, ssize_t nread);
int SPIMaster_InitConfig_cmd(uint8_t *buf, ssize_t nread);
int SPIMaster_SetBusSpeed_cmd(uint8_t *buf, ssize_t nread);
int SPIMaster_SetMode_cmd(uint8_t *buf, ssize_t nread);
int SPIMaster_SetBitOrder_cmd(uint8_t *buf, ssize_t nread);
int SPIMaster_WriteThenRead_cmd(uint8_t *buf, ssize_t nread);
int SPIMaster_InitTransfers_cmd(uint8_t *buf, ssize_t nread);
int SPIMaster_TransferSequential_cmd(uint8_t *buf, ssize_t nread);

int PWM_Open_cmd(uint8_t *buf, ssize_t nread);
int PWM_Apply_cmd(uint8_t *buf, ssize_t nread);

int ADC_Open_cmd(uint8_t *buf, ssize_t nread);
int ADC_GetSampleBitCount_cmd(uint8_t *buf, ssize_t nread);
int ADC_SetReferenceVoltage_cmd(uint8_t *buf, ssize_t nread);
int ADC_Poll_cmd(uint8_t *buf, ssize_t nread);
