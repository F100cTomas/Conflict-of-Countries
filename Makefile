UI := TerminalUI
BUILD := debug
CXX := clang++
CC := clang

CXXFLAGS_BASE := -std=c++20 -Iinclude
ifneq ($(OS),Windows_NT)
WAYLAND_PROTOCOL_DIR := $(shell pkg-config --variable=pkgdatadir wayland-protocols)
WAYLAND_PROTOCOLS := $(shell find $(WAYLAND_PROTOCOL_DIR) -name "*.xml")
WAYLAND_SOURCE := $(WAYLAND_PROTOCOLS:$(WAYLAND_PROTOCOL_DIR)/%.xml=.wayland/%.c)
WAYLAND_HEADERS := $(WAYLAND_SOURCE:.c=.h)
WAYLAND_OBJ := $(WAYLAND_SOURCE:.c=.o)
LIB_WAYLAND := libwayland-protocols.a
CXXFLAGS_LIBS := -I.wayland $(shell pkg-config --cflags wayland-client wayland-cursor vulkan)
LIBS := $(shell pkg-config --libs wayland-client wayland-cursor vulkan)
else
WAYLAND_PROTOCOL_DIR :=
WAYLAND_SOURCE :=
WAYLAND_HEADERS :=
WAYLAND_OBJ :=
CXXFLAGS_LIBS :=
LIBS := -lkernel32 -luser32 -lgdi32
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
	./game

all: libengine.a game

clean:
	$(RM) -r .build libengine.a game

clean-wayland:
	$(RM) -r .wayland libwayland-protocols.a

libengine.a: $(OBJ_ENGN)
	ar rcs $@ $^

libwayland-protocols.a: $(WAYLAND_OBJ)
	ar rcs $@ $^

ifneq ($(OS),Windows_NT)
LIB_WAYLAND_PROTOCOLS := libwayland-protocols.a
else
LIB_WAYLAND_PROTOCOLS :=
endif
game: libengine.a $(LIB_WAYLAND_PROTOCOLS) $(OBJ_GAME)
	$(CXX) $(CXXFLAGS) $(LIBS) $^ -o $@
	@chmod +x $@

-include $(DEP_ENGN)
.build/Engine/%.o: Engine/%.cpp $(WAYLAND_HEADERS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_LIBS) -IEngine/$(UI) -MMD -MP -c $< -o $@

-include $(DEP_GAME)
.build/Game/%.o: Game/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -IGame -MMD -MP -c $< -o $@

.wayland/%.h: $(WAYLAND_PROTOCOL_DIR)/%.xml
	@mkdir -p $(dir $@)
	wayland-scanner client-header $< $@

.wayland/%.c: $(WAYLAND_PROTOCOL_DIR)/%.xml $(WAYLAND_HEADERS)
	@mkdir -p $(dir $@)
	wayland-scanner private-code $< $@

.wayland/%.o: .wayland/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $< -o $@
