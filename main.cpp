#include <WProgram.h>

#include "Streaming.h"
#include "I2CLiquidCrystal.h"
#include "Wire.h"
#include "VirtualWire.h"
#include "NewSoftSerial.h"
#include "FiniteStateMachine.h"
#include "EventDispatcher.h"
#include "EventQueue.h"
#include "Events.h"
#include "BudikEvents.h"
#include "BudikStates.h"
#include "BudikInterfaces.h"
#include "utils.h"
#include "config.h"
#include "BudikRTC.h"

// I2C hooked LCD
I2CLiquidCrystal lcd(I2CDATA, I2CLCD, 6, 5, 4, 0, 1, 2, 3);

// Serial link to alarm board
NewSoftSerial alarmBoard(ALRX, ALTX);

// Interfaces
BudikBasicInterface intf_time(lcd);
BudikMenuInterface intf_menu(lcd);
BudikBacklightInterface intf_backlight(lcd);
BudikSetTimeInterface intf_settime(lcd);
BudikSetAlarmInterface intf_setalarm(lcd);
BudikTickerInterface intf_sensors(lcd);

// FSM transition prototypes
void ste_tomainmenu();
void ste_tosettime();
void ste_totime();
void ste_tobacklight();

// FSM states
TimeState st_time(intf_time, ste_tomainmenu, ste_tosettime);
SetTimeState st_settime(intf_settime, ste_totime);
AlarmsState st_alarms(intf_setalarm, ste_totime);
MenuState<5> st_mainmenu(intf_menu, "");
MenuState<2> st_submenu(intf_menu, "Sub menu");
BacklightState st_backlight(intf_backlight, 4, ste_tomainmenu);
//SensorState st_sensors(intf_sensors, ste_tomainmenu);

// FSM engine
FiniteStateMachine fsm(st_time);

// FSM transitions
void ste_totime() {fsm.transitionTo(st_time);}
void ste_tosettime() {fsm.transitionTo(st_settime);}
void ste_toalarms() {fsm.transitionTo(st_alarms);}
void ste_tomainmenu() {fsm.transitionTo(st_mainmenu);}
void ste_tosubmenu() {fsm.transitionTo(st_submenu);}
void ste_tobacklight() {fsm.transitionTo(st_backlight);}
//void ste_tosensors() {fsm.transitionTo(st_sensors);}

// 555 timer at about 200Hz
#define TIMER 2
#define INT555 TIMER-2 //interrupt 0, pin 2
#define SKIP555 1800 // about 10s

// Button debounce counter
#define DEBOUNCETIME 20
#define HOLDTIME 3000
long debouncems = 0;

// Alarm we are waiting for
AlarmValue upcoming_alarm;
bool is_upcoming_alarm;

void wakeupTimer(void)
{
	static long timer555 = 0;
	if(timer555==0){
		queue.enqueueEvent(EV_REFRESH, 0);
                queue.enqueueEvent(EV_CHECKALARM, 0);
		timer555 = SKIP555;
	}
	else timer555--;
}

void rotInterrupt(void)
{
	if(millis() - debouncems < DEBOUNCETIME) return;
	debouncems = millis();
	
	uint8_t b = digitalRead(ROTB);
	uint8_t a = digitalRead(ROTA);
	
	if(b == a){
		queue.enqueueEvent(EV_RIGHT, 0);
	}
	else{
		queue.enqueueEvent(EV_LEFT, 0);
	}
}

