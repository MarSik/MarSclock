#ifndef budik_states_20101223_h
#define budik_states_20101223_h

#include "WProgram.h"
#include "BudikEvents.h"
#include "BudikInterfaces.h"
#include "utils.h"
#include "config.h"
#include "BudikRTC.h"

class BudikState: public State {
 public:
 BudikState(): State() {}

    virtual void event(int event, int data)
    {
        switch(event){
        case EV_BACKLIGHT:
            analogWrite(LCDLIGHT, (1 << data)-1);
            break;
        }
    }
};

class TimeState : public BudikState {
protected:
    void (*funcPress)();
    void (*funcHold)();
    BudikTimeInterface &out;

public:
 TimeState(BudikTimeInterface &out,
           void (*funcPress)(),
           void (*funcHold)()):
    BudikState(), funcPress(funcPress), funcHold(funcHold), out(out)
    {
    };

    virtual void event(int event, int data)
    {
        switch(event){
        case EV_LEFT:
            break;
        case EV_RIGHT:
            break;
        case EV_SELECT:
            if(data) (*funcPress)();
            break;
        case EV_HOLD:
            if(data) (*funcHold)();
            break;
        }

        BudikState::event(event, data);
    };

    virtual void update()
    {
    }

    virtual void enter()
    {
        out.setup(0, 0);
        refresh(1);
    }

    virtual void refresh(int data)
    {
        out.setTime(readTime(false));
        out.print(0, 0, data);
    }

};

class SetTimeState : public BudikState {
protected:
    void (*funcHold)();
    BudikSetTimeInterface &out;
    uint8_t nibble;
    bool set;

public:
 SetTimeState(BudikSetTimeInterface &out,
           void (*funcHold)()):
    BudikState(), funcHold(funcHold), out(out),
        nibble(0), set(false)
    {
    };

    virtual void event(int event, int data)
    {
        switch(event){
        case EV_LEFT:
            // select number on the left
            if(!set && nibble){
                nibble--;
                queue.enqueueEvent(EV_REFRESH, 0);
            }
            break;

        case EV_RIGHT:
            // select number on the right
            if(!set && nibble<6){
                nibble++;
                queue.enqueueEvent(EV_REFRESH, 0);
            }
            break;

        case EV_SELECT:
            // select number vs. set number
            if(data){
                set = !set;
                queue.enqueueEvent(EV_REFRESH, 0);
            }
            break;

        case EV_HOLD:
            if(data) (*funcHold)();
            break;
        }

        BudikState::event(event, data);
    };

    virtual void update()
    {
    }

    virtual void enter()
    {
        out.setup(0, 0);
        refresh(1);
    }

    virtual void refresh(int data)
    {
        out.setTime(readTime(false));
        out.setMode(set);
        out.setNibble(nibble);
        out.print(0, 0, data);
    }

};

class AlarmsState : public BudikState {
protected:
    void (*funcHold)();
    BudikSetAlarmInterface &out;
    uint8_t nibble;
    bool set;

public:
 AlarmsState(BudikSetAlarmInterface &out,
           void (*funcHold)()):
    BudikState(), funcHold(funcHold), out(out),
        nibble(0), set(false)
    {
    };

    virtual void event(int event, int data)
    {
        switch(event){
        case EV_LEFT:
            // select number on the left
            if(!set && nibble){
                nibble--;
                queue.enqueueEvent(EV_REFRESH, 0);
            }
            break;

        case EV_RIGHT:
            // select number on the right
            if(!set && nibble<6){
                nibble++;
                queue.enqueueEvent(EV_REFRESH, 0);
            }
            break;

        case EV_SELECT:
            // select number vs. set number
            if(data){
                set = !set;
                queue.enqueueEvent(EV_REFRESH, 0);
            }
            break;

        case EV_HOLD:
            if(data) (*funcHold)();
            break;
        }

        BudikState::event(event, data);
    };

    virtual void update()
    {
    }

    virtual void enter()
    {
        out.setup(0, 0);
        refresh(1);
    }

