UI := GUI
BUILD := debug
CXX := clang++
CC := clang

CXXFLAGS_BASE := -std=c++20 -Iinclude
ifneq ($(OS),Windows_NT)
WL_DIR := $(shell pkg-config --variable=pkgdatadir wayland-protocols)
WL_PROTOCOLS := $(WL_DIR)/stable/xdg-shell/xdg-shell.xml $(WL_DIR)/staging/fractional-scale/fractional-scale-v1.xml $(WL_DIR)/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml
WL_SOURCE := $(WL_PROTOCOLS:$(WL_DIR)/%.xml=.wayland/%.c)
WL_HEADERS := $(WL_SOURCE:.c=.h)
WL_OBJ := $(WL_SOURCE:.c=.o)
LIB_WL := libwayland-protocols.a
EXE :=
CXXFLAGS_LIBS := -I.wayland $(shell pkg-config --cflags wayland-client wayland-cursor vulkan)
LDFLAGS := $(shell pkg-config --libs wayland-client wayland-cursor vulkan)
else
EXE := .exe
CXXFLAGS_LIBS := -DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN
ifneq ($(UI),TerminalUI)
SUBSYSTEM := -mwindows -municode
endif
LDFLAGS := -l:libvulkan-1.dll.a -lkernel32 -luser32 -lgdi32 $(SUBSYSTEM) -static-libgcc -static-libstdc++ -static
endif
ifeq ($(BUILD),debug)
CXXFLAGS := -Wall -Wextra -g3 -O0 -DDEBUG -fno-omit-frame-pointer $(CXXFLAGS_BASE)
else ifeq ($(BUILD),release)
CXXFLAGS := -O3 -DNDEBUG $(CXXFLAGS_BASE)
endif

ifneq ($(OS),Windows_NT)
SRC_ENGN := $(shell find Engine/$(UI)/common Engine/$(UI)/linux -name "*.cpp")
else
SRC_ENGN := $(shell find Engine/$(UI)/common Engine/$(UI)/win -name "*.cpp")
endif
OBJ_ENGN := $(SRC_ENGN:%.cpp=.build/%.o)
DEP_ENGN := $(OBJ_ENGN:.o=.d)

SRC_GAME := $(shell find Game -name "*.cpp")
OBJ_GAME := $(SRC_GAME:%.cpp=.build/%.o)
DEP_GAME := $(OBJ_GAME:.o=.d)

.PHONY: run all clean clean-wayland

run: all
	./game$(EXE)

all: libengine.a game$(EXE) shader.spv

clean:
	$(RM) -r .build libengine.a game$(EXE)

clean-wayland:
	$(RM) -r .wayland libwayland-protocols.a

libengine.a: $(OBJ_ENGN)
	ar rcs $@ $^

libwayland-protocols.a: $(WL_OBJ)
	ar rcs $@ $^

ifneq ($(OS),Windows_NT)
LIB_WL_PROTOCOLS := libwayland-protocols.a
else
LIB_WL_PROTOCOLS :=
endif
game$(EXE): $(OBJ_GAME) libengine.a $(LIB_WL_PROTOCOLS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@
	@chmod +x $@

shader.spv: shader.slang
	slangc $< -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name -entry vertMain -entry fragMain -o $@

-include $(DEP_ENGN)
.build/Engine/%.o: Engine/%.cpp $(WL_HEADERS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_LIBS) -IEngine/$(UI) -MMD -MP -c $< -o $@

-include $(DEP_GAME)
.build/Game/%.o: Game/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -IGame -MMD -MP -c $< -o $@

.wayland/%.h: $(WL_DIR)/%.xml
	@mkdir -p $(dir $@)
	wayland-scanner client-header $< $@

.wayland/%.c: $(WL_DIR)/%.xml $(WL_HEADERS)
	@mkdir -p $(dir $@)
	wayland-scanner private-code $< $@

.wayland/%.o: .wayland/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $< -o $@
