#ifndef BUDIK_INTERFACES_H_20101225
#define BUDIK_INTERFACES_H_20101225

#include "I2CLiquidCrystal.h"
#include "config.h"
#include "utils.h"
#include "FiniteStateMachine.h"
#include "Streaming.h"

void writeTime(LiquidCrystal &out, int address, uint8_t col, uint8_t row, uint8_t mode);
void writeTemp(LiquidCrystal &out, uint8_t col, uint8_t row);

extern const char* downame[];
extern const char* monthname[];

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
        lcd.enableCursor(false, false);
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

    typedef const struct{
        uint8_t row:8;
        uint8_t column:4;
    } UIPosition;

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

typedef struct _TimeValue {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t dow;
    uint8_t day;
    uint8_t month;
    uint8_t year;
    uint8_t century;
} TimeValue;

class BudikTimeInterface: public BudikInterface
{
 public:
 BudikTimeInterface(LiquidCrystal &out):
    BudikInterface(out)
    {}

    virtual void print(uint8_t col, uint8_t row, int mode)
    {
        BudikInterface::print(col, row, mode);

	lcd.setCursor(col, row);
	lcd << downame[time->dow-1] << ((time->day<0x10) ? "0" : "") << _HEX(time->day);

        if(mode==3){
            lcd.setCursor(col+8, row);
            lcd << monthname[decodeBCD(time->month)-1] << " " << _HEX(time->century);
            if(time->year<0x10) lcd << "0";
            lcd << _HEX(time->year);
        }

	
	lcd.setCursor(col+5,row+((mode==2)?0:1));
	if(time->hour<0x10) lcd << " ";
	lcd << _HEX(time->hour) << ":";
	if(time->minute<0x10) lcd << "0";
	lcd << _HEX(time->minute);
    }

    virtual void setup(uint8_t col, uint8_t row)
    {
        BudikInterface::setup(col, row);
    }

    virtual inline void setTime(TimeValue &t)
    {
        time = &t;
    }


 protected:
    TimeValue *time;
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
        lcd.setCursor(col+nibbles[nibble].column, row+nibbles[nibble].row);
        lcd.enableCursor(true, set_mode);
        //lcd << ((set_mode) ? "=": "^");
    }

    virtual void setup(uint8_t col, uint8_t row)
    {
        BudikTimeInterface::setup(col, row);
        clearRow(col, row+2);
        lcd.setCursor(col, row+3);
        lcd << "  Nastavit cas";
    }

    virtual void setMode(bool mode)
    {
        set_mode = mode;
    }

    virtual void setNibble(uint8_t n)
    {
        nibble = n;
    }

 protected:
    bool set_mode;
    uint8_t nibble;
    static UIPosition nibbles[];
};

typedef struct _AlarmValue {
    uint8_t hour:6;
    uint8_t minute:7;
    uint8_t dow:7;
    uint8_t en:1; //MSB> Su Sa Fr Th We Tu Mo EN <LSB
    uint8_t flags:3;
} AlarmValue;

class BudikSetAlarmInterface: public BudikTimeInterface
{
 public:
 BudikSetAlarmInterface(LiquidCrystal &out):
    BudikTimeInterface(out), set_mode(false), nibble(0)
    {}

    virtual void print(uint8_t col, uint8_t row, int data)
    {
        BudikTimeInterface::print(col, row, 2);
        clearRow(col, row+2);
        lcd.setCursor(col+nibbles[nibble].column, row+nibbles[nibble].row);
        lcd.enableCursor(true, set_mode);
        //lcd << ((set_mode) ? "=": "^");
    }

    virtual void setup(uint8_t col, uint8_t row)
    {
        BudikTimeInterface::setup(col, row);
        clearRow(col, row+2);
        lcd.setCursor(col, row+3);
    }

    virtual void setMode(bool mode)
    {
        set_mode = mode;
    }

    virtual void setNibble(uint8_t n)
    {
        nibble = n;
    }

    virtual void setAlarm(AlarmValue &a)
    {
        alarm = &a;
    }

 protected:
    AlarmValue *alarm;
    bool set_mode;
    uint8_t nibble;
    static UIPosition nibbles[];
};


class BudikTickerInterface: public BudikTimeInterface
{
 public:
 BudikTickerInterface(LiquidCrystal &out):
    BudikTimeInterface(out)
    {
        line1[0] = 0;
        line2[0] = 0;
    }

    virtual void print(uint8_t col, uint8_t row, int data)
    {
        BudikTimeInterface::print(col, row, 3);
        clearRow(col, row+2);
        lcd.setCursor(col, row+2);
        lcd << line1;

        clearRow(col, row+3);
        lcd.setCursor(col, row+3);
        lcd << line2;

    }

    virtual void setup(uint8_t col, uint8_t row)
    {
        BudikTimeInterface::setup(col, row);
        clearRow(col, row+2);
        clearRow(col, row+3);
    }

    virtual void setLine1(String &s)
    {
        s.toCharArray(line1, 17);
        line1[16] = 0;
    }

    virtual void setLine2(String &s)
    {
        s.toCharArray(line2, 17);
        line2[16] = 0;
    }

 protected:
    char line1[17];
    char line2[17];
};

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

    virtual void setTitle(const char *title0)
    {
        title = title0;
    }

    virtual void setItem(const char *item0)
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

    virtual void setBacklight(uint8_t backlightlevel)
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