void setup() {
	// RTC WE  & OE, RFLink EN
	pinMode(WE, OUTPUT);
	pinMode(OE, OUTPUT);
	pinMode(RFE, OUTPUT);

	digitalWrite(WE, 1); // disable write
	digitalWrite(OE, 1); // disable read
	
	// Setup Timer
	pinMode(TIMER, INPUT);
	
	// Setup Temp.
	analogReference(DEFAULT);
	pinMode(TEMPERATURE, INPUT);
	pinMode(A0, INPUT);
	
	// Setup LCD backlight
	analogWrite(LCDLIGHT, 128);
	
	// Setup serial link
	Serial.begin(9600);
	
	//Setup I2c
	Wire.begin();
	
	// Setup MCP23016
	delay(100); // min 75ms power up timer
	writeI2CData(I2CADDR, 0x6, 0x0); // Set output mode
	writeI2CData(I2CADDR, 0x7, 0x0);
	writeI2CData(I2CADDR, 0x0, 0xFF); // Set address
	writeI2CData(I2CADDR, 0x1, 0xFF);
	
	writeI2CData(I2CDATA, 0x6 + I2CLCD, 0x0); // LCD set output mode
	writeI2CData(I2CDATA, 0x6 + I2CRTC, 0xFF); // RTC set input mode
	writeI2CData(I2CDATA, I2CLCD, 0x80); // highest bit is used for address
	
	// set high resolution mode for I2C expander DATA RTC port
	writeI2CData(I2CDATA, 0xA + I2CRTC, 0x1);
	
	//Init lcd
	delay(200);
	lcd.begin();
	lcd.clear();
        lcd.enableCursor(false, false);
        analogWrite(11, 128); // initialize backlight and it's PWM driver

	// Init 555 interrupt
	pinMode(TIMER, INPUT);
	attachInterrupt(INT555, wakeupTimer, FALLING);

        // Init wireless, but keep the receiver disabled
	digitalWrite(RFE, 1); // disable wireless modules
        vw_set_tx_pin(5);
        vw_set_rx_pin(6);
        vw_setup(2000);
        //vw_rx_start();

        // Alarm board comm link
        alarmBoard.begin(9600);
	
        // Init states
        st_mainmenu.addMenuItem(0, "Podsviceni", ste_tobacklight);
        st_mainmenu.addMenuItem(1, "Casovac", ste_toalarms);
        st_mainmenu.addMenuItem(2, "Akce", ste_tosubmenu);
        st_mainmenu.addMenuItem(3, "Senzory", ste_tomainmenu);
        st_mainmenu.addMenuItem(4, "Zpet", ste_totime);

        st_submenu.addMenuItem(0, "Zpet do menu", ste_tomainmenu);
        st_submenu.addMenuItem(1, "Podsviceni", ste_tobacklight);
        
	// Init rotary
	pinMode(ROTA, INPUT);
	pinMode(ROTB, INPUT);
	pinMode(ROTX, INPUT);
	digitalWrite(ROTA, HIGH);
	digitalWrite(ROTB, HIGH);
	digitalWrite(ROTX, HIGH);
	attachInterrupt(INTROT, rotInterrupt, CHANGE);
	
	//enable Pin Change Int for rotary button = PCINT1, port 9 (PB1)
	PCMSK0 = _BV(PCINT1);
	PCICR |= _BV(PCIE0); 

        // Inital refresh of gui
	queue.enqueueEvent(EV_BACKLIGHT, 4);
        queue.enqueueEvent(EV_REFRESH, 0);
        queue.enqueueEvent(EV_REHASHALARM, 0);

	Serial.println("Setup done");
}

// Pin Change vector 0 (Port B)
ISR(PCINT0_vect){
        static uint8_t oldstate = 0;

	if(millis() - debouncems < DEBOUNCETIME) return;

        // change from LOW to HIGH (active LOW)
        // hold event after 3s
        if(!oldstate &&
           (PINB & 0x02) &&
           (millis()-debouncems) >= HOLDTIME){
            queue.enqueueEvent(EV_HOLD, (PINB & 0x02));
        }
        else if(!oldstate && (PINB & 0x02)){
            queue.enqueueEvent(EV_SELECT, (PINB & 0x02));
        }
        else{
            queue.enqueueEvent(EV_SELECT, (PINB & 0x02));
        }

	debouncems = millis();
        oldstate = (PINB & 0x02);
}


void writeTemp(LiquidCrystal &lcd, uint8_t col, uint8_t row)
{
	uint16_t temp = 0;
	uint16_t vcc;
	uint16_t vcccoef;
	
	// calibrate sensor
	vcc = analogRead(A0); //23.5.2 of the manual, discard first reading
	vcc = analogRead(A0); //read internal hard 1.1V
	//should be 225 on stabilized 5V Aref
	vcccoef = map(vcc, 0, 1024, 0, 500);
	
	// discard one reading
	analogRead(TEMPERATURE);
	
	// average 4 readings
	//for(i=0; i<4; i++)
	temp = analogRead(TEMPERATURE);
	//temp = temp >> 2;
	temp = map(temp, 0, 1024, 0, 500);
	temp = temp - 273; //2982mV = 25C, 1C = 10mV
	lcd.setCursor(col+2,row);
	if(temp<10) lcd << ' ';
	lcd << temp << 'C';
	lcd.setCursor(col,row+1);
	lcd << "     ";
	lcd.setCursor(col,row+1);
	lcd << _DEC(vcccoef/100) << "." << ((vcccoef%100 > 10)?"":"0") << _DEC(vcccoef % 100) << "V";
}

