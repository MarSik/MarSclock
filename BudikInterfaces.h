#ifndef BUDIK_INTERFACES_H_20101225
#define BUDIK_INTERFACES_H_20101225

#include "I2CLiquidCrystal.h"
#include "config.h"

void writeTime(LiquidCrystal &out, int address, uint8_t col, uint8_t row, uint8_t mode);
void writeTemp(LiquidCrystal &out, uint8_t col, uint8_t row);


class BudikInterface
{
 public:
 BudikInterface(LiquidCrystal &out):
    lcd(out)
    {}

    virtual void print(uint8_t col, uint8_t row, int data)
    {
    }

    virtual void print(uint8_t col, uint8_t row,
                       uint8_t width, uint8_t height, int data)
    {
        print(col, row, data);
    }

    virtual void setup(uint8_t col, uint8_t row)
    {
        lcd.clear();
        queue.enqueueEvent(EV_REFRESH, 1);
    }

    virtual void cleanup()
    {
    }

    virtual void clearRow(uint8_t col, uint8_t row)
    {
        int i;
        lcd.setCursor(col, row);
        for(i=col; i<16; i++)
            lcd << " ";
    }

 protected:
    LiquidCrystal &lcd;
};

class BudikTempInterface: public BudikInterface
{
 public:
 BudikTempInterface(LiquidCrystal &out):
    BudikInterface(out)
    {}

    virtual void print(uint8_t col, uint8_t row, int data)
    {
        BudikInterface::print(col, row, data);
        writeTemp(lcd, col, row);
    }

    virtual void setup(uint8_t col, uint8_t row)
    {
        //BudikInterface::setup(col, row);
    }
};

class BudikTimeInterface: public BudikInterface
{
 public:
 BudikTimeInterface(LiquidCrystal &out):
    BudikInterface(out)
    {}

    virtual void print(uint8_t col, uint8_t row, int data)
    {
        BudikInterface::print(col, row, data);
        writeTime(lcd, I2CDATA, col, row, data);
    }

    virtual void setup(uint8_t col, uint8_t row)
    {
        BudikInterface::setup(col, row);
    }

};

class BudikSetTimeInterface: public BudikTimeInterface
{
 public:
 BudikSetTimeInterface(LiquidCrystal &out):
    BudikTimeInterface(out), set_mode(false), nibble(0)
    {}

    virtual void print(uint8_t col, uint8_t row, int data)
    {
        BudikTimeInterface::print(col, row, 3);
        clearRow(col, row+2);
        lcd.setCursor(col+nibbles[nibble], row+2);
        lcd << ((set_mode) ? "=": "^");
    }

    virtual void setup(uint8_t col, uint8_t row)
    {
        BudikTimeInterface::setup(col, row);
        clearRow(col, row+2);
        lcd.setCursor(col, row+3);
        lcd << "  Nastavit cas";
    }

    void setMode(bool mode)
    {
        set_mode = mode;
    }

    void setNibble(uint8_t n)
    {
        nibble = n;
    }

 protected:
    bool set_mode;
    uint8_t nibble;
    static uint8_t nibbles[];
};

uint8_t BudikSetTimeInterface::nibbles[] = {0, 2, 5, 8, 10, 12, 14};

class BudikBasicInterface: public BudikTimeInterface, BudikTempInterface
{
 public:
 BudikBasicInterface(LiquidCrystal &out):
    BudikTimeInterface(out),
        BudikTempInterface(out)
        {}

    virtual void print(uint8_t col, uint8_t row, int data)
    {
        BudikTimeInterface::print(col, row, data);
        BudikTempInterface::print(col+11, row, data);
    }

    virtual void setup(uint8_t col, uint8_t row)
    {
        BudikTimeInterface::setup(col, row);
        BudikTempInterface::setup(col+11, row);
    }
};

class BudikMenuInterface: public BudikTimeInterface
{
 public:
 BudikMenuInterface(LiquidCrystal &out):
    BudikTimeInterface(out), title(""), item("")
    {}

    void setTitle(const char *title0)
    {
        title = title0;
    }

    void setItem(const char *item0)
    {
        item = item0;
    }

    virtual void print(uint8_t col, uint8_t row, int data)
    {
        BudikTimeInterface::print(col, row, data);
        clearRow(strlen(item)+3+col, row+3);
        lcd.setCursor(col, row+3);
        lcd << "<<";
        lcd.setCursor(col+16-2, row+3);
        lcd << ">>";
        lcd.setCursor(col+3, row+3);
        lcd << item;
    }

    virtual void setup(uint8_t col, uint8_t row)
    {
        BudikTimeInterface::setup(col, row);
        lcd.setCursor(col, row+2);
        if(*title) lcd << "- " << title << " -";
    }

 protected:
    const char* title;
    const char* item;
};

class BudikBacklightInterface: public BudikTimeInterface
{
 public:
 BudikBacklightInterface(LiquidCrystal &out):
    BudikTimeInterface(out), backlight(0)
    {}

    void setBacklight(uint8_t backlightlevel)
    {
        backlight = backlightlevel;
    }

    virtual void print(uint8_t col, uint8_t row, int data)
    {
        int i;

        BudikTimeInterface::print(col, row, data);
        lcd.setCursor(col, row+3);
        lcd << " <<";
        for(i=0; i<9; i++){
            if(i==backlight) lcd << "|";
            else lcd << " ";
        }
        lcd << ">>";
    }

    virtual void setup(uint8_t col, uint8_t row)
    {
        BudikTimeInterface::setup(col, row);
        lcd.setCursor(col, row+2);
        lcd << " - Podsviceni -";
    }


 protected:
    uint8_t backlight;
};

#endif
