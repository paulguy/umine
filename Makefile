OBJS   = libmine.o test.o
TARGET = umine
CFLAGS = -W -Wall -Wextra -pedantic -ggdb
LDFLAGS = -ggdb

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJS)

all: $(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: clean
