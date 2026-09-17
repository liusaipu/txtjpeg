# Makefile for TxtJPEG-C
# Works on Linux, macOS, and MSYS2/MinGW

CC      ?= gcc
CFLAGS  ?= -O3 -Wall -Wextra -std=c99
LDFLAGS ?= -ljpeg -lm

SRCS = main.c parser.c format.c shape.c normalize.c \
       jpeg_codec.c container.c compress.c decompress.c
OBJS = $(SRCS:.c=.o)

TARGET = txtjpeg

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c txtjpeg.h compat.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
