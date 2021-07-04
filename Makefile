ifndef OS

C = gcc
CFLAGS = -Wall -Wextra -O2 -lm -lSDL2 --std=c99 -no-pie
TCFLAGS = -Wall -Wextra -O2 -lm --std=c99 -no-pie

BUILD__ = $(C) digitus.c $(CFLAGS) -o digitus && chmod +x ./digitus
BUILD_TOOLS_TILEMAKER = $(C) ./tools/tilemaker.c $(TCFLAGS) -o ./tools/tilemaker && chmod +x ./tools/tilemaker
BUILD_TOOLS_MAPMAKER = $(C) ./tools/mapmaker.c $(TCFLAGS) -o ./tools/mapmaker && chmod +x ./tools/mapmaker
BUILD_TOOLS_MAPPACKER = $(C) ./tools/mappacker.c $(TCFLAGS) -o ./tools/mappacker && chmod +x ./tools/mappacker

.PHONY: tools

all: clean build run

build:
	$(BUILD__)

build_tools: tools

tools:
	mkdir -p tools/
	$(BUILD_TOOLS_TILEMAKER); $(BUILD_TOOLS_MAPMAKER); $(BUILD_TOOLS_MAPPACKER)

run:
	./digitus

clean_all: clean clean_tools

clean:
	rm -f ./digitus

clean_tools:
	rm -f ./tools/tilemaker ./tools/mapmaker ./tools/mappacker

else



endif
