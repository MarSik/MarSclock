#ifndef budik_events_20101223_h
#define budik_events_20101223_h

// events
enum _events {
	EV_SLEEP = 0,
	EV_LEFT = 1,
	EV_RIGHT,
	EV_SELECT,
        EV_HOLD,
	EV_TAP,
	EV_REFRESH,
	EV_BACKLIGHT
};

// the event queue
EventQueue queue;

#endif
