#ifndef BUDIK_CONFIG_H_20101225
#define BUDIK_CONFIG_H_20101225

// RTC direct ports
#define WE 8
#define OE 7
#define CE 4

// LCD backlight PWM controlled
#define LCDLIGHT 11

// LM335 sensor
#define TEMPERATURE A2

// I2C I/O MUXes (MCP23016)
#define I2CADDR 0x20
#define I2CDATA 0x21

// DATA MUX PORTS 
#define I2CLCD 0x0
#define I2CRTC 0x1

// I2C RUNTIME CONFIG
#define I2CREADDELAYUS 200
#define I2CWRITEDELAYUS 12

// INTERNAL LED
#define STATUSLED 13

#endif
