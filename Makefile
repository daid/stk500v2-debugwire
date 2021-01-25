# Product name
TARGET := stk500v2-debugwire

# Platform settings
CLOCK := 16000000
MCU   := atmega328p

# Toolchain
TPREFIX := avr-
AVRSIZE := size
COMPILE := g++
OBJCOPY := objcopy
OBJDUMP := objdump
AVRDUDE ?= avrdude

# Compiler and Linker flags
CXXFLAGS := -std=gnu++17
LDFLAGS  :=

# Project Source Directory
SRCDIR := src

# Include Directories
PINCDIRS := \
 	include
SINCDIRS := \

# Sources
SRCS := \
	main.cpp uart.cpp monosoftuart.cpp chipinfo.cpp debugwire.cpp

#- DON'T EDIT BELOW HERE UNLESS YOU KNOW WHAT YOU ARE GETTING YOURSELF INTO -#

AVRSIZE := $(shell command -v $(TPREFIX)$(AVRSIZE) 2>/dev/null)
COMPILE := $(shell command -v $(TPREFIX)$(COMPILE) 2>/dev/null)
OBJCOPY := $(shell command -v $(TPREFIX)$(OBJCOPY) 2>/dev/null)
OBJDUMP := $(shell command -v $(TPREFIX)$(OBJDUMP) 2>/dev/null)

PINCDIRS := $(patsubst %,-I %,$(PINCDIRS))
SINCDIRS := $(patsubst %,-isystem %,$(SINCDIRS))

BINDIR := bin
OBJDIR := .obj

CXXFLAGS_BASE := -g -Os -Wextra -Werror -pedantic-errors -mmcu=$(MCU) -DF_CPU=$(CLOCK) -ffunction-sections -fdata-sections
LDFLAGS_BASE  := -mmcu=$(MCU) -flto -Wl,--gc-sections

CXXFLAGS := $(CXXFLAGS_BASE) $(CXXFLAGS) $(SINCDIRS) $(PINCDIRS)
LDFLAGS  := $(LDFLAGS_BASE) $(LDFLAGS)

SRCS := $(patsubst %.cpp,$(SRCDIR)/%.cpp,$(SRCS))
OBJS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))
DEPS := $(patsubst %.o,%.d,$(OBJS))
ELFS := $(BINDIR)/$(TARGET).elf
HEXS := $(BINDIR)/$(TARGET).hex

.PHONY: clean
.SECONDARY: $(OBJS) $(ELFS)

#-# General Goals

all: $(HEXS)

upload: $(HEXS)
	@$(AVRDUDE) -p$(MCU) -carduino -P$(AVRDUDE_PORT) -b57600 -Uflash:w:"$(HEXS)":i

clean:
	@rm -rf $(BINDIR)
	@rm -rf $(OBJDIR)

#-# File Pattern Goals

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	@echo "COMPILE $@"
	@$(COMPILE) $(CXXFLAGS) -MMD -c -o $@ $<

-include $(OBJDIR)/*.d

$(BINDIR)/%.elf: $(OBJS) | $(BINDIR)
	@echo "LINK $@"
	@$(COMPILE) $(LDFLAGS) -o $@ $^
	@$(AVRSIZE) -C --mcu=$(MCU) $@
	@$(OBJDUMP) -d -S $@ > $@.list

$(BINDIR)/%.hex: $(ELFS) | $(BINDIR)
	@echo "OBJCOPY $@"
	@$(OBJCOPY) -j .text -j .data -O ihex $^ $@

#-# Directory Goals

$(OBJDIR):
	@mkdir -p $@

$(BINDIR):
	@mkdir -p $@
