#include <i2c.h>
#include <tick1ms.h>
#include <string.h>

static volatile uint8_t i2c_txdata[MAXI2CTXDATA];
static volatile uint8_t i2c_rxdata[MAXI2CTXDATA];

static volatile int i2c_TXByteCounter;
static volatile int i2c_RXByteCounter;

static volatile uint8_t i2c_mode;
static volatile uint8_t i2cBusyFlag;
static volatile uint16_t deviceAddress;

void I2C3_EV_IRQHandler(void)
{
	static int idx;

	if(I2C_GetFlagStatus(I2C3, I2C_FLAG_SB) == SET) {
		// START wurde erzeugt, sende jetzt die Device-Adresse
		if(i2c_mode == I2C_TX || i2c_mode == I2C_RX_SENDADR) {
			// STM32 Transmitter
			I2C_Send7bitAddress(I2C3, deviceAddress, I2C_Direction_Transmitter);
		} else {
			// STM32 Receiver
			I2C_Send7bitAddress(I2C3, deviceAddress, I2C_Direction_Receiver);
		}
		idx = 0;
		return;
	}

	if(I2C_GetFlagStatus(I2C3, I2C_FLAG_ADDR) == SET || I2C_GetFlagStatus(I2C3, I2C_FLAG_BTF) == SET) {
		// Device Adresse wurde gesendet und vom Slave bestätigt
		I2C_ReadRegister(I2C3, I2C_Register_SR1);
		I2C_ReadRegister(I2C3, I2C_Register_SR2);
		if(i2c_mode == I2C_TX) {
			// stm -> device
			if(i2c_TXByteCounter == 0) {
				if(i2cBusyFlag == 0) {
					I2C3->SR1 = 0;
					I2C3->SR2 = 0;
					return;
				}
				I2C_GenerateSTOP(I2C3, ENABLE);
				I2C3->SR1 = 0;
				I2C3->SR2 = 0;
				i2cBusyFlag = 0;
			} else {
				I2C_SendData(I2C3, i2c_txdata[idx++]);
				i2c_TXByteCounter--;
			}
		}

		if(i2c_mode == I2C_RX_SENDADR) {
			// stm -> device, send eeprom Address
			if(i2c_TXByteCounter == 0)
			{
				I2C_GenerateSTART(I2C3, ENABLE);
				i2c_mode = I2C_RX;
				idx = 0;
			}
			else
			{
				I2C_SendData(I2C3, i2c_txdata[idx++]);
				i2c_TXByteCounter--;
			}
		}
		return;
	}

	if(I2C_GetFlagStatus(I2C3, I2C_FLAG_RXNE) == SET) {
		// STM32 Receiver
		I2C_ReadRegister(I2C3, I2C_Register_SR1);
		I2C_ReadRegister(I2C3, I2C_Register_SR2);
		i2c_RXByteCounter--;
		if(i2c_RXByteCounter == 0) {
			I2C_AcknowledgeConfig(I2C3, DISABLE);
			I2C_GenerateSTOP(I2C3, ENABLE);
			i2c_rxdata[idx++] = I2C_ReceiveData(I2C3);
			i2cBusyFlag = 0;
		} else {
			i2c_rxdata[idx++] = I2C_ReceiveData(I2C3);
		}
		return;
	}
	I2C3->SR1 = 0;
	I2C3->SR2 = 0;
}

void I2C3_ER_IRQHandler(void)
{
	I2C_GenerateSTOP(I2C3, ENABLE);
	i2cBusyFlag = 0;

	I2C_ClearFlag(I2C3, I2C_FLAG_AF);
	I2C_ClearFlag(I2C3, I2C_FLAG_ARLO);
	I2C_ClearFlag(I2C3, I2C_FLAG_BERR);
}

void i2c_wait_ready(void)
{
	while(i2cBusyFlag);
	while(I2C_GetFlagStatus(I2C3,I2C_FLAG_BUSY));
}

