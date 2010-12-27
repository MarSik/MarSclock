#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <WProgram.h>
#include "Wire.h"
#include "I2CLiquidCrystal.h"

// When the display powers up, it is configured as follows:
//
// 1. Display clear
// 2. Function set: 
//    DL = 1; 8-bit interface data 
//    N = 0; 1-line display 
//    F = 0; 5x8 dot character font 
// 3. Display on/off control: 
//    D = 0; Display off 
//    C = 0; Cursor off 
//    B = 0; Blinking off 
// 4. Entry mode set: 
//    I/D = 1; Increment by 1 
//    S = 0; No shift 
//
// Note, however, that resetting the Arduino doesn't reset the LCD, so we
// can't assume that its in that state when a sketch starts (and the
// LiquidCrystal constructor is called).

inline void I2CLiquidCrystal::digitalWrite(byte bitno, uint8_t value)
{
    if(value==LOW) bitsToReset |= 1 << bitno;
    else bitsToSet |= 1 << bitno;
}

void I2CLiquidCrystal::flush()
{
  byte pre = 0x00;
  
  // query
  Wire.beginTransmission(i2caddr);
  Wire.send(0x00+command);
  Wire.endTransmission();
  
  // reply
  Wire.requestFrom(i2caddr, 1);
  if(Wire.available())
    pre = Wire.receive();
  
  pre &= ~bitsToReset;
  pre |= bitsToSet;

  bitsToReset = bitsToSet = 0;
  
  // Switch to mode true -> in / false -> out
  Wire.beginTransmission(i2caddr);
  Wire.send(0x00 + command);
  Wire.send(pre);
  Wire.endTransmission();
}

inline void I2CLiquidCrystal::pinMode(byte bitno, uint8_t value)
{
    if(value==OUTPUT) modeOutput |= 1 << bitno;
    else modeInput |= 1 << bitno;
}

void I2CLiquidCrystal::flushMode()
{
  byte pre = 0x00;
  
  // query
  Wire.beginTransmission(i2caddr);
  Wire.send(0x06+command);
  Wire.endTransmission();
  
  // reply
  Wire.requestFrom(i2caddr, 1);
  pre = Wire.receive();
  
  pre &= ~(modeOutput);
  pre |= modeInput;

  modeOutput = modeInput = 0;

  // Switch to mode true -> in / false -> out
  Wire.beginTransmission(i2caddr);
  Wire.send(0x06 + command);
  Wire.send(pre);
  Wire.endTransmission();
}


LiquidCrystal::LiquidCrystal(uint8_t rs, uint8_t rw, uint8_t enable,
  uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3,
  uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7) :
  _four_bit_mode(0), _rs_pin(rs), _rw_pin(rw), _enable_pin(enable)
{
  _data_pins[0] = d0;
  _data_pins[1] = d1;
  _data_pins[2] = d2;
  _data_pins[3] = d3; 
  _data_pins[4] = d4;
  _data_pins[5] = d5;
  _data_pins[6] = d6;
  _data_pins[7] = d7;
}

void LiquidCrystal::begin()
{
  init();
    
  pinMode(_rs_pin, OUTPUT);
  pinMode(_rw_pin, OUTPUT);
  pinMode(_enable_pin, OUTPUT);

  int bits = 8;
  if(_four_bit_mode) bits = 4;

  for (int i = 0; i < bits; i++)
    pinMode(_data_pins[i], OUTPUT);
  flushMode();
 
  if(!_four_bit_mode) command(0x38);  // function set: 8 bits, 1 line, 5x8 dots
  else command(0x28);  // function set: 4 bits, 2 lines, 5x8 dots
  command(0x0C);  // display control: turn display on, cursor off, no blinking
  clear();
  command(0x06);  // entry mode set: increment automatically, display shift, right shift
}

