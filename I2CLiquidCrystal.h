#ifndef I2CLiquidCrystal_h
#define I2CLiquidCrystal_h

#include <inttypes.h>
#include <WProgram.h>
#include <Print.h>


class LiquidCrystal : public Print {
public:
  LiquidCrystal(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);
  LiquidCrystal(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t,
    uint8_t, uint8_t, uint8_t, uint8_t);
  void clear();
  void begin();
  void init();
  void home();
  void setCursor(int, int); 
  /*
  void shiftDisplayLeft();
  void shiftDisplayRight();
  */
  virtual void write(uint8_t);
  virtual void digitalWrite(byte bitno, uint8_t value) { ::digitalWrite(bitno, value); }
  virtual void pinMode(byte bitno, uint8_t value) { ::pinMode(bitno, value); }
  virtual void flush(void) {}
  virtual void flushMode(void) {}
  void command(uint8_t);

private:
  void send(uint8_t, uint8_t);
  
  uint8_t _four_bit_mode;
  uint8_t _rs_pin; // LOW: command.  HIGH: character.
  uint8_t _rw_pin; // LOW: write to LCD.  HIGH: read from LCD.
  uint8_t _enable_pin; // activated by a HIGH pulse.
  uint8_t _data_pins[8];
};

class I2CLiquidCrystal: public LiquidCrystal
{
  public:
    I2CLiquidCrystal(byte i2caddr, byte command, uint8_t rs, uint8_t rw, uint8_t enable,
                     uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3):
    LiquidCrystal(rs, rw, enable, d0, d1, d2, d3),
        i2caddr(i2caddr), command(command), bitsToSet(0), bitsToReset(0), modeOutput(0), modeInput(0)
        { }

  protected:
    int i2caddr;
    byte command;
    uint8_t bitsToSet;
    uint8_t bitsToReset;
    uint8_t modeOutput;
    uint8_t modeInput;
    
    void digitalWrite(byte bitno, uint8_t value);
    void pinMode(byte bitno, uint8_t value);
    void flush(void);
    void flushMode(void);
};

#endif
