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
    for (size_t i = 0; i < LEDGE_SIZE; i++)
    {
        if (file_descriptor_ledger[i] == -1)
        {
            file_descriptor_ledger[i] = fd;
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

BEGIN_CMD(GPIO_OpenAsOutput, data, nread)
{
    data->returns = GPIO_OpenAsOutput(data->gpioId, data->outputMode, data->initialValue);
    data->err_no = errno;

    ledger_add_file_descriptor(data->returns);
}
END_CMD(GPIO_OpenAsOutput, nread)

BEGIN_CMD(GPIO_OpenAsInput, data, nread)
{
    data->returns = GPIO_OpenAsInput(data->gpioId);
    data->err_no = errno;

    ledger_add_file_descriptor(data->returns);
}
END_CMD(GPIO_OpenAsInput, nread)

BEGIN_CMD(GPIO_SetValue, data, nread)
{
    data->returns = GPIO_SetValue(data->gpioFd, data->value);
    data->err_no = errno;
}
END_CMD(GPIO_SetValue, data->header.respond ? nread : -1)

BEGIN_CMD(GPIO_GetValue, data, nread)
{
    GPIO_Value_Type outValue;

    data->returns = GPIO_GetValue(data->gpioFd, &outValue);
    data->outValue = outValue;
    data->err_no = errno;
}
END_CMD(GPIO_GetValue, nread)

BEGIN_CMD(I2CMaster_Open, data, nread)
{
    data->returns = I2CMaster_Open(data->I2C_InterfaceId);
    data->err_no = errno;

    ledger_add_file_descriptor(data->returns);
}
END_CMD(I2CMaster_Open, nread)

BEGIN_CMD(I2CMaster_SetBusSpeed, data, nread)
{
    data->returns = I2CMaster_SetBusSpeed(data->fd, data->speedInHz);
    data->err_no = errno;
}
END_CMD(I2CMaster_SetBusSpeed, nread)

BEGIN_CMD(I2CMaster_SetTimeout, data, nread)
{
    data->returns = I2CMaster_SetTimeout(data->fd, data->timeoutInMs);
    data->err_no = errno;
}
END_CMD(I2CMaster_SetTimeout, nread)

BEGIN_CMD(I2CMaster_Write, data, nread)
{
    data->returns = I2CMaster_Write(data->fd, data->address, (const uint8_t *)data->data_block.data, data->data_block.length);
    data->err_no = errno;

    // just return the core control block length minus the data block length
    nread = (int)(sizeof(I2CMaster_Write_t) - sizeof(((I2CMaster_Write_t *)0)->data_block));
}
END_CMD(I2CMaster_SetTimeout, data->header.respond ? nread : -1)

BEGIN_CMD(I2CMaster_WriteThenRead, data, nread)
{
    data->returns = I2CMaster_WriteThenRead(data->fd, data->address, (const uint8_t *)data->data_block.data, data->lenWriteData, (uint8_t *)data->data_block.data, data->lenReadData);
    data->err_no = errno;

    nread = (int)(sizeof(I2CMaster_WriteThenRead_t) -
                  sizeof(((I2CMaster_WriteThenRead_t *)0)->data_block.data) +
                  data->lenReadData);
}
END_CMD(I2CMaster_WriteThenRead, nread)

BEGIN_CMD(I2CMaster_Read, data, nread)
{
    data->returns = I2CMaster_Read(data->fd, data->address, data->data_block.data, data->data_block.length);
    data->err_no = errno;

    nread = (int)(sizeof(I2CMaster_Read_t) -
                  sizeof(((I2CMaster_Read_t *)0)->data_block.data) +
                  data->data_block.length);
}
END_CMD(I2CMaster_Read, nread)

BEGIN_CMD(I2CMaster_SetDefaultTargetAddress, data, nread)
{
    data->returns = I2CMaster_SetDefaultTargetAddress(data->fd, data->address);
    data->err_no = errno;
}
END_CMD(I2CMaster_SetDefaultTargetAddress, nread)

BEGIN_CMD(PWM_Open, data, nread)
{
    data->returns = PWM_Open(data->pwm);
    data->err_no = errno;

    ledger_add_file_descriptor(data->returns);
}
END_CMD(PWM_Open, nread)

BEGIN_CMD(PWM_Apply, data, nread)
{
    PwmState newState;
    newState.period_nsec = data->period_nsec;
    newState.dutyCycle_nsec = data->dutyCycle_nsec;
    newState.polarity = data->polarity;
    newState.enabled = data->enabled;

    data->returns = PWM_Apply(data->pwmFd, data->pwmChannel, &newState);
    data->err_no = errno;
}
END_CMD(PWM_Apply, data->header.respond ? nread : -1)

BEGIN_CMD(ADC_Open, data, nread)
{
    data->returns = ADC_Open(data->id);
    data->err_no = errno;

    ledger_add_file_descriptor(data->returns);
}
END_CMD(ADC_Open, nread)

BEGIN_CMD(ADC_GetSampleBitCount, data, nread)
{
    data->returns = ADC_GetSampleBitCount(data->fd, data->channel);
    data->err_no = errno;
}
END_CMD(ADC_GetSampleBitCount, nread)

BEGIN_CMD(ADC_SetReferenceVoltage, data, nread)
{
    data->returns = ADC_SetReferenceVoltage(data->fd, data->channel, data->referenceVoltage);
    data->err_no = errno;
}
END_CMD(ADC_GetSampleBitCount, nread)

BEGIN_CMD(ADC_Poll, data, nread)
{
    uint32_t outSampleValue = 0;

    data->returns = ADC_Poll(data->fd, data->channel, &outSampleValue);
    data->outSampleValue = outSampleValue;
    data->err_no = errno;
}
END_CMD(ADC_Poll, nread)

BEGIN_CMD(SPIMaster_Open, data, nread)
{
    SPIMaster_Config config;
    config.csPolarity = data->csPolarity;
    config.z__magicAndVersion = data->z__magicAndVersion;

    data->returns = SPIMaster_Open(data->interfaceId, data->chipSelectId, &config);
    data->err_no = errno;
    ledger_add_file_descriptor(data->returns);
}
END_CMD(SPIMaster_Open, nread)

BEGIN_CMD(SPIMaster_InitConfig, data, nread)
{
    SPIMaster_Config config;

    data->returns = SPIMaster_InitConfig(&config);
    data->err_no = errno;

    data->csPolarity = config.csPolarity;
    data->z__magicAndVersion = config.z__magicAndVersion;
}
END_CMD(SPIMaster_InitConfig, nread)

BEGIN_CMD(SPIMaster_SetBusSpeed, data, nread)
{
    data->returns = SPIMaster_SetBusSpeed(data->fd, data->speedInHz);
    data->err_no = errno;
}
END_CMD(SPIMaster_SetBusSpeed, nread)

BEGIN_CMD(SPIMaster_SetMode, data, nread)
{
    data->returns = SPIMaster_SetMode(data->fd, data->mode);
    data->err_no = errno;
}
END_CMD(SPIMaster_SetMode, nread)

BEGIN_CMD(SPIMaster_SetBitOrder, data, nread)
{
    data->returns = SPIMaster_SetBitOrder(data->fd, data->order);
    data->err_no = errno;
}
END_CMD(SPIMaster_SetBitOrder, nread)

BEGIN_CMD(SPIMaster_WriteThenRead, data, nread)
{
    data->returns = SPIMaster_WriteThenRead(data->fd,
                                            (const uint8_t *)data->data_block.data,
                                            data->lenWriteData,
                                            data->data_block.data,
                                            data->lenReadData);
    data->err_no = errno;
    data->data_block.length = (uint16_t)data->lenReadData;

    // The calculated return size is the size of the total data structure,
    // minus the size of the datablock,
    // plus the size of the data to be returned
    nread = (int)(sizeof(SPIMaster_WriteThenRead_t) -
                  sizeof(((SPIMaster_WriteThenRead_t *)0)->data_block.data) +
                  data->lenReadData);
}
END_CMD(SPIMaster_WriteThenRead, nread)

BEGIN_CMD(SPIMaster_TransferSequential, data, nread)
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
        data->data_block.length = 0;

        for (size_t i = 0; i < data->transferCount; i++)
        {
            transfers[i].readData = data_ptr;
            data_ptr += transfers[i].length;
            data->data_block.length = (uint16_t)(data->data_block.length + transfers[i].length);
        }

        nread = (int)(sizeof(SPIMaster_TransferSequential_t) -
                      sizeof(((SPIMaster_TransferSequential_t *)0)->data_block.data) +
                      data->data_block.length);
    }

    if (write_transfer)
    {
        for (size_t i = 0; i < data->transferCount; i++)
        {
            transfers[i].writeData = data_ptr;
            data_ptr += transfers[i].length;
        }

        // just return the core control block length minus the data block length
        nread = (int)(sizeof(SPIMaster_TransferSequential_t) - sizeof(((SPIMaster_TransferSequential_t *)0)->data_block));
    }

    data->returns = SPIMaster_TransferSequential(data->fd, transfers, data->transferCount);
    data->err_no = errno;
}
END_CMD(SPIMaster_TransferSequential, data->header.respond ? nread : -1)
