# Makefile for building embedded application.
# by Brian Fraser

# Edit this file to compile extra C files into their own programs.
TARGETS1= sampler
TARGETS2= noworky


PUBDIR = $(HOME)/cmpt433/public/myApps
OUTDIR = $(PUBDIR)
CROSS_TOOL = arm-linux-gnueabihf-
CC_CPP = $(CROSS_TOOL)g++
CC_C = $(CROSS_TOOL)gcc

CFLAGS = -Wall -g -std=c99 -D _POSIX_C_SOURCE=200809L -Werror 
# -pg for supporting gprof profiling.
CFLAGS += -pg



# Convert the targets to full-path executables to delete.
OUTPUTS1= $(addprefix $(OUTDIR)/, $(TARGETS1))
OUTPUTS2= $(addprefix $(OUTDIR)/, $(TARGETS2))

all: $(TARGETS1) $(TARGETS2)

$(TARGETS1):
	$(CC_C) $(CFLAGS) joystickRead.c matrixDisplay.c sampler.c -o $(OUTDIR)/$(TARGETS1) -lpthread

$(TARGETS2):
	$(CC_C) $(CFLAGS) noworky.c -o $(OUTDIR)/$(TARGETS2)

clean:
	rm -f $(OUTPUTS1) $(OUTPUTS2)