# Makefile for Geekstyles v2 Nixie Clock Firmware
# Using SDCC (Small Device C Compiler)

MCU = stm8s003f3
CC = sdcc
CFLAGS = -mstm8 --std-sdcc99 --std-c99 --opt-code-speed -ISTM8_headers/include
LDFLAGS = -mstm8

SRC = real_clock.c ds3231.c i2c.c
OBJ = $(SRC:.c=.rel)
TARGET = real_clock.ihx

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

%.rel: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.rel *.lst *.rst *.sym *.asm *.ihx *.map *.lk *.cdb

.PHONY: all clean
