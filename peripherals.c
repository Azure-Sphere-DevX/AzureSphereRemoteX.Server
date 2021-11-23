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

int GPIO_OpenAsOutput_cmd(uint8_t *buf, ssize_t nread)
{
    GPIO_OpenAsOutput_t *data = (GPIO_OpenAsOutput_t *)buf;

    data->returns = GPIO_OpenAsOutput(data->gpioId, data->outputMode, data->initialValue);
    data->err_no = errno;

    ledger_add_file_descriptor(data->returns);

    return data->header.respond ? nread : -1;
}

int GPIO_OpenAsInput_cmd(uint8_t *buf, ssize_t nread)
{
    GPIO_OpenAsInput_t *data = (GPIO_OpenAsInput_t *)buf;

    data->returns = GPIO_OpenAsInput(data->gpioId);
    data->err_no = errno;

    ledger_add_file_descriptor(data->returns);

    return data->header.respond ? nread : -1;
}

int GPIO_SetValue_cmd(uint8_t *buf, ssize_t nread)
{
    GPIO_SetValue_t *data = (GPIO_SetValue_t *)buf;
    data->returns = GPIO_SetValue(data->gpioFd, data->value);
    data->err_no = errno;

    return data->header.respond ? nread : -1;
}

int GPIO_GetValue_cmd(uint8_t *buf, ssize_t nread)
{
    GPIO_Value_Type outValue;

    GPIO_GetValue_t *data = (GPIO_GetValue_t *)buf;
    data->returns = GPIO_GetValue(data->gpioFd, &outValue);
    data->outValue = outValue;
    data->err_no = errno;

    return data->header.respond ? nread : -1;
}

int I2CMaster_Open_cmd(uint8_t *buf, ssize_t nread)
{
    I2CMaster_Open_t *data = (I2CMaster_Open_t *)buf;

    data->returns = I2CMaster_Open(data->I2C_InterfaceId);
    data->err_no = errno;

    ledger_add_file_descriptor(data->returns);

    return data->header.respond ? nread : -1;
}

int I2CMaster_SetBusSpeed_cmd(uint8_t *buf, ssize_t nread)
{
    I2CMaster_SetBusSpeed_t *data = (I2CMaster_SetBusSpeed_t *)buf;

    data->returns = I2CMaster_SetBusSpeed(data->fd, data->speedInHz);
    data->err_no = errno;

    // Log_Debug("%s\n", __func__);
    return data->header.respond ? nread : -1;
}

int I2CMaster_SetTimeout_cmd(uint8_t *buf, ssize_t nread)
{
    I2CMaster_SetTimeout_t *data = (I2CMaster_SetTimeout_t *)buf;

    data->returns = I2CMaster_SetTimeout(data->fd, data->timeoutInMs);
    data->err_no = errno;

    return data->header.respond ? nread : -1;
}

int I2CMaster_Write_cmd(uint8_t *buf, ssize_t nread)
{
    I2CMaster_Write_t *data = (I2CMaster_Write_t *)buf;

    data->returns = I2CMaster_Write(data->fd, data->address, (const uint8_t *)data->data_block.data, data->data_block.length);
    data->err_no = errno;

    return data->header.respond ? nread : -1;
}

int I2CMaster_WriteThenRead_cmd(uint8_t *buf, ssize_t nread)
{
    I2CMaster_WriteThenRead_t *data = (I2CMaster_WriteThenRead_t *)buf;

    data->returns = I2CMaster_WriteThenRead(data->fd, data->address, (const uint8_t *)data->data_block.data, data->lenWriteData, (uint8_t *)data->data_block.data, data->lenReadData);
    data->err_no = errno;

    nread = (int)(sizeof(I2CMaster_WriteThenRead_t) -
                  sizeof(((I2CMaster_WriteThenRead_t *)0)->data_block.data) +
                  data->lenReadData);

    return data->header.respond ? nread : -1;
}

int I2CMaster_Read_cmd(uint8_t *buf, ssize_t nread)
{
    I2CMaster_Read_t *data = (I2CMaster_Read_t *)buf;

    data->returns = I2CMaster_Read(data->fd, data->address, data->data_block.data, data->data_block.length);
    data->err_no = errno;

    return data->header.respond ? nread : -1;
}

int I2CMaster_SetDefaultTargetAddress_cmd(uint8_t *buf, ssize_t nread)
{
    I2CMaster_SetDefaultTargetAddress_t *data = (I2CMaster_SetDefaultTargetAddress_t *)buf;

    data->returns = I2CMaster_SetDefaultTargetAddress(data->fd, data->address);
    data->err_no = errno;

    return data->header.respond ? nread : -1;
}

int PWM_Open_cmd(uint8_t *buf, ssize_t nread)
{
    PWM_Open_t *data = (PWM_Open_t *)buf;

    data->returns = PWM_Open(data->pwm);
    data->err_no = errno;

    ledger_add_file_descriptor(data->returns);

    return data->header.respond ? nread : -1;
}

int PWM_Apply_cmd(uint8_t *buf, ssize_t nread)
{
    PWM_Apply_t *data = (PWM_Apply_t *)buf;

    PwmState newState;
    newState.period_nsec = data->period_nsec;
    newState.dutyCycle_nsec = data->dutyCycle_nsec;
    newState.polarity = data->polarity;
    newState.enabled = data->enabled;

    data->returns = PWM_Apply(data->pwmFd, data->pwmChannel, &newState);
    data->err_no = errno;

    return data->header.respond ? nread : -1;
}

