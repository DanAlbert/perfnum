EXE = compute

SHELL = sh
CC = gcc
REMOVE = rm -f
REMOVEDIR = rm -rf

SRC =	compute.c \
		packets.c \
		shmem.c \
		sock.c \

DEBUG = -ggdb
OPTIMIZATION = 
INCLUDEDIRS = 
OBJDIR = obj

CFLAGS =	$(INCLUDEDIRS) \
			-Wall \
			-Wextra \
			-Wmissing-prototypes \
			-Wmissing-declarations \
			-Wstrict-prototypes \
			-std=gnu99 \
			$(OPTIMIZATION) \
			$(DEBUG) \

LDFLAGS =	-lrt \

# Compiler flags to generate dependency files.
GENDEPFLAGS = -MMD -MP -MF .dep/$(@F).d

# Combine all necessary flags and optional flags.
ALL_CFLAGS = $(CFLAGS) $(GENDEPFLAGS)

OBJ = $(SRC:%.c=$(OBJDIR)/%.o)

all: $(EXE)

# Link object files to executable
$(EXE): $(OBJ)
	@echo
	@echo Linking: $@
	$(CC) -o $@ $(ALL_CFLAGS) $(LDFLAGS) $^

# Compile: create object files from C source files.
$(OBJDIR)/%.o : %.c
	@echo Compiling: $<
	$(CC) -c $(ALL_CFLAGS) $< -o $@ 

doc:
	doxygen

clean:
	$(REMOVE) $(EXE)
	$(REMOVE) $(SRC:%.c=$(OBJDIR)/%.o)
	$(REMOVE) $(SRC:.c=.d)
	$(REMOVEDIR) .dep
	$(REMOVEDIR) $(OBJDIR)

# Create object files directory
$(shell mkdir $(OBJDIR) 2>/dev/null)

# Include the dependency files.
-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)

# Listing of phony targets.
.PHONY : clean
