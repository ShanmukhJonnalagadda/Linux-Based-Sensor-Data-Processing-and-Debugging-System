CC := gcc
CFLAGS := -std=c11 -O0 -g3 -Wall -Wextra -Wpedantic -Werror
SOURCE := src/sensor_monitor.c

.PHONY: all buggy test clean
all: build/sensor_monitor

build:
	mkdir -p build

build/sensor_monitor: $(SOURCE) | build
	$(CC) $(CFLAGS) $(SOURCE) -o $@

buggy: | build
	$(CC) $(CFLAGS) -DINTRODUCE_BUG $(SOURCE) -o build/sensor_monitor_buggy

test: all
	./build/sensor_monitor samples/frames.txt > build/actual.txt
	diff -u samples/expected.txt build/actual.txt
	@echo "PASS: sample-frame validation"

clean:
	rm -rf build
