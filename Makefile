all: tune.elf

INCS=-Iinc -ICMSIS/device/ -ICMSIS/core -IStdPeriph_Driver/inc -include StdPeriph_Driver/inc/stm32f4xx_conf.h
CFLAGS=-O2 -Wall -Wextra -DSTM32F427_437xx=1 -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -u _printf_float -MMD -MP
LDFLAGS=-Wl,-T"LinkerScript.ld" -Wl,-Map="output.map" -Wl,--gc-sections -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -u _printf_float
CROSS_COMPILE=arm-none-eabi-
CC=$(CROSS_COMPILE)gcc
AS=$(CROSS_COMPILE)as
OBJCOPY=$(CROSS_COMPILE)objcopy

ifeq ($(V),1)
	QUIET=
	NOTQUIET=@\#
else
	QUIET=@
	NOTQUIET=
endif
SRCS=src/adc.c					\
	src/bandcalc.c				\
	src/crc16.c				\
	src/eeprom.c				\
	src/main.c				\
	src/ntc.c				\
	src/remote.c				\
	src/serial_tx_proc.c			\
	src/setC.c				\
	src/setL.c				\
	src/swr.c				\
	src/syscalls.c				\
	src/system_stm32f4xx.c			\
	src/t2_led.c				\
	src/tick1ms.c				\
	src/tuner.c				\
	src/tune_storage.c			\
	src/uart.c				\
	src/t2_relais.c				\
	src/uart_civ.c				\
	StdPeriph_Driver/src/stm32f4xx_adc.c	\
	StdPeriph_Driver/src/stm32f4xx_dma.c	\
	StdPeriph_Driver/src/stm32f4xx_gpio.c	\
	StdPeriph_Driver/src/stm32f4xx_i2c.c	\
	StdPeriph_Driver/src/stm32f4xx_tim.c	\
	StdPeriph_Driver/src/stm32f4xx_rcc.c	\
	StdPeriph_Driver/src/misc.c		\
	StdPeriph_Driver/src/stm32f4xx_usart.c

OBJS=$(SRCS:.c=.o)
DEPS=$(SRCS:.c=.d)

%.o:	%.c
	$(NOTQUIET)@echo "CC	$<"
	$(QUIET)$(CC) $(INCS) $(CFLAGS) -c -o $@ $<

%.o:	%.s
	$(NOTQUIET)@echo "AS	$<"
	$(QUIET)$(CC) $(INCS) $(CFLAGS) -c -o $@ $<

tune.elf: $(OBJS) startup/startup_stm32.o
	$(NOTQUIET)@echo "LD	$@"
	$(QUIET)$(CC) $(LDFLAGS) -o $@ $^ -lm

tune.bin: tune.elf
	$(NOTQUIET)@echo "BIN	$@"
	$(QUIET)$(OBJCOPY) -O binary $< $@

clean:
	$(NOTQUIET)@echo "CLEAN"
	$(QUIET)rm -rf $(OBJS) $(DEPS) tune.elf

-include $(DEPS)
