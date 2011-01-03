#include <WProgram.h>
#include <Wire.h>
#include "config.h"
#include "BudikRTC.h"
#include "utils.h"
#include "Streaming.h"
#include "BudikInterfaces.h"

void setAddr(int val)
{
	int i;
	
	writeI2CData(I2CADDR, 0x0, 0xF0+val);
	writeI2CData(I2CADDR, 0x1, 0xFF);
	
	delay(30);
}

int readI2CMux()
{
	int GP0 = 0;
	int GP1 = 0;
	
	digitalWrite(WE, 1);
	digitalWrite(OE, 0);
	
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

// mode 0 - normal
// mode 1 - force refresh
// mode 2 - 1 + onelined
// mode 3 - 1 + month, year and century
TimeValue readTime(bool fullmode)
{
    static TimeValue ts;
	
    byte* vars[] = {&(ts.second), &(ts.minute), &(ts.hour), &(ts.dow), &(ts.day), &(ts.month), &(ts.year), &(ts.century)};
    int addrs[] = {0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x8};
    byte i;
    int val;
    
    dirI2CMux0(true);
    digitalWrite(OE, 0);
    digitalWrite(WE, 1);
    
    for(i=0; i<8; i++){
        setAddr(addrs[i]);
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