    virtual void refresh(int data)
    {
        out.setTime(readTime(false));
        out.setMode(set);
        out.setNibble(nibble);
        out.print(0, 0, data);
    }

};

template<int NoOfItems>
class MenuState : public BudikState {
protected:
    BudikMenuInterface &out;
    int selected;
public:
 MenuState(BudikMenuInterface &out, char* title) :
    BudikState(), title(title), out(out), selected(0) {};

    void addMenuItem(int no, const char *title, void (*func)())
    {
        menuItems[no].title = title;
        menuItems[no].func = func;
    };

    virtual void event(int event, int data)
    {
        switch(event){
        case EV_LEFT:
            if(selected) selected--;
            else selected = NoOfItems - 1;
            queue.enqueueEvent(EV_REFRESH, 0);
            break;
        case EV_RIGHT:
            if(selected<NoOfItems-1) selected++;
            else selected = 0;
            queue.enqueueEvent(EV_REFRESH, 0);
            break;
        case EV_SELECT:
            if(data) (*(menuItems[selected].func))();
            break;
        }

        BudikState::event(event, data);
    };

    virtual void refresh(int data)
    {
        out.setTime(readTime(false));
        out.setItem(menuItems[selected].title);
        out.print(0,0, data);
    };

    virtual void enter(void)
    {
        out.setTitle(title);
        out.setup(0, 0);
        refresh(1);
    }

    virtual void exit(void)
    {
    }

protected:
    struct {
        const char *title;
        void (*func)();
    } menuItems[NoOfItems];
    char *title;
};

class BacklightState : public BudikState {
protected:
    int backlight;
    void (*func)();
    BudikBacklightInterface &out;

public:
 BacklightState(BudikBacklightInterface &out, int backlight, void (*func)()):
    BudikState(), func(func), out(out), backlight(backlight)
    {
    };

    virtual void event(int event, int data)
    {
        switch(event){
        case EV_LEFT:
            if(backlight) backlight--;
            queue.enqueueEvent(EV_BACKLIGHT, backlight);
            queue.enqueueEvent(EV_REFRESH, 0);
            break;
        case EV_RIGHT:
            if(backlight<8) backlight++;
            queue.enqueueEvent(EV_BACKLIGHT, backlight);
            queue.enqueueEvent(EV_REFRESH, 0);
            break;
        case EV_SELECT:
            if(data) (*func)();
            break;
        }

        BudikState::event(event, data);
    };

    virtual void enter()
    {
        out.setup(0, 0);
        refresh(1);
    }

    virtual void refresh(int data)
    {
        out.setTime(readTime(false));
        out.setBacklight(backlight);
        out.print(0, 0, data);
    }

};


class SensorState : public BudikState {
protected:
    void (*func)();
    BudikTickerInterface &out;
    uint8_t sensor[5];
    uint8_t code[5];
    uint8_t value[5];
    int first;
    int last;

public:
 SensorState(BudikTickerInterface &out, void (*func)()):
    BudikState(), func(func), out(out), first(0), last(0)
    {
    };

    virtual void event(int event, int data)
    {
        switch(event){
        case EV_SELECT:
            if(data) (*func)();
            break;
        }

        BudikState::event(event, data);
    };

    virtual void enter()
    {
        out.setup(0, 0);
        refresh(1);
    }

    virtual void refresh(int data)
    {
        String str;
        int i = last;
        int c = 0;
        
        while(i!=first){
            i--;
            if(i<0) i=4;
            str += String(sensor[i], HEX);
            str += String(code[i], HEX);
            str += String(value[i], HEX);
            str += " ";
            c++;
            if(c==2){
                out.setLine1(str);
                str = "";
            }
        }

        out.setLine2(str);
        out.setTime(readTime(false));
        out.print(0, 0, data);
    }

    virtual void addData(uint8_t sens0, uint8_t code0, uint8_t val0)
    {
        sensor[last] = sens0;
        code[last] = code0;
        value[last] = val0;

        last++;
        if(last==5) last=0;
        if(last==first){
            first++;
            if(first==5) first=0;
        }
    }

};


#endif