void loop()
{
	int command = 0;
	int data1 = 0;
	
	if (Serial.available() > 0) {
		command = Serial.read();
		
		if(command=='L'){
                    while (Serial.available() <= 0) delayMicroseconds(2);
                    data1 = readHex(Serial.read());
                    while (Serial.available() <= 0) delayMicroseconds(2);
                    data1 = data1 << 4;
                    data1 += readHex(Serial.read());
                    if(data1==0) digitalWrite(LCDLIGHT, LOW);
                    else if(data1==0xFF) digitalWrite(LCDLIGHT, HIGH);
                    else analogWrite(LCDLIGHT, data1);
		}

                else if(command==',') queue.enqueueEvent(EV_LEFT, 0);
                else if(command=='.') queue.enqueueEvent(EV_RIGHT, 0);
                else if(command==' ') queue.enqueueEvent(EV_SELECT, 1);
                else if(command=='c') queue.enqueueEvent(EV_SELECT, 0);
                else if(command=='v') queue.enqueueEvent(EV_HOLD, 1);

                else if(command=='R'){
                    AlarmValue al;
                    al.hour = 0;
                    al.minute = 0;
                    al.dow = 0;
                    al.en = 0;
                    for(int idx=0; idx < ALARM_MAX; idx++)
                        writeAlarm(idx, al);
                    is_upcoming_alarm = false;
                }

                else if(command=='a'){
                    TimeValue tv = readTime(false);
                    uint8_t enabledAlarms = findNextAlarm(tv, &upcoming_alarm);
                    is_upcoming_alarm =  (enabledAlarms > 0);
                    if(is_upcoming_alarm){
                        Serial << _DEC(upcoming_alarm.id) << ". "
                               << _HEX(upcoming_alarm.hour) << ":" << _HEX(upcoming_alarm.minute)
                               << " " << _BIN(upcoming_alarm.dow) << endl;
                    }
                }

                else if(command=='o'){
                    uint8_t addr, hbyte, lbyte;
                    Serial << "Expecting 1 HEX char for LED address:" << endl;
                    while (!Serial.available());
                    addr = readHex(Serial.read());
                    Serial << "Expecting 3 HEX chars for LED value:" << endl;
                    while (!Serial.available());
                    hbyte = readHex(Serial.read());
                    while (!Serial.available());
                    lbyte = readHex(Serial.read()) << 4;
                    while (!Serial.available());
                    lbyte += readHex(Serial.read());
                    Serial << "Set!" << endl;
                    alarmBoard.write(0xff);
                    alarmBoard.write(addr);
                    alarmBoard.write(hbyte);
                    alarmBoard.write(lbyte);
                    alarmBoard.write(0xfc); //commit to leds
                }
	} 
	
        if(vw_have_message()){
            uint8_t msg[VW_MAX_MESSAGE_LEN];
            uint8_t len = VW_MAX_MESSAGE_LEN;

            
            if(vw_get_message(msg, &len)){
                int i;
                Serial << "RF:";
                for(i=0; i<len; i++) Serial << " " << _HEX(msg[i]);
                Serial << endl;
                //st_sensors.addData(msg[0], msg[1], msg[2]);
                queue.enqueueEvent(EV_REFRESH, 0);
            }
            else{
                //st_sensors.addData(0xfa, 0x11, 0xed);
                queue.enqueueEvent(EV_REFRESH, 0);
            }
        }

	// process queue
	if(!queue.isEmpty()){
		int event;
		int data;

                queue.dequeueEvent(&event, &data);

                if(event==EV_REFRESH){
                    digitalWrite(STATUSLED, HIGH);
                    fsm.getCurrentState().refresh(data);
                    digitalWrite(STATUSLED, LOW);
                }
                else if(event==EV_CHECKALARM && is_upcoming_alarm){
                    TimeValue tv = readTime(false);
                    if(tv.hour == upcoming_alarm.hour &&
                       tv.minute == upcoming_alarm.minute &&
                       (upcoming_alarm.dow & _BV(tv.dow-1))){
                        is_upcoming_alarm = false;
                        queue.enqueueEvent(EV_ALARM, upcoming_alarm.id);
                    }
                }
                else if(event==EV_REHASHALARM){
                    TimeValue tv = readTime(false);
                    uint8_t enabledAlarms = findNextAlarm(tv, &upcoming_alarm);
                    is_upcoming_alarm =  (enabledAlarms > 0);
                }

                fsm.getCurrentState().event(event, data);
	}

        fsm.update();
}

int main(void)
{
	init();

	setup();
    
	for (;;)
		loop();
        
	return 0;
}

extern "C" void __cxa_pure_virtual()
{
	cli();
	for (;;);
}
