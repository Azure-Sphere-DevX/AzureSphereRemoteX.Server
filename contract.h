#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

#define REMOTEX_CONTRACT_VERSION 4

typedef enum __attribute__((packed))
{
    GPIO_OpenAsOutput_c,
    GPIO_OpenAsInput_c,
    GPIO_SetValue_c,
    GPIO_GetValue_c,

    I2CMaster_Open_c,
    I2CMaster_SetBusSpeed_c,
    I2CMaster_SetTimeout_c,
    I2CMaster_Write_c,
    I2CMaster_WriteThenRead_c,
    I2CMaster_Read_c,
    I2CMaster_SetDefaultTargetAddress_c,

    SPIMaster_Open_c,
    SPIMaster_InitConfig_c,
    SPIMaster_SetBusSpeed_c,
    SPIMaster_SetMode_c,
    SPIMaster_SetBitOrder_c,
    SPIMaster_WriteThenRead_c,
    SPIMaster_TransferSequential_c,

    PWM_Open_c,
    PWM_Apply_c,

    ADC_Open_c,
    ADC_GetSampleBitCount_c,
    ADC_SetReferenceVoltage_c,
    ADC_Poll_c,

    Storage_OpenMutableFile_c,
    Storage_DeleteMutableFile_c,
    
    RemoteX_Write_c,
    RemoteX_Read_c,
    RemoteX_Lseek_c,
    RemoteX_PlatformInformation_c
} SOCKET_CMD;

typedef struct __attribute__((packed))
{
    uint16_t block_length;
    uint16_t response_length;
    SOCKET_CMD cmd;
    bool respond;
    uint8_t contract_version;
} CTX_HEADER;

typedef struct __attribute__((packed))
{
    uint8_t flags;
    uint16_t length;
} SPI_TransferConfig;

// Note the data block sent is variable length but not greater that 4096 and must be the last field in control block
typedef struct __attribute__((packed))
{
    uint8_t data[4096];
} DATA_BLOCK;

typedef struct __attribute__((packed))
{
    CTX_HEADER header;
    int gpioId;
    uint8_t outputMode;
    uint8_t initialValue;
    int returns;
    int err_no;
} GPIO_OpenAsOutput_t;

typedef struct __attribute__((packed))
{
    CTX_HEADER header;
    int gpioId;
    int returns;
    int err_no;
} GPIO_OpenAsInput_t;

typedef struct __attribute__((packed))
{
    CTX_HEADER header;
    int gpioFd;
    uint8_t value;
    int returns;
    int err_no;
} GPIO_SetValue_t;

typedef struct __attribute__((packed))
{
    CTX_HEADER header;
    int gpioFd;
    uint8_t outValue;
    int returns;
    int err_no;
} GPIO_GetValue_t;

typedef struct __attribute__((packed))
{
    CTX_HEADER header;
    int I2C_InterfaceId;
    int returns;
    int err_no;
} I2CMaster_Open_t;

typedef struct __attribute__((packed))
{
    CTX_HEADER header;
    int fd;
    uint32_t speedInHz;
    int returns;
    int err_no;
} I2CMaster_SetBusSpeed_t;

typedef struct __attribute__((packed))
{
    CTX_HEADER header;
    int fd;
    uint32_t timeoutInMs;
    int returns;
    int err_no;
} I2CMaster_SetTimeout_t;

typedef struct __attribute__((packed))
{
    CTX_HEADER header;
    int fd;
    unsigned char address;
    int length;
    int returns;
    int err_no;
    DATA_BLOCK data_block; // Must be the last element in the struct
} I2CMaster_Write_t;

typedef struct __attribute__((packed))
{
    CTX_HEADER header;
    int fd;
    unsigned char address;
    unsigned int lenWriteData;
    unsigned int lenReadData;
    int returns;
    int err_no;
    DATA_BLOCK data_block; // Must be the last element in the struct
} I2CMaster_WriteThenRead_t;

typedef struct __attribute__((packed))
{
    CTX_HEADER header;
    int fd;
    unsigned char address;
    unsigned int maxLength;
    int returns;
    int err_no;
    DATA_BLOCK data_block; // Must be the last element in the struct
} I2CMaster_Read_t;

typedef struct __attribute__((packed))
{
    CTX_HEADER header;
    int fd;
    unsigned char address;
    int returns;
    int err_no;
} I2CMaster_SetDefaultTargetAddress_t;

