#include "i2c.h"
#include <stdint.h>
#include <stdio.h>
#include <tick1ms.h>
#include <stdarg.h>
#include <string.h>

#define LCD_DB7	0x80
#define LCD_DB6	0x40
#define LCD_DB5	0x20
#define LCD_DB4	0x10
#define LCD_BL	0x08
#define LCD_E	0x04
#define LCD_RW	0x02
#define LCD_RS	0x01

#define LCD_CMD_CLEAR			0x01
#define LCD_CMD_ENTRY_MODE		0x06
#define LCD_CMD_DISPLAY_CONTROL		0x0c
#define LCD_CMD_SET_DDRAM_ADDRESS(x)	(0x80 | x)

static void lcd_add_data_nibble(uint8_t **buffer, uint8_t byte)
{
	uint8_t *p = *buffer;

	*p++ = LCD_BL | LCD_RS | LCD_E | byte << 4;
	*p++ = LCD_BL | LCD_RS | byte << 4;
	*p++ = LCD_BL | LCD_RS | LCD_E | byte << 4;

	*buffer = p;
}

static void lcd_add_cmd_nibble(uint8_t **buffer, uint8_t byte)
{
	uint8_t *p = *buffer;

	*p++ = LCD_BL | LCD_E | byte << 4;
	*p++ = LCD_BL | byte << 4;
	*p++ = LCD_BL | LCD_E | byte << 4;
	*buffer = p;
}

static void lcd_add_data_byte(uint8_t **buffer, uint8_t byte)
{
	lcd_add_data_nibble(buffer, byte >> 4);
	lcd_add_data_nibble(buffer, byte & 0xf);
}

static void lcd_add_cmd_byte(uint8_t **buffer, uint8_t byte)
{
	lcd_add_cmd_nibble(buffer, byte >> 4);
	lcd_add_cmd_nibble(buffer, byte & 0xf);
}

static void lcd_send_cmd8(uint8_t cmd, int delay)
{
	uint8_t *p, lcd_init[16];

	p = lcd_init;
	lcd_add_cmd_byte(&p, cmd);
	i2c_write(0x4e, lcd_init, p - lcd_init);
	if (delay)
		delay_1ms(delay);
}

static void lcd_send_cmd4(uint8_t cmd, int delay)
{
	uint8_t *p, lcd_init[4];

	p = lcd_init;
	lcd_add_cmd_nibble(&p, cmd);
	i2c_write(0x4e, lcd_init, p - lcd_init);
	if (delay)
		delay_1ms(delay);
}

void lcd_printf(int y, int x, char *fmt, ...)
{
	uint8_t lcd_data[30 * 4], *p;
	char tmp[21];
	va_list args;
	int len;

	memset(tmp, 0, sizeof(tmp));
	va_start(args, fmt);
	len = vsnprintf(tmp, sizeof(tmp), fmt, args);
	va_end(args);

	if (len > 20)
		len = 20;
	if (y & 1)
		x += 0x40;
	if (y & 2)
		x += 0x14;

	lcd_send_cmd8(LCD_CMD_SET_DDRAM_ADDRESS(x), 0);
	p = lcd_data;
	for (int i = 0; i < len; i++)
		lcd_add_data_byte(&p, tmp[i]);
	i2c_write(0x4e, lcd_data, p - lcd_data);
}

void lcd_init(void)
{
	lcd_send_cmd4(0x03, 5);
	lcd_send_cmd4(0x03, 5);
	lcd_send_cmd4(0x03, 5);
	lcd_send_cmd4(0x02, 5);
	lcd_send_cmd8(0x28, 1);
	lcd_send_cmd8(0x08, 1);
	lcd_send_cmd8(LCD_CMD_CLEAR, 100);
	lcd_send_cmd8(LCD_CMD_ENTRY_MODE, 100);
	lcd_send_cmd8(LCD_CMD_DISPLAY_CONTROL, 100);

	lcd_printf(0, 0, "Initializing...");
}
