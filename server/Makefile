CC = $(CROSS_COMPILE)gcc
CFLAGS ?= -Wall -Wextra -Werror
CFLAGS += -pthread
USE_AESD_CHAR_DEVICE ?= 1
CFLAGS += -DUSE_AESD_CHAR_DEVICE=$(USE_AESD_CHAR_DEVICE)
LDFLAGS ?=
LDFLAGS += -pthread

TARGET := aesdsocket
SOURCES := aesdsocket.c
OBJECTS := $(SOURCES:.c=.o)

.PHONY: all default clean

default: all

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) $(TARGET) $(OBJECTS)