int ADC_Open_cmd(uint8_t *buf, ssize_t nread)
{
    ADC_Open_t *data = (ADC_Open_t *)buf;

    data->returns = ADC_Open(data->id);
    data->err_no = errno;

    ledger_add_file_descriptor(data->returns);

    return data->header.respond ? nread : -1;
}

int ADC_GetSampleBitCount_cmd(uint8_t *buf, ssize_t nread)
{
    ADC_GetSampleBitCount_t *data = (ADC_GetSampleBitCount_t *)buf;

    data->returns = ADC_GetSampleBitCount(data->fd, data->channel);
    data->err_no = errno;

    return data->header.respond ? nread : -1;
}

int ADC_SetReferenceVoltage_cmd(uint8_t *buf, ssize_t nread)
{
    ADC_SetReferenceVoltage_t *data = (ADC_SetReferenceVoltage_t *)buf;

    data->returns = ADC_SetReferenceVoltage(data->fd, data->channel, data->referenceVoltage);
    data->err_no = errno;

    return data->header.respond ? nread : -1;
}

int ADC_Poll_cmd(uint8_t *buf, ssize_t nread)
{
    ADC_Poll_t *data = (ADC_Poll_t *)buf;

    uint32_t outSampleValue = 0;

    data->returns = ADC_Poll(data->fd, data->channel, &outSampleValue);
    data->outSampleValue = outSampleValue;
    data->err_no = errno;

    return data->header.respond ? nread : -1;
}

int SPIMaster_Open_cmd(uint8_t *buf, ssize_t nread)
{
    SPIMaster_Open_t *data = (SPIMaster_Open_t *)buf;

    SPIMaster_Config config;
    config.csPolarity = data->csPolarity;
    config.z__magicAndVersion = data->z__magicAndVersion;

    data->returns = SPIMaster_Open(data->interfaceId, data->chipSelectId, &config);
    data->err_no = errno;
    ledger_add_file_descriptor(data->returns);

    return data->header.respond ? nread : -1;
}

int SPIMaster_InitConfig_cmd(uint8_t *buf, ssize_t nread)
{
    SPIMaster_InitConfig_t *data = (SPIMaster_InitConfig_t *)buf;
    SPIMaster_Config config;

    data->returns = SPIMaster_InitConfig(&config);
    data->err_no = errno;

    data->csPolarity = config.csPolarity;
    data->z__magicAndVersion = config.z__magicAndVersion;

    return data->header.respond ? nread : -1;
}

int SPIMaster_SetBusSpeed_cmd(uint8_t *buf, ssize_t nread)
{
    SPIMaster_SetBusSpeed_t *data = (SPIMaster_SetBusSpeed_t *)buf;

    data->returns = SPIMaster_SetBusSpeed(data->fd, data->speedInHz);
    data->err_no = errno;

    return data->header.respond ? nread : -1;
}

int SPIMaster_SetMode_cmd(uint8_t *buf, ssize_t nread)
{
    SPIMaster_SetMode_t *data = (SPIMaster_SetMode_t *)buf;

    data->returns = SPIMaster_SetMode(data->fd, data->mode);
    data->err_no = errno;

    return data->header.respond ? nread : -1;
}

int SPIMaster_SetBitOrder_cmd(uint8_t *buf, ssize_t nread)
{
    SPIMaster_SetBitOrder_t *data = (SPIMaster_SetBitOrder_t *)buf;

    data->returns = SPIMaster_SetBitOrder(data->fd, data->order);
    data->err_no = errno;

    return data->header.respond ? nread : -1;
}

int SPIMaster_WriteThenRead_cmd(uint8_t *buf, ssize_t nread)
{
    SPIMaster_WriteThenRead_t *data = (SPIMaster_WriteThenRead_t *)buf;

    // uint8_t *data_ptr = data->data_block.data;

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

    return data->header.respond ? nread : -1;
}

int SPIMaster_InitTransfers_cmd(uint8_t *buf, ssize_t nread)
{
    SPIMaster_InitTransfers_t *data = (SPIMaster_InitTransfers_t *)buf;

    SPIMaster_Transfer transfer;
    transfer.z__magicAndVersion = data->z__magicAndVersion;

    data->returns = SPIMaster_InitTransfers(&transfer, data->transferCount);
    data->err_no = errno;

    data->z__magicAndVersion = transfer.z__magicAndVersion;

    return data->header.respond ? nread : -1;
}

int SPIMaster_TransferSequential_cmd(uint8_t *buf, ssize_t nread)
{
    bool read_transfer = false, write_transfer = false;

    SPIMaster_TransferSequential_t *data = (SPIMaster_TransferSequential_t *)buf;

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
    else
    {
        for (size_t i = 0; i < data->transferCount; i++)
        {
            transfers[i].writeData = data_ptr;
            data_ptr += transfers[i].length;
        }
    }

    data->returns = SPIMaster_TransferSequential(data->fd, transfers, data->transferCount);
    data->err_no = errno;

    return data->header.respond ? nread : -1;
}
