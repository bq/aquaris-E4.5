
#include "typedefs.h"
#include "platform.h"
#include "uart.h"
#include "meta.h"
#include "i2c.h"

//#define MT8193_CHIP_ADDR                0x3A
#define MT8193_CHIP_ADDR                0x1D

#if 0
static U32 _mt8193_i2c_read (U8 chip, U8 *cmdBuffer, int cmdBufferLen, U8 *dataBuffer, int dataBufferLen)
{
    U32 ret_code = I2C_OK;

    ret_code = mt_i2c_write(I2C0, chip, cmdBuffer, cmdBufferLen, 0);    // set register command
    if (ret_code != I2C_OK)
        return ret_code;

    ret_code = mt_i2c_read(I2C0, chip, dataBuffer, dataBufferLen, 0);

    dbg_print("[_mt8193_i2c_read] Done\n");

    return ret_code;
}

static U32 _mt8193_i2c_write (U8 chip, U8 *cmdBuffer, int cmdBufferLen, U8 *dataBuffer, int dataBufferLen)
{
    U32 ret_code = I2C_OK;
    U8 write_data[I2C_FIFO_SIZE];
    int transfer_len = cmdBufferLen + dataBufferLen;
    int i=0, cmdIndex=0, dataIndex=0;

    if(I2C_FIFO_SIZE < (cmdBufferLen + dataBufferLen))
    {   return -1;
    }

    //write_data[0] = cmd;
    //write_data[1] = writeData;

    while(cmdIndex < cmdBufferLen)
    {
        write_data[i] = cmdBuffer[cmdIndex];
        cmdIndex++;
        i++;
    }

    while(dataIndex < dataBufferLen)
    {
        write_data[i] = dataBuffer[dataIndex];
        dataIndex++;
        i++;
    }

    /* dump write_data for check */
    for( i=0 ; i < transfer_len ; i++ )
    {
        dbg_print("[mt8193_i2c_write] write_data[%d]=%x\n", i, write_data[i]);
    }

    ret_code = mt_i2c_write(I2C0, chip, write_data, transfer_len, 0);

    dbg_print("[mt8193_i2c_write] Done\n");
    
    return ret_code;
}

u8 mt8193_i2c_read8(u16 addr)
{
    U8 chip_slave_address = MT8193_CHIP_ADDR;
    U8 cmd = addr;
	U32 result_tmp;
	int cmd_len = 1;
	U8 data = 0xFF;
    int data_len = 1;

	cmd = addr;
	result_tmp = _mt8193_i2c_read(MT8193_CHIP_ADDR, cmd, cmd_len, &data, data_len);
	
    return result_tmp;
}

int mt8193_i2c_write8(u16 addr, u8 value)
{
    U8 chip_slave_address = MT8193_CHIP_ADDR;
    U8 cmd = addr;
    int cmd_len = 1;
    U8 data = value;
    int data_len = 1;	
    U32 result_tmp;

    cmd = addr;	

    result_tmp = _mt8193_i2c_write(chip_slave_address, &cmd, cmd_len, &data, data_len);

    //check 
    result_tmp = _mt8193_i2c_read(chip_slave_address, &cmd, cmd_len, &data, data_len);
	
	printf("[mt8193_i2c_write] Reg[0x%x]=0x%x\n", addr, data);

	return 0;
}

u16 mt8193_i2c_read16(u16 addr)
{
    U8 chip_slave_address = MT8193_CHIP_ADDR;
    U16 cmd = addr;
	U32 result_tmp;
	int cmd_len = 2;
	U16 data = 0xFFFF;
    int data_len = 2;

	cmd = addr;
	result_tmp = _mt8193_i2c_read(MT8193_CHIP_ADDR, cmd, cmd_len, &data, data_len);
	
    return result_tmp;
}

int mt8193_i2c_write16(u16 addr, u16 value)
{
    U8 chip_slave_address = MT8193_CHIP_ADDR;
    U16 cmd = addr;
    int cmd_len = 2;
    U16 data = value;
    int data_len = 2;	
    U32 result_tmp;

    cmd = addr;	

    result_tmp = _mt8193_i2c_write(chip_slave_address, &cmd, cmd_len, &data, data_len);

    //check 
    result_tmp = _mt8193_i2c_read(chip_slave_address, &cmd, cmd_len, &data, data_len);
	
	printf("[mt8193_i2c_write] Reg[0x%x]=0x%x\n", addr, data);

	return 0;
}

u32 mt8193_i2c_read32(u16 addr)
{
    U8 chip_slave_address = MT8193_CHIP_ADDR;
    U16 cmd = addr;
	U32 result_tmp;
	int cmd_len = 2;
	U32 data = 0xFFFFFFFF;
    int data_len = 4;

	cmd = addr;
	result_tmp = _mt8193_i2c_read(MT8193_CHIP_ADDR, cmd, cmd_len, &data, data_len);
	
    return result_tmp;
}

