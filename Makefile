BUILD_DIR := build

# Choose build mode, don't define for release
BUILD_MODE := debug

# Choose backend, don't define for native
# BACKEND := raylib

# Choose display manager, don't define for Wayland
# DISPLAY_MANAGER := x_

ifndef BUILD_MODE
	BUILD_MODE := release
endif

MACROS :=

ifeq ($(BUILD_MODE),debug)
	MACROS := $(MACROS) EDEBUG_MODE
endif

ifeq ($(BACKEND),raylib)
	INC_DIR := raylib-5.0_win64_msvc16/include
	LIB_DIR := raylib-5.0_win64_msvc16/lib
	ADD_LIBS := raylib.lib
else
	INC_DIR :=
	LIB_DIR :=
	ADD_LIBS :=
endif

null :=
space := $(null) $(null)
comma := ,

ifeq ($(OS),Windows_NT)
	ifndef BACKEND
	    BACKEND := win
	endif
	BUILD_CMD := winenv
	CLEAN_CMD := winclean
	CC := cl
	CFLAGS := /c $(foreach I,$(INC_DIR),/I.\$(I) ) /Fo
	EXTENSION := dll
	OBJ_EXT := obj
	LINKER := link
	LINKER_FLAGS := /DLL /NODEFAULTLIB:libcmt $(foreach I,$(LIB_DIR),/LIBPATH:.\$(I) ) /OUT:
	EXT_LIBS := User32.lib Gdi32.lib Shell32.lib winmm.lib $(ADD_LIBS)
else
	ifndef BACKEND
	    BACKEND := unix
	endif
	ifndef DISPLAY_MANAGER
		DISPLAY_MANAGER := wl_
	endif
	BUILD_CMD := build
	CLEAN_CMD := linuxclean
	CC := gcc
	CFLAGS := -c -g $(foreach I,$(INC_DIR),-I$(I) ) $(foreach M,$(MACROS),-D$(M) ) -fPIC -o 
	EXTENSION := so
	OBJ_EXT := o
	LINKER := gcc
	LINKER_FLAGS := -shared $(foreach I,$(LIB_DIR),-L $(I) ) -o 
	EXT_LIBS := -lm -ldl -lpthread
endif

SRC_FILES := engine.c $(BACKEND)/$(DISPLAY_MANAGER)window.c logger.c #scene.c $(BACKEND)/renderer.c $(BACKEND)/asset.c
OBJ_FILES := $(patsubst %.c,$(BUILD_DIR)/%.$(OBJ_EXT),$(notdir $(SRC_FILES)))

all: $(BUILD_CMD)

winenv:
	@call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" && make build 

build: $(BUILD_DIR)/libegg.$(EXTENSION) 

$(BUILD_DIR)/%.$(OBJ_EXT): src/%.c
	$(CC) $< $(CFLAGS)$@

$(BUILD_DIR)/%.$(OBJ_EXT): src/$(BACKEND)/%.c
	$(CC) $< $(CFLAGS)$@

$(BUILD_DIR)/libegg.$(EXTENSION): $(OBJ_FILES)
	$(LINKER) $(OBJ_FILES) $(EXT_LIBS) $(LINKER_FLAGS)$@

clean: $(CLEAN_CMD)

winclean:
	powershell -Command "foreach ($$path in @($(subst $(space),$(comma),$(foreach obj,$(OBJ_FILES),'$(obj)')))) { if (Test-Path $$path) { rm -r -fo $$path } }"

linuxclean:
	rm -rf $(OBJ_FILES)

gendb:
	bear -- make

.PHONY: clean all winenv winclean linuxclean gendb

build/test.exe: test.c src/entry.h
	@call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" && cl test.c build/libegg.lib /Febuild/test.exe

build/test2: test2.c src/entry.h
	gcc -g $(foreach M,$(MACROS),-D$(M) ) test2.c build/libegg.so -o build/test2
