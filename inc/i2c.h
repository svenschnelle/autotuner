#ifndef TUNER_I2C_H
#define TUNER_I2C_H

#include <stdint.h>

enum _I2C_MODE_
{
	I2C_TX = 0,
	I2C_RX,
	I2C_RX_SENDADR
};

#define MAXI2CTXDATA	70	    // 64 Bytes Page + Adress = max 66 Bytes

void i2c_init(void);
void i2c_wait_ready();
void i2c_readpage64(uint8_t slave, uint16_t adr, uint8_t* pdata);
void i2c_sendpage64(uint8_t slave, uint16_t adr, uint8_t* pdata);
void i2c_write(uint8_t slave, uint8_t *bytes, int len);
#endif
