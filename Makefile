TARGET = tcpardu
LIBS = -lm
IDIR = inc
CC = gcc
CFLAGS = -g -Wall -I$(IDIR)
ODIR=out

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard src/*.c))
HEADERS = $(wildcard inc/*.h)

$(ODIR)/%.o: src/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o out/$@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	-rm -f $(ODIR)/*.o
	-rm -f $(TARGET)
