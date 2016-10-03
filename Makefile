TARGET = tcpardu
CC       = gcc
SRCDIR   = src
INCDIR   = inc
OBJDIR   = obj

# compiling flags here
CFLAGS   = -Wall -I$(INCDIR)

LINKER   = gcc -o
# linking flags here
LFLAGS   = -Wall -I$(INCDIR) -lm -lpaho-mqtt3c


SOURCES  := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(INCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
rm       = rm -f


$(TARGET): $(OBJECTS)
	@$(LINKER) $@ $(LFLAGS) $(OBJECTS)
	@echo "Linking complete!"

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

.PHONEY: clean
clean:
	@$(rm) $(OBJECTS)
	@echo "Cleanup complete!"
