# Teensyduino Core Library
# http://www.pjrc.com/teensy/
# Copyright (c) 2017 PJRC.COM, LLC.
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# 1. The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# 2. If the Software is incorporated into a build system that allows
# selection among a list of target devices, then similar target
# devices manufactured by PJRC.COM must be included in the list of
# target devices and selectable in the same manner.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# set your MCU type here, or make command line `make MCU=MK20DX256`
MCU=MK66FX1M0

# make it lower case
LOWER_MCU := $(subst A,a,$(subst B,b,$(subst C,c,$(subst D,d,$(subst E,e,$(subst F,f,$(subst G,g,$(subst H,h,$(subst I,i,$(subst J,j,$(subst K,k,$(subst L,l,$(subst M,m,$(subst N,n,$(subst O,o,$(subst P,p,$(subst Q,q,$(subst R,r,$(subst S,s,$(subst T,t,$(subst U,u,$(subst V,v,$(subst W,w,$(subst X,x,$(subst Y,y,$(subst Z,z,$(MCU)))))))))))))))))))))))))))
MCU_LD = $(LOWER_MCU).ld

# The name of your project (used to name the compiled .hex file)
TARGET = frequencyshifter

BUILDDIR=build

OPTIONS = -DF_CPU=180000000 -DUSB_MIDI_SERIAL -DLAYOUT_US_ENGLISH -DUSING_MAKEFILE

OPTIONS += -D__$(MCU)__ -DARDUINO=10805 -DTEENSYDUINO=144

CPUARCH = cortex-m4



# path location for Teensy Loader, teensy_post_compile and teensy_reboot (on Linux)
TOOLSPATH = tools

COREPATH =  teensy3

# path location for Arduino libraries (currently not used)
LIBRARYPATH =  libraries

# path location for the arm-none-eabi compiler
COMPILERPATH =  tools/arm/bin

LD_SCRIPT = $(COREPATH)/$(MCU_LD)



#************************************************************************
# Settings below this point usually do not need to be edited
#************************************************************************

# CPPFLAGS = compiler options for C and C++
CPPFLAGS = -Wall -g -Os -mcpu=$(CPUARCH) -mthumb -MMD $(OPTIONS) -I. -I$(LIBRARYPATH)/BALibrary-master/src -I$(COREPATH) -I$(LIBRARYPATH)/Wire -I$(LIBRARYPATH)/Audio -I$(LIBRARYPATH)/Audio/utility -I$(LIBRARYPATH)/Bounce2 -I$(LIBRARYPATH)/Encoder -I$(LIBRARYPATH)/SPI -I$(LIBRARYPATH)/SD -I$(LIBRARYPATH)/SD/utility -I$(LIBRARYPATH)/SerialFlash -I$(LIBRARYPATH)/SoftwareSerial -I$(LIBRARYPATH)/OpenAudio_ArduinoLibrary -I$(LIBRARYPATH)/MIDI/src

# compiler options for C++ only
CXXFLAGS = -std=gnu++14 -felide-constructors -fno-exceptions -fno-rtti

# compiler options for C only
CFLAGS =

# linker options
LDFLAGS = -Os -Wl,--gc-sections,--defsym=__rtc_localtime=0 --specs=nano.specs -mcpu=$(CPUARCH) -mthumb -T$(LD_SCRIPT)

# additional libraries to link
LIBS = -lm -larm_cortexM4l_math


# names for the compiler programs
CC = $(COMPILERPATH)/arm-none-eabi-gcc
CXX = $(COMPILERPATH)/arm-none-eabi-g++
OBJCOPY = $(COMPILERPATH)/arm-none-eabi-objcopy
SIZE = $(COMPILERPATH)/arm-none-eabi-size

# automatically create lists of the sources and objects
# TODO: this does not handle Arduino libraries yet...
C_FILES := $(wildcard src/*.c)
CPP_FILES := $(wildcard src/*.cpp)
TEENSY_C_FILES := $(wildcard $(COREPATH)/*.c)
TEENSY_CPP_FILES := $(wildcard $(COREPATH)/*.cpp)
LIBRARY_C_FILES := $(wildcard $(LIBRARYPATH)/*/*.c)
LIBRARY_CPP_FILES := $(wildcard $(LIBRARYPATH)/*/*.cpp)
LIBRARY_SD_CPP_FILES := $(wildcard $(LIBRARYPATH)/SD/*/*.cpp)
LIBRARY_MIDI_CPP_FILES := $(wildcard $(LIBRARYPATH)/MIDI/src/*.cpp)
LIBRARY_S_FILES := $(wildcard $(LIBRARYPATH)/*/*.S)
BA_C_FILES := $(wildcard $(LIBRARYPATH)/BALibrary-master/src/*/*.c)
BA_CPP_FILES := $(wildcard $(LIBRARYPATH)/BALibrary-master/src/*/*.cpp)

OBJ_FILES := $(C_FILES:.c=.o) $(CPP_FILES:.cpp=.o) $(BA_C_FILES:.c=.o) $(BA_CPP_FILES:.cpp=.o) $(LIBRARY_C_FILES:.c=.o) $(LIBRARY_CPP_FILES:.cpp=.o) $(LIBRARY_SD_CPP_FILES:.cpp=.o) $(LIBRARY_MIDI_CPP_FILES:.cpp=.o) $(LIBRARY_S_FILES:.S=.o) $(TEENSY_C_FILES:.c=.o) $(TEENSY_CPP_FILES:.cpp=.o)
OBJS := $(foreach obj,$(OBJ_FILES), $(BUILDDIR)/$(obj))

all: $(TARGET).hex

$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

$(BUILDDIR)/%.o: %.c
	$(Q)mkdir -p "$(dir $@)"
	$(Q)$(CC) $(CPPFLAGS) $(CFLAGS) $(L_INC) -o "$@" -c "$<"

$(BUILDDIR)/%.o: %.cpp
	$(Q)mkdir -p "$(dir $@)"
	$(Q)$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(L_INC) -o "$@" -c "$<"

$(BUILDDIR)/%.o: %.S
	$(Q)mkdir -p "$(dir $@)"
	$(Q)$(CXX) $(CPPFLAGS) $(L_INC) -o "$@" -c "$<"

%.hex: %.elf
	$(SIZE) $<
	$(OBJCOPY) -O ihex -R .eeprom $< $@
ifneq (,$(wildcard $(TOOLSPATH)))
	$(TOOLSPATH)/teensy_post_compile -file=$(basename $@) -path=$(shell pwd) -tools=$(TOOLSPATH)
	-$(TOOLSPATH)/teensy_reboot
endif

# compiler generated dependency info
-include $(OBJS:.o=.d)

clean:
	@echo Cleaning...
	$(Q)rm -rf "$(BUILDDIR)"
	$(Q)rm -f "$(TARGET).elf" "$(TARGET).hex" "$(TARGET).map"
