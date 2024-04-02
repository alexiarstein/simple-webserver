CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -I/opt/homebrew/include/ -L/opt/homebrew/lib
LDFLAGS = -lssl -lcrypto
SRC_DIR = src
SRC = $(SRC_DIR)/lexiserver.c
TARGET = lexiserver

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)

