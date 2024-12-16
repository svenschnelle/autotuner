/*
 * adc.c
 *
 *  Created on: 08.04.2018
 *      Author: kurt
 */

#include <main.h>

/*
 * Analoge Messungen
 *
 * Analoge Eingänge:
 * 0: Phase ....... PA0 ... ADC123_IN0
 * 1: UMESS ....... PA4 ... ADC12_IN4
 * 2: UFWDANT ..... PA5 ... ADC12_IN5
 * 3: UREVANT ..... PA6 ... ADC12_IN6
 * 4: Amplit ...... PA7 ... ADC12_IN7
 * 5: Temp ........ PB0 ... ADC12_IN8
 *
 * für alle Eingänge wird ADC1 benutzt
 *
 * Phi-Eingang ist digital: PC15
 */

#define ADCMITTELANZ	16
#define ADCCHANNELS	6

enum ADCCHANNEL {
	UANT=0,
	UMESS,
	UFWDANT,
	UREVANT,
	IANT,
	TEMPERATURE
};

static volatile uint16_t ADC1ConvertedValue[ADCMITTELANZ * ADCCHANNELS];
static const unsigned char st = ADC_SampleTime_480Cycles;

int revvoltage, fwdvoltage;
int realZ;
int z;		// Z  : 0= kleiner 50, 100=größer 50, 50=ungefähr 50 (in einem Fenster von..bis)
int phi;	// phi: -1 oder +1
int meas_u, meas_i;
int ant_U,ant_I;	// 10x Antennen U und I
int itemp;
int meas_UB;

// Initialisiere ADC1
void t2_ADC_Init()
{
	ADC_InitTypeDef       ADC_InitStructure;
	ADC_CommonInitTypeDef ADC_CommonInitStructure;
	DMA_InitTypeDef       DMA_InitStructure;
	GPIO_InitTypeDef      GPIO_InitStructure;

	// Enable ADC1, DMA2 and GPIO clocks
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2 | RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	// DMA2 Stream4 channel0 configuration (auf Stream 0 startet er sich nicht selbst, Ursache unbekannt)
	DMA_InitStructure.DMA_Channel = DMA_Channel_0;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(ADC1_BASE + ADC_DR_OFFSET);
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)ADC1ConvertedValue;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
	DMA_InitStructure.DMA_BufferSize = ADCMITTELANZ * ADCCHANNELS;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_Init(DMA2_Stream4, &DMA_InitStructure);
	DMA_Cmd(DMA2_Stream4, ENABLE);

	NVIC_InitTypeDef   NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	DMA_ITConfig(DMA2_Stream4, DMA_IT_TC, ENABLE);

	// Configure ADC1 as analog input
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	// ADC Common Init
	ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2;
	ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled; //nur für Multi-ADC mode
	ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
	ADC_CommonInit(&ADC_CommonInitStructure);

	// ADC1 Init
	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfConversion = ADCCHANNELS;
	ADC_Init(ADC1, &ADC_InitStructure);

	// ADC1 regular channel0 configuration
	ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, st);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_4, 2, st);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_5, 3, st);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 4, st);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_7, 5, st);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_8, 6, st);

	// Enable ADC1
	ADC_Cmd(ADC1, ENABLE);

	// Enable ADC1 DMA
	ADC_DMACmd(ADC1, ENABLE);

	// Enable DMA request after last transfer (Single-ADC mode)
	ADC_DMARequestAfterLastTransferCmd(ADC1, ENABLE);

	ADC_SoftwareStartConv(ADC1);   // A/D convert start

	// und PC15 auf Eingang schalten
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	GPIO_InitStructure.GPIO_Pin=0x8000; // Port 15
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
}

// prüfen ob eine Messung fertig ist
static volatile int adccnv_ready = 0;

void DMA2_Stream4_IRQHandler()
{
	if(DMA_GetITStatus(DMA2_Stream4,DMA_IT_TCIF4) == SET)
	{
		adccnv_ready = 1;
		DMA_ClearITPendingBit(DMA2_Stream4,DMA_IT_TCIF4);
	}
}

static int isAdcConvReady(void)
{
	if(adccnv_ready == 1) {
		adccnv_ready = 0;
		return 1;
	}
	return 0;
}

// es werden per DMA genau ADCMITTELANZ Messungen pro ADC gemacht
// und im Array ADC1ConvertedValue abgelegt
// dann wird hier aus diesen Messungen der Mittelwert gebildet
static int ui16_Read_ADC1_ConvertedValue(int channel)
{
	unsigned long v = 0;

	for(unsigned long i = 0; i < ADCMITTELANZ; i++)
		v += ADC1ConvertedValue[i * ADCCHANNELS + channel];

	v /= ADCMITTELANZ;

	return (int)((v*2500)/4096);      // Read and return conversion result
}

int scan_analog_inputs(void)
{
	while(isAdcConvReady() == 0);

	calc_swr(fwdvoltage = ui16_Read_ADC1_ConvertedValue(UFWDANT),revvoltage = ui16_Read_ADC1_ConvertedValue(UREVANT));

	meas_u = ui16_Read_ADC1_ConvertedValue(UANT);
	meas_i = ui16_Read_ADC1_ConvertedValue(IANT);
	itemp = cf_calc_temp(ui16_Read_ADC1_ConvertedValue(TEMPERATURE));

	// Rv=100k Rp=10k
	meas_UB = ui16_Read_ADC1_ConvertedValue(UMESS);
	meas_UB = (110*meas_UB)/10;

	// Antennenspannung und Strom an 50 Ohm
	// refU3W in mV = gemessene Spannung bei 3 Watt, das sind 12,2Veff
	// refU100W in mV = gemessene Spannung bei 100 Watt, das sind 70,7Veff
	// refI3W in mV = gemessener Strom bei 3 Watt, das sind 0,225A
	// refI100W in mV = gemessener Strom bei 100 Watt, das sind 1,41A
	// antU x 10 und antI x 100
	if(meas_u < 5) {
		meas_u = 0;
		meas_i = 0;
		fswr = 0;
	}

	float fu = (70.7-12.2)*(meas_u-eeconfig.refU3W)/(eeconfig.refU100W-eeconfig.refU3W) + 12.2;
	float fi = (1.41-0.225)*(meas_i-eeconfig.refI3W)/(eeconfig.refI100W-eeconfig.refI3W) + 0.225;

	// bei RX Nullstellen
	if(fu < 0)
		fu = 0;
	if(fi < 0)
		fi = 0;

	ant_U = (int)(fu * 10);
	ant_I = (int)(fi * 100);

	// bei 50 Ohm sind i und u gleich groß, damit bei R=U/I  50 herauskommt, normiere das U
	// vermeide div durch 0, falls kein Strom
	if(meas_i > 0)
		realZ = (meas_u * 50) / meas_i;
	else
		realZ = 255;

#if 0
	z = realZ;
#else
	if(realZ > 255)
		realZ = 255;
	if(realZ < 40)
		z=0;
	else if (realZ > 60)
		z=100;
	else
		z=50;
#endif
	// frage noch PHI ab
	if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_15))
		phi = -1;
	else
		phi = 1;

	return 1;
}
