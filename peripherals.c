#include "peripherals.h"

void ledger_initialize(void)
{
    for (size_t i = 0; i < LEDGE_SIZE; i++)
    {
        file_descriptor_ledger[i] = -1;
    }
}

void ledger_add_file_descriptor(int fd)
{
    if (fd == -1)
    {
        return;
    }

    for (size_t i = 0; i < LEDGE_SIZE; i++)
    {
        if (file_descriptor_ledger[i] == -1)
        {
            file_descriptor_ledger[i] = fd;
            break;
        }
    }
}

void ledger_remove_file_descriptor(int fd)
{
    if (fd == -1)
    {
        return;
    }

    for (size_t i = 0; i < LEDGE_SIZE; i++)
    {
        if (file_descriptor_ledger[i] == fd)
        {
            file_descriptor_ledger[i] = -1;
            break;
        }
    }
}

void ledger_close(void)
{
    for (size_t i = 0; i < LEDGE_SIZE; i++)
    {
        if (file_descriptor_ledger[i] != -1)
        {
            close(file_descriptor_ledger[i]);
            file_descriptor_ledger[i] = -1;
        }
    }
}

DEFINE_CMD(GPIO_OpenAsOutput, data, nread)
{
    data->header.returns = GPIO_OpenAsOutput(data->gpioId, data->outputMode, data->initialValue);
    ledger_add_file_descriptor(data->header.returns);
}
END_CMD

DEFINE_CMD(GPIO_OpenAsInput, data, nread)
{
    data->header.returns = GPIO_OpenAsInput(data->gpioId);
    ledger_add_file_descriptor(data->header.returns);
}
END_CMD

DEFINE_CMD(GPIO_SetValue, data, nread)
{
    data->header.returns = GPIO_SetValue(data->gpioFd, data->value);
}
END_CMD

DEFINE_CMD(GPIO_GetValue, data, nread)
{
    GPIO_Value_Type outValue;

    data->header.returns = GPIO_GetValue(data->gpioFd, &outValue);
    data->outValue = outValue;
}
END_CMD

DEFINE_CMD(I2CMaster_Open, data, nread)
{
    data->header.returns = I2CMaster_Open(data->I2C_InterfaceId);
    ledger_add_file_descriptor(data->header.returns);
}
END_CMD

DEFINE_CMD(I2CMaster_SetBusSpeed, data, nread)
{
    data->header.returns = I2CMaster_SetBusSpeed(data->fd, data->speedInHz);
}
END_CMD

DEFINE_CMD(I2CMaster_SetTimeout, data, nread)
{
    data->header.returns = I2CMaster_SetTimeout(data->fd, data->timeoutInMs);
}
END_CMD

DEFINE_CMD(I2CMaster_Write, data, nread)
{
    data->header.returns = I2CMaster_Write(data->fd, data->address, (const uint8_t *)data->data_block.data, (size_t)data->length);
}
END_CMD

DEFINE_CMD(I2CMaster_WriteThenRead, data, nread)
{
    data->header.returns = I2CMaster_WriteThenRead(data->fd, data->address, (const uint8_t *)data->data_block.data, data->lenWriteData, (uint8_t *)data->data_block.data, data->lenReadData);
}
END_CMD

DEFINE_CMD(I2CMaster_Read, data, nread)
{
    data->header.returns = I2CMaster_Read(data->fd, data->address, data->data_block.data, data->maxLength);
}
END_CMD

DEFINE_CMD(I2CMaster_SetDefaultTargetAddress, data, nread)
{
    data->header.returns = I2CMaster_SetDefaultTargetAddress(data->fd, data->address);
}
END_CMD

DEFINE_CMD(PWM_Open, data, nread)
{
    data->header.returns = PWM_Open(data->pwm);
    ledger_add_file_descriptor(data->header.returns);
}
END_CMD

DEFINE_CMD(PWM_Apply, data, nread)
{
    uint8_t *data_ptr = data->data_block.data;
    PwmState *newState = (PwmState *)data_ptr;

    data->header.returns = PWM_Apply(data->pwmFd, data->pwmChannel, newState);
}
END_CMD

DEFINE_CMD(ADC_Open, data, nread)
{
    data->header.returns = ADC_Open(data->id);
    ledger_add_file_descriptor(data->header.returns);
}
END_CMD

DEFINE_CMD(ADC_GetSampleBitCount, data, nread)
{
    data->header.returns = ADC_GetSampleBitCount(data->fd, data->channel);
}
END_CMD

DEFINE_CMD(ADC_SetReferenceVoltage, data, nread)
{
    data->header.returns = ADC_SetReferenceVoltage(data->fd, data->channel, data->referenceVoltage);
}
END_CMD

DEFINE_CMD(ADC_Poll, data, nread)
{
    uint32_t outSampleValue = 0;

    data->header.returns = ADC_Poll(data->fd, data->channel, &outSampleValue);
    data->outSampleValue = outSampleValue;
}
END_CMD

