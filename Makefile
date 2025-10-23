BUILD_DIR := build

INC_DIR := raylib-5.0_win64_msvc16/include
LIB_DIR := raylib-5.0_win64_msvc16/lib

null :=
space := $(null) $(null)
comma := ,

ifeq ($(OS),Windows_NT)
	BUILD_CMD := winenv
	CLEAN_CMD := winclean
	CC := cl
	CFLAGS := /c /I.\$(INC_DIR) /Fo
	EXTENSION := dll
	OBJ_EXT := obj
	LINKER := link
	LINKER_FLAGS := /DLL /NODEFAULTLIB:libcmt /LIBPATH:.\$(LIB_DIR) /OUT:
	EXT_LIBS := User32.lib Gdi32.lib Shell32.lib winmm.lib raylib.lib
else
	BUILD_CMD := build
	CLEAN_CMD := linuxclean
	CC := gcc
	CFLAGS := -c -I$(INC_DIR) -o 
	EXTENSION := so
	OBJ_EXT := o
	LINKER := gcc
	LINKER_FLAGS := -shared -L $(LIB_DIR) -o 
	EXT_LIBS := -lm -ldl -lpthread
endif

SRC_FILES := engine.c window.c renderer.c scene.c asset.c
OBJ_FILES := $(SRC_FILES:%.c=$(BUILD_DIR)/%.$(OBJ_EXT))

all: $(BUILD_CMD)

winenv:
	@call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" && make build 

build: $(BUILD_DIR)/libegg.$(EXTENSION) 

$(BUILD_DIR)/%.$(OBJ_EXT): src/%.c
	$(CC) $< $(CFLAGS)$@

$(BUILD_DIR)/libegg.$(EXTENSION): $(OBJ_FILES)
	$(LINKER) $(OBJ_FILES) $(EXT_LIBS) $(LINKER_FLAGS)$@

clean: $(CLEAN_CMD)

winclean:
	powershell -Command "foreach ($$path in @($(subst $(space),$(comma),$(foreach obj,$(OBJ_FILES),'$(obj)')))) { if (Test-Path $$path) { rm -r -fo $$path } }"

linuxclean:
	rm -rf $(OBJ_FILES)

.PHONY: clean all winenv winclean linuxclean

build/test.exe: test.c src/entry.h
	@call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" && cl test.c build/libegg.lib /Febuild/test.exe