typedef struct __attribute__((packed))
{
    CTX_HEADER header;
    unsigned int pwm;
    int returns;
    int err_no;
} PWM_Open_t;

typedef struct __attribute__((packed))
{
    CTX_HEADER header;
    int pwmFd;
    uint32_t pwmChannel;
    unsigned int period_nsec;
    unsigned int dutyCycle_nsec;
    uint32_t polarity;
    bool enabled;
    int returns;
    int err_no;
} PWM_Apply_t;

typedef struct __attribute__((packed))
{
    CTX_HEADER header;
    uint32_t id;
    int returns;
    int err_no;
} ADC_Open_t;

typedef struct __attribute__((packed))
{
    CTX_HEADER header;
    int fd;
    uint32_t channel;
    int returns;
    int err_no;
} ADC_GetSampleBitCount_t;

typedef struct __attribute__((packed))
{
    CTX_HEADER header;
    int fd;
    uint32_t channel;
    float referenceVoltage;
    int returns;
    int err_no;
} ADC_SetReferenceVoltage_t;

typedef struct __attribute__((packed))
{
    CTX_HEADER header;
    int fd;
    uint32_t channel;
    uint32_t outSampleValue;
    int returns;
    int err_no;
} ADC_Poll_t;

typedef struct __attribute__((packed))
{
    CTX_HEADER header;
    int interfaceId;
    int chipSelectId;
    unsigned char csPolarity;
    uint32_t z__magicAndVersion;
    int returns;
    int err_no;
} SPIMaster_Open_t;

typedef struct __attribute__((packed))
{
    CTX_HEADER header;
    unsigned char csPolarity;
    uint32_t z__magicAndVersion;
    int returns;
    int err_no;
} SPIMaster_InitConfig_t;

typedef struct __attribute__((packed))
{
    CTX_HEADER header;
    int fd;
    uint32_t speedInHz;
    int returns;
    int err_no;
} SPIMaster_SetBusSpeed_t;

typedef struct __attribute__((packed))
{
    CTX_HEADER header;
    int fd;
    uint32_t mode;
    int returns;
    int err_no;
} SPIMaster_SetMode_t;

typedef struct __attribute__((packed))
{
    CTX_HEADER header;
    int fd;
    uint32_t order;
    int returns;
    int err_no;
} SPIMaster_SetBitOrder_t;

typedef struct __attribute__((packed))
{
    CTX_HEADER header;
    int fd;
    unsigned int lenWriteData;
    unsigned int lenReadData;
    uint32_t order;
    int returns;
    int err_no;
    DATA_BLOCK data_block; // Must be the last element in the struct
} SPIMaster_WriteThenRead_t;

typedef struct __attribute__((packed))
{
    CTX_HEADER header;
    uint32_t transferCount;
    uint32_t z__magicAndVersion;
    int returns;
    int err_no;
} SPIMaster_InitTransfers_t;

typedef struct __attribute__((packed))
{
    CTX_HEADER header;
    int fd;
    uint32_t transferCount;
    int length;
    int returns;
    int err_no;
    DATA_BLOCK data_block; // Must be the last element in the struct
} SPIMaster_TransferSequential_t;

typedef struct __attribute__((packed))
{
    CTX_HEADER header;
    int returns;
    int err_no;
} Storage_OpenMutableFile_t;

typedef struct __attribute__((packed))
{
    CTX_HEADER header;
    int returns;
    int err_no;
} Storage_DeleteMutableFile_t;

typedef struct __attribute__((packed))
{
    CTX_HEADER header;
    int fd;
    int length;
    int returns;
    int err_no;
    DATA_BLOCK data_block; // Must be the last element in the struct
} RemoteX_Write_t;

typedef struct __attribute__((packed))
{
    CTX_HEADER header;
    int fd;
    int length;
    int returns;
    int err_no;
    DATA_BLOCK data_block; // Must be the last element in the struct
} RemoteX_Read_t;

typedef struct __attribute__((packed))
{
    CTX_HEADER header;
    int fd;
    off_t offset;
    int whence;
    int returns;
    int err_no;
} RemoteX_Lseek_t;

typedef struct __attribute__((packed))
{
    CTX_HEADER header;
    int length;
    int returns;
    int err_no;
    DATA_BLOCK data_block; // Must be the last element in the struct
} RemoteX_PlatformInformation_t;