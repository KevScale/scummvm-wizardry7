MODULE := engines/wizardry7

MODULE_OBJS := \
	console.o \
	font.o \
	gfx.o \
	metaengine.o \
	msg.o \
	rng.o \
	save.o \
	scenario.o \
	wizardry7.o

# This module can be built as a plugin
ifeq ($(ENABLE_WIZARDRY7), DYNAMIC_PLUGIN)
PLUGIN := 1
endif

# Include common rules
include $(srcdir)/rules.mk

# Detection objects
DETECT_OBJS += $(MODULE)/detection.o
