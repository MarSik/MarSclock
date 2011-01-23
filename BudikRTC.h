#ifndef budik_rtc_20110102_h
#define budik_rtc_20110102_h

#include "BudikInterfaces.h"
#include "I2CLiquidCrystal.h"

#define ALARM_MAX 64

void setAddr(uint8_t a2, uint8_t a1, uint8_t a0);
int readI2CMux();
void dirI2CMux0(boolean input);
void writeI2CMux0(byte data);
void writeI2CData(int address, byte block, byte data);
TimeValue readTime(bool fullmode);
void writeTime(TimeValue &t);
void writeAlarm(uint8_t idx, AlarmValue &v);
AlarmValue readAlarm(uint8_t alidx);
uint8_t findNextAlarm(TimeValue &tv, AlarmValue *upcoming);

#endif
