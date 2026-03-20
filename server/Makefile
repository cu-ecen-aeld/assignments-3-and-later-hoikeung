CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -Wall -Werror -g
LDFLAGS ?= -lpthread -lrt
TARGET ?= aesdsocket

default: $(TARGET)
all: $(TARGET)

$(TARGET): aesdsocket.c
	$(CC) $(CFLAGS) $^ -o $@ $(INCLUDES) $(LDFLAGS)

clean:
	rm -f $(TARGET) *.o