void i2c_sendbyte(uint16_t adr, uint8_t data)
{
	i2c_wait_ready();
	i2c_txdata[0] = adr >> 8;
	i2c_txdata[1] = adr;
	i2c_txdata[2] = data;
	i2c_TXByteCounter = 3;
	i2c_mode = I2C_TX;

	i2cBusyFlag = 1;
	I2C_GenerateSTART(I2C3, ENABLE);
	while(i2cBusyFlag);
	delay_1ms(10);	// das EEPROM braucht min 5ms um seine Zellen zu schreiben
}

void i2c_write(uint8_t slave, uint8_t *bytes, int len)
{
	i2c_wait_ready();
	memcpy((void *)i2c_txdata, bytes, len);
	deviceAddress = slave;
	i2c_TXByteCounter = len;
	i2c_mode = I2C_TX;
	i2cBusyFlag = 1;
	I2C_AcknowledgeConfig(I2C3, ENABLE);
	I2C_GenerateSTART(I2C3, ENABLE);
	while(i2cBusyFlag);
}

uint8_t i2c_readbyte(uint16_t adr)
{
	i2c_wait_ready();
	i2c_txdata[0] = adr >> 8;
	i2c_txdata[1] = adr;
	i2c_TXByteCounter = 2;
	i2c_mode = I2C_RX_SENDADR;

	i2c_RXByteCounter = 1;

	i2cBusyFlag = 1;
	I2C_AcknowledgeConfig(I2C3, ENABLE);
	I2C_GenerateSTART(I2C3, ENABLE);
	while(i2cBusyFlag);

	return i2c_rxdata[0];
}

void i2c_sendpage64(uint8_t slave, uint16_t adr, uint8_t* pdata)
{
	i2c_wait_ready();
	i2c_txdata[0] = adr>>8;
	i2c_txdata[1] = adr;
	for(int i=0; i<64; i++)
		i2c_txdata[2+i] = *pdata++;
	i2c_TXByteCounter = 64+2;
	deviceAddress = slave;
	i2c_mode = I2C_TX;

	i2cBusyFlag = 1;
	I2C_GenerateSTART(I2C3, ENABLE);
	while(i2cBusyFlag);
	delay_1ms(10);	// das EEPROM braucht min 5ms um seine Zellen zu schreiben
}

void i2c_readpage64(uint8_t slave, uint16_t adr, uint8_t* pdata)
{
	i2c_wait_ready();
	i2c_txdata[0] = adr >> 8;
	i2c_txdata[1] = adr;
	i2c_TXByteCounter = 2;
	i2c_mode = I2C_RX_SENDADR;

	i2c_RXByteCounter = 64;
	deviceAddress = slave;
	i2cBusyFlag = 1;
	I2C_AcknowledgeConfig(I2C3, ENABLE);
	I2C_GenerateSTART(I2C3, ENABLE);
	while(i2cBusyFlag);

	for(int i=0; i<64; i++)
		pdata[i] = i2c_rxdata[i];
}

void i2c_init(void)
{
		// I2C-3 Clock
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C3, ENABLE);
	// Schalte den Takt für GPIOA und C ein
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	// WP (PC8): Schalte auf Output
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_ResetBits(GPIOC, GPIO_Pin_8);

	// define alternate Function SCL, SDA
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_I2C3);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource9, GPIO_AF_I2C3);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	// Init NVIC
	NVIC_InitTypeDef   NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = I2C3_EV_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = I2C3_ER_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	// I2C Config
	I2C_DeInit(I2C3);

	I2C_InitTypeDef i2ctd;
	i2ctd.I2C_ClockSpeed = 400000;
	i2ctd.I2C_Mode = I2C_Mode_I2C;
	i2ctd.I2C_DutyCycle = I2C_DutyCycle_2;
	i2ctd.I2C_OwnAddress1 = 0;
	i2ctd.I2C_Ack = I2C_Ack_Enable;
	i2ctd.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_Init(I2C3, &i2ctd);

	I2C_ITConfig(I2C3, I2C_IT_EVT, ENABLE);
	I2C_ITConfig(I2C3, I2C_IT_BUF, ENABLE);
	I2C_ITConfig(I2C3, I2C_IT_ERR, ENABLE);

	i2cBusyFlag = 0;
	I2C_Cmd(I2C3, ENABLE);
}