int mt8193_i2c_write32(u16 addr, u32 value)
{
    U8 chip_slave_address = MT8193_CHIP_ADDR;
    U16 cmd = addr;
    int cmd_len = 2;
    U32 data = value;
    int data_len = 4;	
    U32 result_tmp;

    cmd = addr;	

    result_tmp = _mt8193_i2c_write(chip_slave_address, &cmd, cmd_len, &data, data_len);

    //check 
    result_tmp = _mt8193_i2c_read(chip_slave_address, &cmd, cmd_len, &data, data_len);
	
	printf("[mt8193_i2c_write] Reg[0x%x]=0x%x\n", addr, data);

	return 0;
}

#endif


#define MT8193_I2C_ID I2C0

u8 mt8193_i2c_read8(u16 addr)
{
    u8 rxBuf[8] = {0};
    u8 lens;
    U32 ret_code = 0;
    u8 data;

    mt_i2c i2c = {0};

    i2c.id = MT8193_I2C_ID;
    i2c.addr = MT8193_CHIP_ADDR;
    i2c.mode = ST_MODE;
    i2c.speed = 100;
    i2c.dma_en = 0;

    if(((addr >> 8) & 0xFF) >= 0x80) // 8 bit : fast mode
    {
        rxBuf[0] = (addr >> 8) & 0xFF;
        lens = 1;
    }
    else // 16 bit : noraml mode
    {
        rxBuf[0] = ( addr >> 8 ) & 0xFF;
        rxBuf[1] = addr & 0xFF;     
        lens = 2;
    }

    // ret_code = mt_i2c_write(MT8193_I2C_ID, MT8193_CHIP_ADDR, rxBuf, lens, 0);    // set register command
    ret_code = i2c_write(&i2c, rxBuf, lens);
    if (ret_code != I2C_OK)
        return ret_code;

    lens = 1;
    // ret_code = mt_i2c_read(I2C2, MT8193_CHIP_ADDR, rxBuf, lens, 0);
    ret_code = i2c_read(&i2c, rxBuf, lens);
    if (ret_code != I2C_OK)
    {
        return ret_code;
    }
    
    data = rxBuf[0]; //LSB fisrt
    
    return data;
    
}

int mt8193_i2c_write8(u16 addr, u8 data)
{
    u8 buffer[8];
    u8 lens;
    u32 ret_code = 0;
    u32 result_tmp = 0;

    mt_i2c i2c = {0};

    i2c.id = MT8193_I2C_ID;
    i2c.addr = MT8193_CHIP_ADDR;
    i2c.mode = ST_MODE;
    i2c.speed = 100;
    i2c.dma_en = 0;

    if(((addr >> 8) & 0xFF) >= 0x80) // 8 bit : fast mode
    {
        buffer[0] = (addr >> 8) & 0xFF;
        buffer[1] = data & 0xFF;
        lens = 2;
    }
    else // 16 bit : noraml mode
    {
        buffer[0] = (addr >> 8) & 0xFF;
        buffer[1] = addr & 0xFF;
        buffer[2] = data & 0xFF;        
        lens = 3;
    }

    // ret_code = mt_i2c_write(I2C2, MT8193_CHIP_ADDR, buffer, lens, 0); // 0:I2C_PATH_NORMAL
    ret_code = i2c_write(&i2c, buffer, lens); 
    if (ret_code != 0)
    {
        return ret_code;
    }
    
    //check 
    result_tmp = mt8193_i2c_read8(addr);
    
    printf("[mt8193_i2c_write] Reg[0x%x]=0x%x\n", addr, data);

    return 0;
}

u16 mt8193_i2c_read16(u16 addr)
{
    
    u8 rxBuf[8] = {0};
    u8 lens;
    U32 ret_code = 0;
    u16 data;
    mt_i2c i2c = {0};

    i2c.id = MT8193_I2C_ID;
    i2c.addr = MT8193_CHIP_ADDR;
    i2c.mode = ST_MODE;
    i2c.speed = 100;
    i2c.dma_en = 0;

    if(((addr >> 8) & 0xFF) >= 0x80) // 8 bit : fast mode
    {
        rxBuf[0] = (addr >> 8) & 0xFF;
        lens = 1;
    }
    else // 16 bit : noraml mode
    {
        rxBuf[0] = ( addr >> 8 ) & 0xFF;
        rxBuf[1] = addr & 0xFF;     
        lens = 2;
    }

    // ret_code = mt_i2c_write(I2C2, MT8193_CHIP_ADDR, rxBuf, lens, 0);    // set register command
    ret_code = i2c_write(&i2c, rxBuf, lens);
    if (ret_code != I2C_OK)
        return ret_code;

    lens = 2;
    // ret_code = mt_i2c_read(I2C2, MT8193_CHIP_ADDR, rxBuf, lens, 0);
    ret_code = i2c_read(&i2c, rxBuf, lens);
    if (ret_code != I2C_OK)
    {
        return ret_code;
    }
    
    data = (rxBuf[1] << 8) | (rxBuf[0]); //LSB fisrt
    
    return data;
    
}

