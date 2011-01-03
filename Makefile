# Arduino makefile
#
# This makefile allows you to build sketches from the command line
# without the Arduino environment (or Java).
#
# The Arduino environment does preliminary processing on a sketch before
# compiling it.  If you're using this makefile instead, you'll need to do
# a few things differently:
#
#   - Give your program's file a .cpp extension (e.g. foo.cpp).
#
#   - Put this line at top of your code: #include <WProgram.h>
#
#   - Write prototypes for all your functions (or define them before you
#     call them).  A prototype declares the types of parameters a
#     function will take and what type of value it will return.  This
#     means that you can have a call to a function before the definition
#     of the function.  A function prototype looks like the first line of
#     the function, with a semi-colon at the end.  For example:
#     int digitalRead(int pin);
#
#   - Write a main() function for your program that returns an int, calls
#     init() and setup() once (in that order), and then calls loop()
#     repeatedly():
#
#	int main()
#	{
#		init();
#		setup();
#
#		for (;;)
#			loop();
#
#		return 0;
#	}
#
# Instructions for using the makefile:
#
#  1. Copy MAkefile and Makefile.inc into the folder with your sketch.
#
#  2. Below, modify the line containing "TARGET" to refer to the name of
#     of your program's file without an extension (e.g. TARGET = foo).
#
#  3. Modify the line containg "ARDUINO" to point the directory that
#     contains the Arduino core (for normal Arduino installations, this
#     is the hardware/cores/arduino sub-directory).
#
#  4. Modify the line containing "PORT" to refer to the filename
#     representing the USB or serial connection to your Arduino board
#     (e.g. PORT = /dev/tty.USB0).  If the exact name of this file
#     changes, you can use * as a wildcard (e.g. PORT = /dev/tty.USB*).
#
#  5. At the command line, change to the directory containing your
#     program's file and the makefile.
#
#  6. Type "make" and press enter to compile/verify your program.
#
#  7. Type "make upload", reset your Arduino board, and press enter  to
#     upload your program to the Arduino board.
#
# $Id$

# Update these to reflect the location of the Arduino IDE and avrdude installations, respectively
ARDUINO = /Applications/Arduino.app/Contents/Resources/Java/hardware/arduino/cores/arduino/
ARDUINO_LIB = /Applications/Arduino.app/Contents/Resources/Java/libraries/
AVRDUDE_DIR = /usr/local/CrossPAck-AVR/bin/
#ARDUINO = /home/msivak/Applications/arduino-0021/hardware/arduino/cores/arduino/
#ARDUINO_LIB =  /home/msivak/Applications/arduino-0021/libraries/
#AVRDUDE_DIR = /usr/bin/

# Target device
MCU = atmega328p
F_CPU = 16000000
UPLOAD_RATE = 115200
LFUSE =
HFUSE =
EFUSE =

# ISP Programmer
PORT = /dev/tty.usbmodem*
#PORT = /dev/ttyACM*
FORMAT = ihex
PROTOCOL = stk500v1

# Application files and targets
TARGET = main

# Place -I options here
CINCS = -I$(ARDUINO)
CXXINCS = -I$(ARDUINO)

# Place -L and -l here
LIBS =

# Source files for C compiler
SRC = $(ARDUINO)/pins_arduino.c $(ARDUINO)/wiring.c \
$(ARDUINO)/wiring_analog.c $(ARDUINO)/wiring_digital.c \
$(ARDUINO)/wiring_pulse.c  \
$(ARDUINO)/wiring_shift.c $(ARDUINO)/WInterrupts.c twi.c utils.c

# Source files for C++ compiler
CXXSRC = main.cpp $(ARDUINO)/HardwareSerial.cpp $(ARDUINO)/WMath.cpp $(ARDUINO)/Print.cpp \
$(ARDUINO)/WString.cpp $(ARDUINO)/Tone.cpp \
EventDispatcher.cpp EventQueue.cpp FiniteStateMachine.cpp I2CLiquidCrystal.cpp VirtualWire.cpp \
Wire.cpp BudikRTC.cpp BudikInterfaces.cpp

# This includes the rest of rules DO NOT DELETE
-include Makefile.inc
