CC      = gcc
CFLAGS  = -Wall -Wextra -pedantic -std=c11 -g
TARGET  = upa
SRC     = upa.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
