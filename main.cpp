#include <WProgram.h>
#include <avr/sleep.h>

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

#define INTSPERSECOND 1 //1Hz
volatile uint8_t secondTimer;

#define SKIP555 (INTSPERSECOND*10) // about 10s

// Button debounce counter
#define DEBOUNCETIME 5 //180Hz * 5 = 27ms
volatile uint8_t debounceTimer;

#define HOLDTIME (INTSPERSECOND*5) // 5s
#define OFFTIME (INTSPERSECOND*15) // 15s

volatile uint8_t holdTimer;
volatile uint8_t inactivityTimer;

// Alarm we are waiting for
AlarmValue upcoming_alarm;
volatile bool is_upcoming_alarm;

#define ALARM_DURATION (60*45) // 60s * 45m; alarm turns off after 45 minutes
#define ALARM_BLINK (60*25) // 25 minutes
volatile uint16_t alarmTimer;

inline bool debounce()
{
    if(!debounceTimer){
        debounceTimer = DEBOUNCETIME;
        return false; // no debounce, valid event
    }
    else return true; // debouncing, discard event
}

// 555 timer
void debounceInterrupt()
{
    if(debounceTimer) debounceTimer--;
}

ISR(WDT_vect)
{
	static long timer555 = 0;

        // triggers once per second
        if(secondTimer){
            secondTimer--;
            if(!secondTimer){
                secondTimer = INTSPERSECOND;

                // turn of alarm after specified time
                if(alarmTimer){
                    alarmTimer--;
                    if(ALARM_DURATION - alarmTimer == ALARM_BLINK) queue.enqueueEvent(EV_ALARMMODE, 1);
                    else if(!alarmTimer) queue.enqueueEvent(EV_ALARM, 0);
                }
            }
        }

        // hold timer
        if(holdTimer){
            holdTimer--;
            if(!holdTimer) queue.enqueueEvent(EV_HOLD, 1);
        }

        // off timer, do not count during hold
        if(!holdTimer && inactivityTimer){
            inactivityTimer--;
            if(!inactivityTimer){
                // user did nothing for past 15 secs, disable backlight and return to time display
                queue.enqueueEvent(EV_BACKLIGHT, 0);
                queue.enqueueEvent(EV_INACTIVITY, 1);
                ste_totime();
            }
        }

	if(timer555==0){
		queue.enqueueEvent(EV_REFRESH, 0);
                queue.enqueueEvent(EV_CHECKALARM, 0);
		timer555 = SKIP555;
	}
	else timer555--;
}

void rotInterrupt(void)
{
        if(debounce()) return;

        if(!inactivityTimer) queue.enqueueEvent(EV_BACKLIGHT, st_backlight.getBacklight());
        inactivityTimer = OFFTIME;
	
	uint8_t b = digitalRead(ROTB);
	uint8_t a = digitalRead(ROTA);

        if(!a) return; // we only want half the steps
	
	if(b == a){
		queue.enqueueEvent(EV_RIGHT, 0);
	}
	else{
		queue.enqueueEvent(EV_LEFT, 0);
	}
}

// Pin Change vector 0 (Port B)
ISR(PCINT0_vect){
        static uint8_t oldstate = 1;

	if(debounce()) return;
        if(!inactivityTimer) queue.enqueueEvent(EV_BACKLIGHT, st_backlight.getBacklight());

        // change from LOW to HIGH (active LOW)
        if(!oldstate && (PINB & 0x02) &&
           holdTimer==0){
            inactivityTimer = OFFTIME; // hold event probably fired ages ago
        }
        else if(!oldstate && (PINB & 0x02)){
            // first release after the display was lighted does nothing
            // another presses a button
            if(inactivityTimer) queue.enqueueEvent(EV_SELECT, (PINB & 0x02));
            inactivityTimer = OFFTIME;
            holdTimer = 0; // reset hold timer
        }
        else{
            // first press event when the display is off just lights it
            if(inactivityTimer) queue.enqueueEvent(EV_SELECT, (PINB & 0x02));
            holdTimer = HOLDTIME;
        }

        oldstate = (PINB & 0x02);
}