int mt8193_i2c_write16(u16 addr, u16 data)
{
    u8 buffer[8];
    u8 lens;
    u32 ret_code = 0;
    u32 result_tmp = 0;
    mt_i2c i2c = {0};

    i2c.id = MT8193_I2C_ID;
    i2c.addr = MT8193_CHIP_ADDR;
    i2c.mode = ST_MODE;
    i2c.speed = 100;
    i2c.dma_en = 0;

    if(((addr >> 8) & 0xFF) >= 0x80) // 8 bit : fast mode
    {
        buffer[0] = (addr >> 8) & 0xFF;
        buffer[1] = (data >> 8) & 0xFF;
        buffer[2] = data & 0xFF;
        lens = 3;
    }
    else // 16 bit : noraml mode
    {
        buffer[0] = (addr >> 8) & 0xFF;
        buffer[1] = addr & 0xFF;
        buffer[2] = (data >> 8) & 0xFF;
        buffer[3] = data & 0xFF;        
        lens = 4;
    }

    // ret_code = mt_i2c_write(I2C2, MT8193_CHIP_ADDR, buffer, lens, 0); // 0:I2C_PATH_NORMAL
    ret_code = i2c_write(&i2c, buffer, lens); 
    if (ret_code != 0)
    {
        return ret_code;
    }
    
    //check 
    result_tmp = mt8193_i2c_read16(addr);
    
    printf("[mt8193_i2c_write] Reg[0x%x]=0x%x\n", addr, data);

    return 0;
}

u32 mt8193_i2c_read32(u16 addr)
{
    
    u8 rxBuf[8] = {0};
    u8 lens;
    U32 ret_code = 0;
    u32 data;
    mt_i2c i2c = {0};

    i2c.id = MT8193_I2C_ID;
    i2c.addr = MT8193_CHIP_ADDR;
    i2c.mode = ST_MODE;
    i2c.speed = 100;
    i2c.dma_en = 0;

    if(((addr >> 8) & 0xFF) >= 0x80) // 8 bit : fast mode
    {
        rxBuf[0] = (addr >> 8) & 0xFF;
        lens = 1;
    }
    else // 16 bit : noraml mode
    {
        rxBuf[0] = ( addr >> 8 ) & 0xFF;
        rxBuf[1] = addr & 0xFF;     
        lens = 2;
    }

    // ret_code = mt_i2c_write(I2C2, MT8193_CHIP_ADDR, rxBuf, lens, 0);    // set register command
    ret_code = i2c_write(&i2c, rxBuf, lens);
    if (ret_code != I2C_OK)
        return ret_code;

    lens = 4;
    // ret_code = mt_i2c_read(I2C2, MT8193_CHIP_ADDR, rxBuf, lens, 0);
    ret_code = i2c_read(&i2c, rxBuf, lens);
    if (ret_code != I2C_OK)
    {
        return ret_code;
    }
    
    data = (rxBuf[3] << 24) | (rxBuf[2] << 16) | (rxBuf[1] << 8) | (rxBuf[0]); //LSB fisrt

    return data;
    
}

int mt8193_i2c_write32(u16 addr, u32 data)
{
    u8 buffer[8];
    u8 lens;
    u32 ret_code = 0;
    u32 result_tmp = 0;
    mt_i2c i2c = {0};

    i2c.id = MT8193_I2C_ID;
    i2c.addr = MT8193_CHIP_ADDR;
    i2c.mode = ST_MODE;
    i2c.speed = 100;
    i2c.dma_en = 0;

    if(((addr >> 8) & 0xFF) >= 0x80) // 8 bit : fast mode
    {
        buffer[0] = (addr >> 8) & 0xFF;
        buffer[1] = (data >> 24) & 0xFF;
        buffer[2] = (data >> 16) & 0xFF;
        buffer[3] = (data >> 8) & 0xFF;
        buffer[4] = data & 0xFF;
        lens = 5;
    }
    else // 16 bit : noraml mode
    {
        buffer[0] = (addr >> 8) & 0xFF;
        buffer[1] = addr & 0xFF;
        buffer[2] = (data >> 24) & 0xFF;
        buffer[3] = (data >> 16) & 0xFF;
        buffer[4] = (data >> 8) & 0xFF;
        buffer[5] = data & 0xFF;        
        lens = 6;
    }

    // ret_code = mt_i2c_write(I2C2, MT8193_CHIP_ADDR, buffer, lens, 0); // 0:I2C_PATH_NORMAL
    ret_code = i2c_write(&i2c, buffer, lens); 
    if (ret_code != 0)
    {
        return ret_code;
    }
    
    //check 
    result_tmp = mt8193_i2c_read32(addr);
    
    printf("[mt8193_i2c_write] Reg[0x%x]=0x%x\n", addr, data);

    return 0;
}





