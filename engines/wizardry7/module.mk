MODULE := engines/wizardry7

MODULE_OBJS := \
	attack.o \
	console.o \
	creation.o \
	equipment.o \
	font.o \
	gfx.o \
	items.o \
	maze.o \
	metaengine.o \
	monster.o \
	msg.o \
	party.o \
	rng.o \
	save.o \
	scenario.o \
	screendiff.o \
	trace.o \
	view.o \
	wizardry7.o \
	xp.o

# This module can be built as a plugin
ifeq ($(ENABLE_WIZARDRY7), DYNAMIC_PLUGIN)
PLUGIN := 1
endif

# Include common rules
include $(srcdir)/rules.mk

# Detection objects
DETECT_OBJS += $(MODULE)/detection.o