LiquidCrystal::LiquidCrystal(uint8_t rs, uint8_t rw, uint8_t enable,
  uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3) :
  _four_bit_mode(1), _rs_pin(rs), _rw_pin(rw), _enable_pin(enable)
{
  _data_pins[0] = d0;
  _data_pins[1] = d1;
  _data_pins[2] = d2;
  _data_pins[3] = d3; 
}

void LiquidCrystal::clear()
{
  command(0x01);  // clear display, set cursor position to zero
  delayMicroseconds(2000);
}

void LiquidCrystal::home()
{
  command(0x02);  // set cursor position to zero
  delayMicroseconds(2000);
}

void LiquidCrystal::setCursor(int col, int row)
{
  int row_offsets[] = { 0x00, 0x40, 0x10, 0x50 };
  command(0x80 | (col + row_offsets[row]));
}

void LiquidCrystal::command(uint8_t value) {
  send(value, LOW);
  delayMicroseconds(50);
}

void LiquidCrystal::write(uint8_t value) {
  send(value, HIGH);
  delayMicroseconds(60);
}

void LiquidCrystal::init()
{
    int pinoffset = (_four_bit_mode) ? 0 : 4;

    digitalWrite(_rs_pin, LOW);
    digitalWrite(_rw_pin, LOW);

    // Init by instruction 0x30 1st time, 5ms delay
    digitalWrite(_enable_pin, HIGH);

    for (int i = 0; i < 4; i++) {
      digitalWrite(_data_pins[i+pinoffset], (0x30 >> (i + 4)) & 0x01);
    }
    flush();

    digitalWrite(_enable_pin, LOW);
    flush();

    delay(5);

    // Init by instruction 0x30 2nd time min 100 us delay
    digitalWrite(_enable_pin, HIGH);

    for (int i = 0; i < 4; i++) {
      digitalWrite(_data_pins[i+pinoffset], (0x30 >> (i + 4)) & 0x01);
    }
    flush();

    digitalWrite(_enable_pin, LOW);
    flush();

    delayMicroseconds(100);

    // Init by instruction 0x30 3rd time min 39 us delay

    digitalWrite(_enable_pin, HIGH);

    for (int i = 0; i < 4; i++) {
      digitalWrite(_data_pins[i+pinoffset], (0x30 >> (i + 4)) & 0x01);
    }
    flush();

    digitalWrite(_enable_pin, LOW);
    flush();

    delayMicroseconds(39);

    // Set preliminary 4-bit mode, delay min 39us
    if (_four_bit_mode) {
        digitalWrite(_enable_pin, HIGH);

        for (int i = 0; i < 4; i++) {
            digitalWrite(_data_pins[i], (0x20 >> (i + 4)) & 0x01);
        }
        flush();

        digitalWrite(_enable_pin, LOW);
        flush();

        delayMicroseconds(39);
    }
}

void LiquidCrystal::send(uint8_t value, uint8_t mode) {
  digitalWrite(_rs_pin, mode);
  digitalWrite(_rw_pin, LOW);

  if (_four_bit_mode) {
    digitalWrite(_enable_pin, HIGH);

    for (int i = 0; i < 4; i++) {
      digitalWrite(_data_pins[i], (value >> (i + 4)) & 0x01);
    }
    flush();
    
    digitalWrite(_enable_pin, LOW);
    flush();

    digitalWrite(_enable_pin, HIGH);
 
    for (int i = 0; i < 4; i++) {
      digitalWrite(_data_pins[i], (value >> i) & 0x01);
    }
    flush();

    digitalWrite(_enable_pin, LOW);
    flush();

  } else {
    digitalWrite(_enable_pin, HIGH);

    for (int i = 0; i < 8; i++) {
      digitalWrite(_data_pins[i], (value >> i) & 0x01);
    }
    flush();

    digitalWrite(_enable_pin, LOW);
    flush();
  }

  digitalWrite(_rs_pin, LOW); 
  digitalWrite(_rw_pin, HIGH);
  
  for (int i = 0; i < 4; i++) {
      digitalWrite(_data_pins[i], LOW);
  }
  flush();
}
