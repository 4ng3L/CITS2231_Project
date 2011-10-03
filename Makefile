define MinCyg_GCC
sed -n /'^gcc version '/s',.*\<\([A-Za-z]*\)\> \(special\|experimental\).*',\\1,p
endef
define Any_GCC
sed -n /'^gcc version '/s',.*\<\([A-Za-z]*\)\>.*',\\1,p
endef

ASK_SHELL_FIND_GCC := $(shell gcc -v 2>&1 | $(MinCyg_GCC))
 ifeq "$(ASK_SHELL_FIND_GCC)" ""
ASK_SHELL_FIND_GCC := $(shell gcc -v 2>&1 | $(Any_GCC))
 endif
 ifneq "$(ASK_SHELL_FIND_GCC)" ""
GCC_FLAVOR := $(strip $(ASK_SHELL_FIND_GCC))
 endif

ifeq "$(GCC_FLAVOR)" "cygming"
CFLAGS  += -mno-cygwin 
override LIBS += -lglut32 -lglu32 -lopengl32
endif

LIBS += -lglut -lGLU -lGL

CFLAGS += -O3 -Wall -std=c99

.PHONY: all

all: scene

scene:	scene.c bitmap.c bitmap.h
	gcc $(CFLAGS) -o scene scene.c bitmap.c $(LIBS)
