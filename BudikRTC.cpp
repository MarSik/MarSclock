#include <WProgram.h>
#include <Wire.h>
#include "config.h"
#include "BudikRTC.h"
#include "utils.h"
#include "Streaming.h"
#include "BudikInterfaces.h"

void setAddr(uint8_t a2, uint8_t a1, uint8_t a0)
{
	static uint8_t b2 = 0x1;
        static uint8_t b1 = 0xFF;
        static uint8_t b0 = 0x0;
        uint8_t lcdp;

        if(b0!=a0){
            writeI2CData(I2CADDR, 0x0, a0);
            b0 = a0;
        }

        if(b1!=a1){
           writeI2CData(I2CADDR, 0x1, a1);
           b1 = a1;
        }

        a2 &= 0x1;

        if(b2!=a2){
            // highest bit is on the lcd port
            Wire.beginTransmission(I2CDATA);
            Wire.send(I2CLCD);
            Wire.endTransmission();
            
            // read old value and changle only one bit
            Wire.requestFrom(I2CDATA, 1);
            delayMicroseconds(I2CWRITEDELAYUS);
            lcdp = Wire.receive() & 0x7F;
            writeI2CData(I2CDATA, I2CLCD, lcdp | ((a2 & 0x1) << 7));

            //
            b2 = a2;
        }
	
}

int readI2CMux()
{
	delayMicroseconds(I2CREADDELAYUS); // standard delay of the I2C chip
	
	// query
	Wire.beginTransmission(I2CDATA);
	Wire.send(I2CRTC);
	Wire.endTransmission();
	
	// reply
	Wire.requestFrom(I2CDATA, 1);
	
	delayMicroseconds(I2CWRITEDELAYUS);
	
	return Wire.receive();
}

// Switch to mode true -> in / false -> out
void dirI2CMux0(boolean input)
{
    digitalWrite(WE, 1);
    digitalWrite(OE, (input) ? 0 : 1);
	
    Wire.beginTransmission(I2CDATA);
    Wire.send(I2CRTC+0x06);
    Wire.send((input)?0xFF:0x00);
    Wire.endTransmission();
}

void writeI2CMux0(byte data)
{
    // Write
    Wire.beginTransmission(I2CDATA);
    Wire.send(I2CRTC);
    Wire.send(data);
    Wire.endTransmission();
}

void writeI2CData(int address, byte block, byte data)
{
    // Write
    Wire.beginTransmission(address);
    Wire.send(block);
    Wire.send(data);
    Wire.endTransmission();
}

#define RTC_ADDR2 0x1
#define RTC_ADDR1 0xFF
#define RTC_SECOND 0xF9
#define RTC_MINUTE 0xFA
#define RTC_HOUR 0xFB
#define RTC_DOW 0xFC
#define RTC_DAY 0xFD
#define RTC_MONTH 0xFE
#define RTC_YEAR 0xFF
#define RTC_CENTURY 0xF8

// mode 0 - normal
// mode 1 - force refresh
// mode 2 - 1 + onelined
// mode 3 - 1 + month, year and century
TimeValue readTime(bool fullmode)
{
    static TimeValue ts;
	
    byte* vars[] = {&(ts.second), &(ts.minute), &(ts.hour), &(ts.dow), &(ts.day), &(ts.month), &(ts.year), &(ts.century)};
    int addrs[] = {RTC_SECOND, RTC_MINUTE, RTC_HOUR, RTC_DOW,
                   RTC_DAY, RTC_MONTH, RTC_YEAR, RTC_CENTURY};
    byte i;
    int val;
    
    dirI2CMux0(true);
    
    for(i=0; i<8; i++){
        setAddr(RTC_ADDR2, RTC_ADDR1, addrs[i]);
        delayMicroseconds(I2CREADDELAYUS);
        val = readI2CMux();
        if(!fullmode && *(vars[i]) == val) break;
        else *(vars[i]) = val;
    }
    
    ts.second = ts.second & 0x7F;
    ts.century = ts.century & 0x3F;
    ts.dow = ts.dow & 0x7;
    
    return ts;
}

#define ALARM_ADDR2 0x1
#define ALARM_ADDR1 0xFE
#define ALARM_ADDR0 0x00
#define ALARM_NEXT_SHIFT 2
#define ALARM_MASK (0xFF >> ALARM_NEXT_SHIFT)
#define ALARM_DOW 0
#define ALARM_HOUR 1
#define ALARM_MINUTE 2
#define ALARM_FLAGS 3

AlarmValue readAlarm(uint8_t alidx)
{
    AlarmValue v;
    uint8_t dow_en;

    dirI2CMux0(true);
    alidx &= ALARM_MASK;
    v.id = alidx;

    // DOW + EN bits
    setAddr(ALARM_ADDR2, ALARM_ADDR1, ALARM_ADDR0 + ALARM_DOW + (alidx << ALARM_NEXT_SHIFT));
    dow_en = readI2CMux();
    v.en = dow_en & 0x1;
    v.dow = dow_en >> 1;
    // Hour
    setAddr(ALARM_ADDR2, ALARM_ADDR1, ALARM_ADDR0 + ALARM_HOUR + (alidx << ALARM_NEXT_SHIFT));
    v.hour = readI2CMux();
    // Minute
    setAddr(ALARM_ADDR2, ALARM_ADDR1, ALARM_ADDR0 + ALARM_MINUTE + (alidx << ALARM_NEXT_SHIFT));
    v.minute = readI2CMux();
    // Flags
    setAddr(ALARM_ADDR2, ALARM_ADDR1, ALARM_ADDR0 + ALARM_FLAGS + (alidx << ALARM_NEXT_SHIFT));
    v.flags = readI2CMux() & 0b111;

    return v;
}

void writeAlarm(uint8_t alidx, AlarmValue &v)
{
    dirI2CMux0(false);	

    // DOW + EN bits
    setAddr(ALARM_ADDR2, ALARM_ADDR1, ALARM_ADDR0 + ALARM_DOW + (alidx << ALARM_NEXT_SHIFT));
    writeI2CData(I2CDATA, I2CRTC, (v.dow << 1) + v.en);

    digitalWrite(WE, 0);
    delayMicroseconds(I2CWRITEDELAYUS);
    digitalWrite(WE, 1);

    // Hour
    setAddr(ALARM_ADDR2, ALARM_ADDR1, ALARM_ADDR0 + ALARM_HOUR + (alidx << ALARM_NEXT_SHIFT));
    writeI2CData(I2CDATA, I2CRTC, v.hour);

    digitalWrite(WE, 0);
    delayMicroseconds(I2CWRITEDELAYUS);
    digitalWrite(WE, 1);

    // Minute
    setAddr(ALARM_ADDR2, ALARM_ADDR1, ALARM_ADDR0 + ALARM_MINUTE + (alidx << ALARM_NEXT_SHIFT));
    writeI2CData(I2CDATA, I2CRTC, v.minute);

    digitalWrite(WE, 0);
    delayMicroseconds(I2CWRITEDELAYUS);
    digitalWrite(WE, 1);

    // Flags
    setAddr(ALARM_ADDR2, ALARM_ADDR1, ALARM_ADDR0 + ALARM_FLAGS + (alidx << ALARM_NEXT_SHIFT));
    writeI2CData(I2CDATA, I2CRTC, v.flags);

    digitalWrite(WE, 0);
    delayMicroseconds(I2CWRITEDELAYUS);
    digitalWrite(WE, 1);
}
