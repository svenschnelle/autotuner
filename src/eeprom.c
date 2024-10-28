/*
 * eeprom.c
 *
 *  Created on: 14.05.2018
 *      Author: kurt
 */

/*
 * I2C3:
 * SCL ... PA8
 * SDA ... PC9
 *
 * GPIO out:
 * WP .... PC8
 */

#include <main.h>

enum _I2C_MODE_
{
	I2C_TX = 0,
	I2C_RX,
	I2C_RX_SENDADR
};

volatile uint8_t i2c_txdata[MAXI2CTXDATA];
volatile uint8_t i2c_rxdata[MAXI2CTXDATA];

volatile uint16_t deviceAddress = EEPROMADR;
volatile int i2c_TXByteCounter;
volatile int i2c_RXByteCounter;
volatile uint8_t i2c_mode;

volatile uint8_t i2cBusyFlag;

void init_eeprom()
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

	copyEeToMem();
}

void I2C3_EV_IRQHandler(void)
{
static int idx;

	if(I2C_GetFlagStatus(I2C3, I2C_FLAG_SB) == SET)
	{
		// START wurde erzeugt, sende jetzt die Device-Adresse
		if(i2c_mode == I2C_TX || i2c_mode == I2C_RX_SENDADR)
		{
			// STM32 Transmitter
			I2C_Send7bitAddress(I2C3, deviceAddress, I2C_Direction_Transmitter);
		}
		else
		{
			// STM32 Receiver
			I2C_Send7bitAddress(I2C3, deviceAddress, I2C_Direction_Receiver);
		}
		idx = 0;
	}
	else if(I2C_GetFlagStatus(I2C3, I2C_FLAG_ADDR) == SET || I2C_GetFlagStatus(I2C3, I2C_FLAG_BTF) == SET)
	{
		// Device Adresse wurde gesendet und vom Slave bestätigt
		I2C_ReadRegister(I2C3, I2C_Register_SR1);
		I2C_ReadRegister(I2C3, I2C_Register_SR2);
		if(i2c_mode == I2C_TX)
		{
			// stm -> device
			if(i2c_TXByteCounter == 0)
			{
				if(i2cBusyFlag == 0)
				{
					I2C3->SR1 = 0;
					I2C3->SR2 = 0;
					return;
				}
				I2C_GenerateSTOP(I2C3, ENABLE);
				I2C3->SR1 = 0;
				I2C3->SR2 = 0;
				i2cBusyFlag = 0;
			}
			else
			{
				I2C_SendData(I2C3, i2c_txdata[idx++]);
				i2c_TXByteCounter--;
			}
		}
		if(i2c_mode == I2C_RX_SENDADR)
		{
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
	}
	else if(I2C_GetFlagStatus(I2C3, I2C_FLAG_RXNE) == SET)
	{
		// STM32 Receiver
		I2C_ReadRegister(I2C3, I2C_Register_SR1);
		I2C_ReadRegister(I2C3, I2C_Register_SR2);
		i2c_RXByteCounter--;
		if(i2c_RXByteCounter == 0)
		{
			I2C_AcknowledgeConfig(I2C3, DISABLE);
			I2C_GenerateSTOP(I2C3, ENABLE);
			i2c_rxdata[idx++] = I2C_ReceiveData(I2C3);
			i2cBusyFlag = 0;
		}
		else
		{
			i2c_rxdata[idx++] = I2C_ReceiveData(I2C3);
		}
	}
	else
	{
		I2C3->SR1 = 0;
		I2C3->SR2 = 0;
	}
}

void I2C3_ER_IRQHandler(void)
{
	I2C_GenerateSTOP(I2C3, ENABLE);
	i2cBusyFlag = 0;

	I2C_ClearFlag(I2C3, I2C_FLAG_AF);
	I2C_ClearFlag(I2C3, I2C_FLAG_ARLO);
	I2C_ClearFlag(I2C3, I2C_FLAG_BERR);
}

void i2c_sendbyte(uint16_t adr, uint8_t data)
{
	while(i2cBusyFlag);
	while(I2C_GetFlagStatus(I2C3,I2C_FLAG_BUSY));

	i2c_txdata[0] = adr>>8;
	i2c_txdata[1] = adr;
	i2c_txdata[2] = data;
	i2c_TXByteCounter = 3;
	i2c_mode = I2C_TX;

	i2cBusyFlag = 1;
	I2C_GenerateSTART(I2C3, ENABLE);
	while(i2cBusyFlag);
	delay_1ms(10);	// das EEPROM braucht min 5ms um seine Zellen zu schreiben
}

// sende eine 64 Byte Page
void i2c_sendpage64(uint16_t adr, uint8_t* pdata)
{
	while(i2cBusyFlag);
	while(I2C_GetFlagStatus(I2C3,I2C_FLAG_BUSY));

	i2c_txdata[0] = adr>>8;
	i2c_txdata[1] = adr;
	for(int i=0; i<64; i++)
		i2c_txdata[2+i] = *pdata++;
	i2c_TXByteCounter = 64+2;
	i2c_mode = I2C_TX;

	i2cBusyFlag = 1;
	I2C_GenerateSTART(I2C3, ENABLE);
	while(i2cBusyFlag);
	delay_1ms(10);	// das EEPROM braucht min 5ms um seine Zellen zu schreiben
}

uint8_t i2c_readbyte(uint16_t adr)
{
	while(i2cBusyFlag);
	while(I2C_GetFlagStatus(I2C3,I2C_FLAG_BUSY));

	i2c_txdata[0] = adr>>8;
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

void i2c_readpage64(uint16_t adr, uint8_t* pdata)
{
	while(i2cBusyFlag);
	while(I2C_GetFlagStatus(I2C3,I2C_FLAG_BUSY));

	i2c_txdata[0] = adr>>8;
	i2c_txdata[1] = adr;
	i2c_TXByteCounter = 2;
	i2c_mode = I2C_RX_SENDADR;

	i2c_RXByteCounter = 64;

	i2cBusyFlag = 1;
	I2C_AcknowledgeConfig(I2C3, ENABLE);
	I2C_GenerateSTART(I2C3, ENABLE);
	while(i2cBusyFlag);

	for(int i=0; i<64; i++)
		pdata[i] = i2c_rxdata[i];
}

// ============= EEPROM Funktionen ===================

/* Das EEPROM hat 256kbit = 32kByte mit 64 Byte Pages
 * das sind 512 Pages
 *
 */

// markiert eine Modifikation der Page damit diese gespeichert wird
uint8_t page_modified[PAGES];

// Speicherabbild des eeproms
uint8_t ee_mem[EESIZE];

// Konfiguration für Page 0
CONFIG eeconfig;


// schreibt geänderte Pages ins EEPROM
void copyMemToEE()
{
	for(int page = 0; page < PAGES; page++)
	{
		if(page_modified[page])
		{
			page_modified[page] = 0;
			i2c_sendpage64(page*PAGESIZE, ee_mem+page*PAGESIZE);
		}
	}
}

// kopiert die Config ins eemem
void configToEEmem()
{
uint8_t *p = (uint8_t *)(&eeconfig);

	uint16_t crc16 = crc16_messagecalc(p, sizeof(CONFIG));

	int idx = 0;
	for(int i=0; i<sizeof(CONFIG); i++)
		ee_mem[idx++] = p[i];

	ee_mem[idx++] = crc16 >> 8;
	ee_mem[idx] = crc16;

	for(int i=0; i<=(sizeof(CONFIG) / 64); i++)
		page_modified[i] = 1;
}

void EEmemToConfig()
{
uint16_t crc16;
uint8_t *p = (uint8_t *)(&eeconfig);

	int idx = 0;
	for(int i=0; i<sizeof(CONFIG); i++)
		p[i] = ee_mem[idx++];

	crc16 = ee_mem[idx++];
	crc16 <<= 8;
	crc16 += ee_mem[idx];

	uint16_t crc = crc16_messagecalc(p, sizeof(CONFIG));
	if(crc != crc16)
	{
		printinfo("EEprom CRC Error");
	}
	else
	{

		printinfo("EEprom Config OK");
	}
}

// checksumme der Config
int calcConfigSum()
{
uint8_t *p = (uint8_t *)(&eeconfig);
int sum = 0;

	for(int i=0; i<sizeof(CONFIG); i++)
		sum += p[i];

	return sum;
}

// überwache die Config, wenn geändert, schreibe sie ins eeprom
void checkConfig()
{
static int first = 1;
static int cfgsum;

	if(first)
	{
		cfgsum = calcConfigSum();
		first = 0;
	}
	else
	{
		int s = calcConfigSum();
		if(s != cfgsum)
		{
			configToEEmem();
			cfgsum = s;
		}
	}
}

// liest das gesamte eeprom in den Speicher (einmal nach dem Einschalten)
void copyEeToMem()
{
	for(int page = 0; page < PAGES; page++)
	{
		i2c_readpage64(page*PAGESIZE, ee_mem+page*PAGESIZE);
		page_modified[page] = 0;
	}

	EEmemToConfig();

	// check EEPROM (4 Bytes in Page 511 sind die Magic Nummer)
	if(	ee_mem[511*PAGESIZE+0] != (MAGIC & 0xff) ||
		ee_mem[511*PAGESIZE+1] != ((MAGIC>>8) & 0xff) ||
		ee_mem[511*PAGESIZE+2] != ((MAGIC>>16) & 0xff) ||
		ee_mem[511*PAGESIZE+3] != ((MAGIC>>24) & 0xff))
	{
		// EEprom neu oder Fehler
		// schreibe Magic neu, diese befindet sich in Page 511
		ee_mem[511*PAGESIZE+0] = MAGIC & 0xff;
		ee_mem[511*PAGESIZE+1] = (MAGIC>>8) & 0xff;
		ee_mem[511*PAGESIZE+2] = (MAGIC>>16) & 0xff;
		ee_mem[511*PAGESIZE+3] = (MAGIC>>24) & 0xff;
		page_modified[511] = 1;

		// Lösche Tuningspeicher
		for(int i=10*PAGESIZE; i<127*PAGESIZE; i++)
			ee_mem[i] = 0xff;

		for(int i=10; i<127; i++)
			page_modified[i] = 1;

		// init Config
		eeconfig.refWlow = 3;
		eeconfig.refWhigh = 100;
		eeconfig.ref_low_dBm = 34;
		eeconfig.ref_high_dBm = 50;
		eeconfig.refmVlow_fwd = 1388;
		eeconfig.refmVhigh_fwd = 1683;
		eeconfig.refmVlow_rev = 1355;
		eeconfig.refmVhigh_rev = 1660;
		eeconfig.refU3W = 1300;
		eeconfig.refU100W = 1700;
		eeconfig.refI3W = 1100;
		eeconfig.refI100W = 1600;
		eeconfig.civ_adr = 0x94;
		configToEEmem();
	}
}

// speichert einen Tuningwert ins EEPROM-Memory
void saveTuningValue()
{
	tuningsource = 0;

	if(civ_freq == 0) return;

	TUNINGVAL tv = makeEEdataset();
	uint16_t adr = memadr(civ_freq,antenne);

	ee_mem[adr] = tv.LC1;
	ee_mem[adr+1] = tv.LC2;
	ee_mem[adr+2] = tv.LC3;

	printinfo("save %d: %d C%02X L%02X",adr,civ_freq/1000,store_actC(),store_actL());

	page_modified[adr/PAGESIZE] = 1;
	page_modified[(adr+1)/PAGESIZE] = 1;	// falls es zur nächsten Page weitergeht
	page_modified[(adr+2)/PAGESIZE] = 1;
}

void clearTuningValue(int freq)
{
	uint16_t adr = memadr(freq,antenne);

	ee_mem[adr] = 0xff;
	ee_mem[adr+1] = 0xff;
	ee_mem[adr+2] = 0xff;

	page_modified[adr/PAGESIZE] = 1;
}

void clearFullEEprom()
{
	// EEprom neu oder Fehler
	for(int i=0; i<EESIZE; i++)
		ee_mem[i] = 0xff;

	ee_mem[0] = MAGIC & 0xff;
	ee_mem[1] = (MAGIC>>8) & 0xff;
	ee_mem[2] = (MAGIC>>16) & 0xff;
	ee_mem[3] = (MAGIC>>24) & 0xff;
	for(int i=0; i<PAGES; i++)
		page_modified[i] = 1;
}

// lösche alle Werte des aktuellen Bandes
void clearBandEEprom()
{
	if(civ_freq == 0) return;

	uint32_t startfreq, stopfreq;
	getFreqEdges(civ_freq , &startfreq, &stopfreq);

	// lösche das komplette Bandmemory
	for(uint32_t freq = startfreq; freq < stopfreq; freq+=FSTEPSIZE)
	{
		clearTuningValue(freq);
	}
}

TUNINGVAL eetv;
uint32_t tunedToFreq = 0;	// Frequenz für die ein Tuning gefunden wurde

TUNINGVAL *readTuningValue()
{
	if(civ_freq == 0) return NULL;

	uint32_t civ_startfreq = (civ_freq / 10000) * 10000;	// entferne Kommastellen unterhalb 10kHz
	tuningsource = 1;

	// gibt es einen Wert an der aktuellen Position ?
	uint16_t adr = memadr(civ_freq,antenne);
	if(ee_mem[adr+1] != 0xff)
	{
		// Wert vorhanden, nehme diesen
		eetv.LC1 = ee_mem[adr];
		eetv.LC2 = ee_mem[adr+1];
		eetv.LC3 = ee_mem[adr+2];
		tunedToFreq = civ_startfreq;
		return &eetv;
	}

	// kein Wert gefunden, suche innerhalb des Bandes den nächstgelegenen
	int band = getBandnum(civ_freq);
	if(band == -1)
	{
		eetv.LC2 = 0xff;	// nichts gefunden
		tuningsource = 0;
		tunedToFreq = 0;
		return NULL;
	}

	uint32_t start = startedges[band];
	uint32_t stop = stopedges[band];
	uint32_t upper = 0;
	uint32_t lower = 0;
	uint16_t upper_adr = 0;
	uint16_t lower_adr = 0;

	// suche höhere
	for(uint32_t freq = civ_startfreq; freq <= stop; freq += FSTEPSIZE)
	{
		adr = memadr(freq,antenne);
		if(ee_mem[adr+1] != 0xff)
		{
			upper = freq;
			upper_adr = adr;
			break;
		}
	}

	// suche niedrigere
	for(uint32_t freq = civ_startfreq; freq >= start; freq -= FSTEPSIZE)
	{
		adr = memadr(freq,antenne);
		if(ee_mem[adr+1] != 0xff)
		{
			lower = freq;
			lower_adr = adr;
			break;
		}
	}

	if(upper == 0 && lower == 0)
	{
		eetv.LC2 = 0xff;	// nichts gefunden
		tuningsource = 0;
		tunedToFreq = 0;
		return NULL;
	}

	if(upper != 0 && lower != 0)
	{
		// suche besseren von beiden
		if((upper - civ_freq) < (civ_freq - lower))
			lower = 0;
		else
			upper = 0;
	}

	if(upper != 0 && lower == 0)
	{
		eetv.LC1 = ee_mem[upper_adr];
		eetv.LC2 = ee_mem[upper_adr+1];
		eetv.LC3 = ee_mem[upper_adr+2];
		tunedToFreq = upper;
		return &eetv;
	}

	if(upper == 0 && lower != 0)
	{
		eetv.LC1 = ee_mem[lower_adr];
		eetv.LC2 = ee_mem[lower_adr+1];
		eetv.LC3 = ee_mem[lower_adr+2];
		tunedToFreq = lower;
		return &eetv;
	}

	eetv.LC2 = 0xff;	// nichts gefunden
	tuningsource = 0;
	tunedToFreq = 0;
	return NULL;
}