void setup() {
	// RTC WE  & OE
	pinMode(WE, OUTPUT);
	pinMode(OE, OUTPUT);

	digitalWrite(WE, 1); // disable write
	digitalWrite(OE, 1); // disable read
	
	// Setup Temp.
	analogReference(DEFAULT);
	pinMode(TEMPERATURE, INPUT);
	pinMode(VOLTAGE, INPUT);
	
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
	
	writeI2CData(I2CDATA, 0x6 + I2CLCD, 0x0); // LCD set output mode
	writeI2CData(I2CDATA, 0x6 + I2CRTC, 0xFF); // RTC set input mode

        // Set default RTC address
        setAddr(0x0, 0x00, 0x00); // to reset the cache address
        setAddr(0x1, 0xff, 0xff); // best default for RTC
	
	// set high resolution mode for I2C expander DATA RTC port
	writeI2CData(I2CDATA, 0xA + I2CRTC, 0x1);
	
	//Init lcd
	delay(200);
	lcd.begin();
	lcd.clear();
        lcd.enableCursor(false, false);

	// Init 555 interrupt
        attachInterrupt(0, debounceInterrupt, FALLING);
	EIMSK |= _BV(INT0);
	EICRA = ISC01; // falling edge

        // Set Watchdog for timer operation
        cli();
        WDTCSR |= _BV(WDCE) | _BV(WDE); // enable change mode
        WDTCSR = _BV(WDIE) | _BV(WDP2) | _BV(WDP1); // interrupt once per second
        sei();

        // Init wireless, but keep the receiver disabled
	pinMode(RFE, OUTPUT);
	digitalWrite(RFE, 1); // disable wireless modules
        vw_set_tx_pin(5);
        vw_set_rx_pin(6);
        vw_setup(2000);
        //vw_rx_start();

        // Alarm board comm link
        alarmBoard.begin(9600);
	
        // Init states
        st_mainmenu.addMenuItem(0, "Podsviceni", ste_tobacklight);
        st_mainmenu.addMenuItem(1, "Budik", ste_toalarms);
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
	queue.enqueueEvent(EV_BACKLIGHT, 8);
        queue.enqueueEvent(EV_REFRESH, 0);
        queue.enqueueEvent(EV_REHASHALARM, 0);

        // Timers
        alarmTimer = 0;
        holdTimer = 0;
        debounceTimer = 0;
        inactivityTimer = OFFTIME;
        secondTimer = INTSPERSECOND;

	Serial.println("Setup done");
}



void writeTemp(LiquidCrystal &lcd, uint8_t col, uint8_t row)
{
	uint16_t temp = 0;
	uint16_t vcc;
	uint16_t vcccoef;
        uint8_t i;

	// calibrate sensor
	vcc = analogRead(VOLTAGE); //23.5.2 of the manual, discard first reading
	vcc = analogRead(VOLTAGE); //read internal hard 1.1V

	//should be 225 on stabilized 5V Aref
	vcccoef = map(vcc, 0, 1024, 0, 500);
	
	// discard one reading
	analogRead(TEMPERATURE);
	
	// average 4 readings
	for(i=0; i<4; i++)
            temp += analogRead(TEMPERATURE);
	temp >>= 2;

	temp = map(temp, 0, 1024, 0, 500);
	temp = temp - 273; //2982mV = 25C, 1C = 10mV

	lcd.setCursor(col+2,row);
	if(temp<10) lcd << ' ';
	lcd << temp << 'C';

	lcd.setCursor(col,row+1);
	lcd << _DEC(vcccoef/100) << "." << (((vcccoef%100) < 10)?"0":"") << _DEC(vcccoef % 100) << "V";
}

void sleepMode(bool full)
{
    /* Now is the time to set the sleep mode. In the Atmega8 datasheet
     * http://www.atmel.com/dyn/resources/prod_documents/doc2486.pdf on page 35
     * there is a list of sleep modes which explains which clocks and 
     * wake up sources are available in which sleep mode.
     *
     * In the avr/sleep.h file, the call names of these sleep modes are to be found:
     *
     * The 5 different modes are:
     *     SLEEP_MODE_IDLE         -the least power savings 
     *     SLEEP_MODE_ADC
     *     SLEEP_MODE_PWR_SAVE
     *     SLEEP_MODE_STANDBY
     *     SLEEP_MODE_PWR_DOWN     -the most power savings
     *
     * For now, we want as much power savings as possible, so we 
     * choose the according 
     * sleep mode: SLEEP_MODE_PWR_DOWN
     * 
     */  

    EIMSK &= ~_BV(INT0);
    
    if(full) set_sleep_mode(SLEEP_MODE_PWR_DOWN);   // sleep mode is set here
    else set_sleep_mode(SLEEP_MODE_IDLE);

    sleep_enable();          // enables the sleep bit in the mcucr register
                             // so sleep is possible. just a safety pin 
    sleep_mode();            // here the device is actually put to sleep!!
                             // THE PROGRAM CONTINUES FROM HERE AFTER WAKING UP

    sleep_disable();         // first thing after waking from sleep:
                             // disable sleep...
    EIMSK |= _BV(INT0);
}

