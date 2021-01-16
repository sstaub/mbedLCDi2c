/* mbed TextLCD Library, for a 4-bit LCD based on HD44780
 * Copyright (c) 2007-2010, sford, http://mbed.org
 * Copyright (c) 2020, sstaub
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "LCDi2c.h"
#include "mbed.h"

LCDi2c::LCDi2c(lcd_t type, int deviceAddress) : i2c(I2C_SDA, I2C_SCL) {
	this->deviceAddress = deviceAddress << 1; // convert 7bit address to mbed 8bit address
	this->type = type;
	init();
	}

LCDi2c::LCDi2c(PinName sda, PinName scl, lcd_t type, int deviceAddress) : i2c(sda, scl) {
	this->deviceAddress = deviceAddress << 1; // convert 7bit address to mbed 8bit address
	this->type = type;
	init();
	}

void LCDi2c::init() {
	displayfunction = LCD_4BIT_MODE | LCD_2_LINE | LCD_5x8DOTS;
  write4bits(0x03 << 4);
  wait_us(4500);
  write4bits(0x03 << 4);
  wait_us(4500);
  write4bits(0x03 << 4); 
  wait_us(150);
  write4bits(0x02 << 4); 
	writeCommand(LCD_FUNCTION_SET | displayfunction);  
	displaycontrol = LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_BLINK_OFF;
	display(DISPLAY_ON);
	display(CURSOR_OFF);
	display(BLINK_OFF);
	cls();
	displaymode = LCD_ENTRY_LEFT | LCD_ENTRY_SHIFT_DECREMENT;
	writeCommand(LCD_ENTRY_MODE_SET | displaymode);
	home();
	display(BACKLIGHT_ON);
	}

void LCDi2c::cls() {
	writeCommand(0x01); // cls, and set cursor to 0
	ThisThread::sleep_for(2ms); // this command takes 1.64 ms
	locate(0, 0);
	}

void LCDi2c::locate(uint8_t column, uint8_t row) {
	this->column = column;
	this->row = row;
	}

void LCDi2c::home() {
	writeCommand(LCD_RETURN_HOME);
	ThisThread::sleep_for(2ms); // this command takes a long time!
	}

void LCDi2c::display(modes_t mode) {
	switch(mode) {
		case DISPLAY_ON :
			displaycontrol |= LCD_DISPLAY_ON;
			writeCommand(LCD_DISPLAY_CONTROL | displaycontrol);
			break;
		case DISPLAY_OFF:
			displaycontrol &= ~LCD_DISPLAY_ON;
			writeCommand(LCD_DISPLAY_CONTROL | displaycontrol);
			break;
		case CURSOR_ON:
			displaycontrol |= LCD_CURSOR_ON;
			writeCommand(LCD_DISPLAY_CONTROL | displaycontrol);
			break;
		case CURSOR_OFF:
			displaycontrol &= ~LCD_CURSOR_ON;
			writeCommand(LCD_DISPLAY_CONTROL | displaycontrol);
			break;
		case BLINK_ON:
			displaycontrol |= LCD_BLINK_ON;
			writeCommand(LCD_DISPLAY_CONTROL | displaycontrol);
			break;
		case BLINK_OFF:
			displaycontrol &= ~LCD_BLINK_ON;
			writeCommand(LCD_DISPLAY_CONTROL | displaycontrol);
			break;
		case SCROLL_LEFT:
			writeCommand(LCD_CURSOR_SHIFT | LCD_DISPLAY_MOVE | LCD_MOVE_LEFT);
			break;
		case SCROLL_RIGHT:
			writeCommand(LCD_CURSOR_SHIFT | LCD_DISPLAY_MOVE | LCD_MOVE_RIGHT);
			break;
		case LEFT_TO_RIGHT:
			displaymode |= LCD_ENTRY_LEFT;
			writeCommand(LCD_ENTRY_MODE_SET | displaymode);
			break;
		case RIGHT_TO_LEFT:
			displaymode &= ~LCD_ENTRY_LEFT;
			writeCommand(LCD_ENTRY_MODE_SET | displaymode);
			break;
		case SCROLL_ON:
			displaymode |= LCD_ENTRY_SHIFT_INCREMENT;
			writeCommand(LCD_ENTRY_MODE_SET | displaymode);
			break;
		case SCROLL_OFF:
			displaymode &= ~LCD_ENTRY_SHIFT_INCREMENT;
			writeCommand(LCD_ENTRY_MODE_SET | displaymode);
			break;
		case BACKLIGHT_ON:
			backlight = LCD_BACKLIGHT_ON;
			sendI2C(0);
			break;
		case BACKLIGHT_OFF:
			backlight = LCD_BACKLIGHT_OFF;
			sendI2C(0);
			break;
		}
	}

void LCDi2c::character(uint8_t column, uint8_t row, uint8_t c) {
	int a = address(column, row);
	writeCommand(a);
	writeData(c);
	}

void LCDi2c::create(uint8_t location, uint8_t charmap[]) {
	if (location > 7) return;
	location &= 0x7;
	writeCommand(LCD_SET_CGRAM_ADDR | (location << 3));
	for (int i = 0; i < 8; i++) {
		writeData(charmap[i]);
		}
	}

int LCDi2c::_putc(int value) {
	if (value == '\n') {
		column = 0;
		row++;
		if (row >= rows()) {
			row = 0;
			}
		}
	else {
		character(column, row, value);
		column++;
		if (column >= columns()) {
			column = 0;
			row++;
			if (row >= rows()) {
				row = 0;
				}
			}
		}
	return value;
	}

int LCDi2c::_getc() {
	return -1;
	}

uint8_t LCDi2c::address(uint8_t column, uint8_t row) {
	switch (type) {
		case LCD20x4:
			switch (row) {
				case 0:
					return 0x80 + column;
				case 1:
					return 0xc0 + column;
				case 2:
					return 0x94 + column;
				case 3:
					return 0xd4 + column;
				default:
					return -1;
				}
		case LCD16x2:
		case LCD20x2:
		case LCD40x2:
		default:
			return 0x80 + (row * 0x40) + column;
		}
	}

uint8_t LCDi2c::columns() {
	switch (type) {
		case LCD20x4:
		case LCD20x2:
		case LCD40x2:
			return 20;
		case LCD16x2:
		default:
			return 16;
		}
	}

uint8_t LCDi2c::rows() {
	switch (type) {
		case LCD20x4:
			return 4;
		case LCD16x2:
		case LCD20x2:
		case LCD40x2:
		default:
			return 2;
		}
	}

void LCDi2c::write4bits(uint8_t value) {
	sendI2C(value);
	pulseEnable(value);
	}

void LCDi2c::pulseEnable(uint8_t value) {
	sendI2C(value & ~I2C_EN);	// En low
	wait_us(1);
	sendI2C(value | I2C_EN);	// En high
	wait_us(1);
	sendI2C(value & ~I2C_EN);	// En low
	wait_us(40);
	}

void LCDi2c::send(uint8_t value, uint8_t mode) {
	uint8_t highnib = value & 0xf0;
	uint8_t lownib = (value << 4) & 0xf0;
	write4bits(highnib | mode);
	write4bits(lownib | mode);
	}

void LCDi2c::writeCommand(uint8_t command) {
	send(command, 0);
	}

void LCDi2c::writeData(uint8_t data) {
	send(data, I2C_RS);
	}

void LCDi2c::sendI2C(uint8_t value) {
	char data[2];
	data[0] = value | backlight;
	i2c.write(deviceAddress, data, 1);
	}

void parseI2C() {
	I2C i2c(I2C_SDA, I2C_SCL);
	for (uint8_t i = 0; i < 255; i++) {
		if (i2c.write(i, 0, 1) == 0) {
			printf("I2C Address 8bit: %X\n", i);
			printf("I2C Address 7bit: %X\n", i >> 1);
			return;
			}
		}
	}