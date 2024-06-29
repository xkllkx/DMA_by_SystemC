# Build the TLM support archive (tlm.a)

IMPERAS_HOME := $(shell getpath.exe "$(IMPERAS_HOME)")
ifndef IMPERAS_HOME
    IMPERAS_ERROR := $(error "IMPERAS_HOME not defined")
endif
include $(IMPERAS_HOME)/bin/Makefile.include

############################ YOUR SYSTEMC  ####################################
# Set this to your SystemC and installation
# SYSTEMC_HOME   = 

## If SYSTEMC is set and SYSTEMC_HOME is not then use SYSTEMC
ifeq ($(SYSTEMC_HOME),)
  ifneq ($(SYSTEMC),)
    SYSTEMC_HOME=$(SYSTEMC)
  endif
endif

ifndef SYSTEMC_HOME
  SYSTEMC_HOME := $(error "SYSTEMC_HOME must be defined for SystemC install directory")
endif

############################# INPUT PARAMETERS ################################

ifeq ($(VERBOSE),1)
  V=
else
  V=@
endif

########################### END INPUT PARAMETERS ##############################

TLMSUPPORTSRC = $(wildcard *.cpp)
LIB           = tlm.a

TLM_CFLAGS  = -Wno-long-long \
              -DSC_INCLUDE_DYNAMIC_PROCESSES \
              -D_CRT_SECURE_NO_WARNINGS \
              -D_CRT_SECURE_NO_DEPRECATE

TLM_CFLAGS  += $(SIM_CFLAGS)  $(SIM_CXXFLAGS)
TLM_LDFLAGS += $(SIM_LDFLAGS) $(SIM_LDXXFLAGS)

TLM_CFLAGS += -I$(SYSTEMC_HOME)/include

#
# Build location for intermediate files
#
BUILDROOT = Build
OBJD  = $(BUILDROOT)/$(IMPERAS_ARCH)/usr

OBJ   = $(sort $(patsubst %.cpp, $(OBJD)/%.o, $(TLMSUPPORTSRC)))
NM    = $(patsubst %.a, %.nm.out, $(LIB))

all: $(LIB) $(NM)
	@    echo "# TLM Support Library $(LIB)"

$(LIB): $(OBJ)
	@    echo "# TLM Support Archive $@"
	$(V) $(HOST_AR) r $@ $^

$(NM) :  $(LIB)
	@    echo "# TLM Support NM $@"
	$(V) $(HOST_NM) --demangle $< > $@

$(OBJD)/%.o: %.cpp
	@    echo "# TLM Support Compiling $@"
	$(V) mkdir -p $(@D)
	$(V) $(HOST_GXX) -c -o $@ $< $(TLM_CFLAGS) $(CFLAGS)

clean:
	- rm -f $(LIB) $(OBJ) $(NM)