void loop()
{
	int command = 0;
	int data1 = 0;
	
	if (Serial.available() > 0) {
		command = Serial.read();
		
                if(command==',') queue.enqueueEvent(EV_LEFT, 0);
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
                    alarmBoard.write(0xff);
                    alarmBoard.write(addr);
                    alarmBoard.write(hbyte);
                    alarmBoard.write(lbyte);
                    alarmBoard.write(0xfc); //commit to leds
                    Serial << "Set!" << endl;
                }

                else if(command=='b'){
                        int l;
                        for(l=0; l<16; l++){
                            alarmBoard.write(0xff);
                            alarmBoard.write(l);
                            alarmBoard.write(0xf);
                            alarmBoard.write(0xff);
                        }
                        alarmBoard.write(0xfd); //switch to second register
                        for(l=0; l<16; l++){
                            alarmBoard.write(0xff);
                            alarmBoard.write(l);
                            alarmBoard.write(0x0);
                            alarmBoard.write(0x00);
                        }
                        alarmBoard.write(0xfb); //start blinking
                        alarmTimer = ALARM_DURATION;
                }

                else if(command=='f'){
                        int l;
                        for(l=0; l<16; l++){
                            alarmBoard.write(0xff);
                            alarmBoard.write(l);
                            alarmBoard.write(0xf);
                            alarmBoard.write(0xff);
                        }
                        alarmBoard.write(0xfd); //switch to second register
                        for(l=0; l<16; l++){
                            alarmBoard.write(0xff);
                            alarmBoard.write(l);
                            alarmBoard.write(0x0);
                            alarmBoard.write(0x00);
                        }
                        alarmBoard.write(0xfe); //start fade
                        alarmTimer = ALARM_DURATION;
                }

                else if(command='A'){
                        queue.enqueueEvent(EV_ALARM, 1);
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
                        queue.enqueueEvent(EV_ALARM, 1);
                        queue.enqueueEvent(EV_REHASHALARM, 0); // find next alarm
                    }
                }
                else if(event==EV_REHASHALARM){
                    TimeValue tv = readTime(false);
                    uint8_t enabledAlarms = findNextAlarm(tv, &upcoming_alarm);
                    is_upcoming_alarm =  (enabledAlarms > 0);
                }
                else if(event==EV_ALARM){
                    if(data){
                        int l;

                        // full intensity for everything
                        for(l=0; l<16; l++){
                            alarmBoard.write(0xff);
                            alarmBoard.write(l);
                            alarmBoard.write(0xf);
                            alarmBoard.write(0xff);
                        }

                        alarmBoard.write(0xfd); //switch to second register

                        // start with red on quarter intensity
                        for(l=0; l<16; l++){
                            alarmBoard.write(0xff);
                            alarmBoard.write(l);

                            if(l<10){
                                alarmBoard.write(0x0);
                                alarmBoard.write(0x00);
                            }
                            else{
                                alarmBoard.write(0x4);
                                alarmBoard.write(0x00);
                            }
                        }

                        // start fading
                        alarmBoard.write(0xfe);
                        alarmTimer = ALARM_DURATION;

                        // if not lit, light up the display
                        if(!inactivityTimer) queue.enqueueEvent(EV_BACKLIGHT, st_backlight.getBacklight());
                    }
                    else{
                        alarmBoard.write(0xf0); //set 0 for everything
                        alarmBoard.write(0xfc); //commit to leds
                        alarmTimer = 0;
                    }
                }
                else if(event==EV_ALARMMODE){
                    if(data){
                        int l;
                        for(l=0; l<16; l++){
                            alarmBoard.write(0xff);
                            alarmBoard.write(l);
                            alarmBoard.write(0xf);
                            alarmBoard.write(0xff);
                        }
                        alarmBoard.write(0xfd); //switch to second register
                        for(l=0; l<16; l++){
                            alarmBoard.write(0xff);
                            alarmBoard.write(l);
                            alarmBoard.write(0x0);
                            alarmBoard.write(0x00);
                        }
                        alarmBoard.write(0xfb); //start blink
                    }
                }

                fsm.getCurrentState().event(event, data);
                fsm.update();
	}

        // no events to process check if we can save some power
        else{
            // backlight is off, hold is not running and button is up -> total sleep
            sleepMode(!inactivityTimer && !holdTimer && (PORTB & 0x02));
        }
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