DEFINE_CMD(SPIMaster_Open, data, nread)
{
    uint8_t *data_ptr = data->data_block.data;
    SPIMaster_Config *config = (SPIMaster_Config *)data_ptr;

    data->header.returns = SPIMaster_Open(data->interfaceId, data->chipSelectId, config);
    ledger_add_file_descriptor(data->header.returns);
}
END_CMD

DEFINE_CMD(SPIMaster_InitConfig, data, nread)
{
    uint8_t *data_ptr = data->data_block.data;
    SPIMaster_Config *config = (SPIMaster_Config *)data_ptr;
    data->header.returns = SPIMaster_InitConfig(config);
}
END_CMD

DEFINE_CMD(SPIMaster_SetBusSpeed, data, nread)
{
    data->header.returns = SPIMaster_SetBusSpeed(data->fd, data->speedInHz);
}
END_CMD

DEFINE_CMD(SPIMaster_SetMode, data, nread)
{
    data->header.returns = SPIMaster_SetMode(data->fd, data->mode);
}
END_CMD

DEFINE_CMD(SPIMaster_SetBitOrder, data, nread)
{
    data->header.returns = SPIMaster_SetBitOrder(data->fd, data->order);
}
END_CMD

DEFINE_CMD(SPIMaster_WriteThenRead, data, nread)
{
    data->header.returns = SPIMaster_WriteThenRead(data->fd,
                                                   (const uint8_t *)data->data_block.data,
                                                   data->lenWriteData,
                                                   data->data_block.data,
                                                   data->lenReadData);
}
END_CMD

DEFINE_CMD(SPIMaster_TransferSequential, data, nread)
{
    bool read_transfer = false, write_transfer = false;

    SPIMaster_Transfer transfers[data->transferCount];
    SPIMaster_InitTransfers(transfers, data->transferCount);

    uint8_t *data_ptr = data->data_block.data;

    for (size_t i = 0; i < data->transferCount; i++)
    {
        SPI_TransferConfig *transfer_config = (SPI_TransferConfig *)data_ptr;

        transfers[i].flags = transfer_config->flags;
        transfers[i].length = transfer_config->length;
        transfers[i].readData = NULL;
        transfers[i].writeData = NULL;

        data_ptr += sizeof(SPI_TransferConfig);

        read_transfer = transfer_config->flags == SPI_TransferFlags_Read ? true : read_transfer;
        write_transfer = transfer_config->flags == SPI_TransferFlags_Write ? true : write_transfer;
    }

    if (read_transfer && write_transfer)
    {
        Log_Debug("can't mix read and write transfers on a single SPI transaction\n");
    }

    if (read_transfer)
    {
        data_ptr = data->data_block.data;

        for (size_t i = 0; i < data->transferCount; i++)
        {
            transfers[i].readData = data_ptr;
            data_ptr += transfers[i].length;
        }
    }

    if (write_transfer)
    {
        for (size_t i = 0; i < data->transferCount; i++)
        {
            transfers[i].writeData = data_ptr;
            data_ptr += transfers[i].length;
        }
    }

    data->header.returns = SPIMaster_TransferSequential(data->fd, transfers, data->transferCount);
}
END_CMD

DEFINE_CMD(Storage_OpenMutableFile, data, nread)
{
    data->header.returns = Storage_OpenMutableFile();
    ledger_add_file_descriptor(data->header.returns);
}
END_CMD

DEFINE_CMD(Storage_DeleteMutableFile, data, nread)
{
    data->header.returns = Storage_DeleteMutableFile();
}
END_CMD

DEFINE_CMD(RemoteX_Write, data, nread)
{
    data->header.returns = write(data->fd, data->data_block.data, (size_t)data->length);
}
END_CMD

DEFINE_CMD(RemoteX_Read, data, nread)
{
    data->header.returns = read(data->fd, data->data_block.data, (size_t)data->length);
}
END_CMD

DEFINE_CMD(RemoteX_Lseek, data, nread)
{
    data->header.returns = (int)lseek(data->fd, data->offset, data->whence);
}
END_CMD

DEFINE_CMD(RemoteX_Close, data, nread)
{
    data->header.returns = close(data->fd);
    ledger_remove_file_descriptor(data->fd);
}
END_CMD

DEFINE_CMD(RemoteX_PlatformInformation, data, nread)
{
    data->header.returns = snprintf(data->data_block.data, (size_t)data->length, "Device platform: %s, Firmware version: %s", DEVICE_PLATFORM, FIRMWARE_VERSION);
}
END_CMD

DEFINE_CMD(UART_InitConfig, data, nread)
{
    uint8_t *data_ptr = data->data_block.data;
    UART_Config *uartConfig = (UART_Config *)data_ptr;
    UART_InitConfig(uartConfig);
}
END_CMD

DEFINE_CMD(UART_Open, data, nread)
{
    uint8_t *data_ptr = data->data_block.data;
    UART_Config *uartConfig = (UART_Config *)data_ptr;

    data->header.returns = UART_Open(data->uartId, uartConfig);
    ledger_add_file_descriptor(data->header.returns);
}
END_CMD