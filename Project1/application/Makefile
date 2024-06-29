export IMPERAS_ARCH=Linux32
ifndef IMPERAS_HOME
  IMPERAS_ERROR := $(error "IMPERAS_HOME not defined, please setup Imperas/OVP environment")
endif
IMPERAS_HOME := $(shell getpath.exe "$(IMPERAS_HOME)")

# Build locally without using a VLNV library
NOVLNV=1

OPTIMISATION?=-O0

CROSS=RISCV32
SRC?=application.c #application file
SRCINC?=

INCOBJ=$(patsubst %.c,%.$(CROSS).o,$(SRCINC))
EXE=$(patsubst %.c,%.$(CROSS).elf,$(SRC))

-include $(IMPERAS_HOME)/lib/$(IMPERAS_ARCH)/CrossCompiler/$(CROSS).makefile.include
ifeq ($($(CROSS)_CC),)
    IMPERAS_ERROR := $(error "Error : $(CROSS)_CC not set. Please check installation of toolchain for $(CROSS)")
endif

all: $(EXE)

%.$(CROSS).elf: %.$(CROSS).o $(INCOBJ) trap.$(CROSS).o
	@    echo "# Linking $(CROSS) $@"
	$(V) $(IMPERAS_LINK) -o $@ $^ $(IMPERAS_LDFLAGS) -lm

%.$(CROSS).o: %.c
	@    echo "# Compiling $(CROSS) $<"
	$(V) $(IMPERAS_CC) -g -c -o $@ $< -D$(CROSS) $(OPTIMISATION)

%.$(CROSS).o: %.S
	$(V) echo "Assembling Application $@"
	$(V) $($(CROSS)_AS) -o $@  $<

clean::
	- rm -f *.$(CROSS).elf *.$(CROSS).o
