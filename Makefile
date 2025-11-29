.PHONY: build all clean

default: all

CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lX11 -lXext

TARGET = crosshair
SRC = crosshair.c

default: all

build:
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm $(TARGET)

all: build

