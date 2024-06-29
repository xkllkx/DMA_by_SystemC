#
# Build the OP C++ API library archive (*.a) and/or
# shared object (*.so on Linux, *.dll on Windows)
#
# - use target "all" to build both shared and archive libraries
# - use target "shared" to build just the shared library (.so/.dll)
# - use target "archive" to build just the archive library (.a)
# - alternativey just use the full path of the library to be built
# - if the library name is not OP++ then set LIBNAME to the 
#   target library name (without the leading "lib" or a suffix) 
#

IMPERAS_HOME := $(shell getpath.exe "$(IMPERAS_HOME)")
ifndef IMPERAS_HOME
    IMPERAS_ERROR := $(error "IMPERAS_HOME not defined")
endif
include $(IMPERAS_HOME)/bin/Makefile.include

OPCPPSRC  = $(wildcard *.cpp)
LIBNAME  ?= OP++
SO        = lib$(LIBNAME).$(IMPERAS_SHRSUF)
LIB       = lib$(LIBNAME).a

ifeq ($(IMPERAS_UNAME),Linux)
CXXCFLAGS += -rdynamic -fno-aggressive-loop-optimizations \
             -fwrapv -fno-strict-overflow
CXXLDFLAGS += -Wl,--no-undefined
endif

ifeq ($(IMPERAS_UNAME),Windows)
CXXCFLAGS += -fwrapv -fno-strict-overflow 
# Not supported on some compilers
# -fno-aggressive-loop-optimizations 
CXXLDFLAGS += -static-libgcc \
              -Wl,--disable-auto-image-base \
              -Wl,-Bsymbolic
endif

#
# Build location for intermediate files
#
BUILDROOT = Build
OBJD      = $(BUILDROOT)/$(IMPERAS_ARCH)/usr

DEP = $(sort $(patsubst %.cpp, $(OBJD)/%.d, $(OPCPPSRC)))
OBJ = $(sort $(patsubst %.cpp, $(OBJD)/%.o, $(OPCPPSRC)))

archive: $(LIB)
	@    echo "# OP++: Created C++ Support archive library: $^"
shared: $(SO)
	@    echo "# OP++: Created C++ Support shared library: $^"
all: $(SO) $(LIB)
	@    echo "# OP++: Created C++ Support Libraries: $^"

#
# include all the dependancy requirements
#
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),realclean)
    $(foreach dep,$(DEP),$(eval -include $(dep)))
endif
endif

#
# Build rules
#
$(SO): $(OBJ)
	@    echo "# OP++: Linking $@"
	$(V) $(HOST_GXX) -o $@ --shared $^ $(SIM_LDFLAGS) $(CXXLDFLAGS)

$(LIB): $(OBJ)
	@    echo "# OP++: Building Support Archive $@"
	$(V) $(HOST_AR) r $@ $^ > /dev/null
	
$(OBJD)/%.o: %.cpp
	@    echo "# OP++: Compiling $@"
	$(V) mkdir -p $(@D)
	$(V) $(HOST_GXX) -c -o $@ $< $(SIM_CFLAGS) $(SIM_CXXFLAGS) $(CXXCFLAGS)

$(OBJD)/%.d: %.cpp
	@    echo "# OP++: Depending $@"
	$(V) mkdir -p $(@D)
	$(V) $(HOST_GXX) -MM $< $(SIM_CFLAGS) $(CFLAGS) -MT $(@D)/$*.o -MF $@

clean:
	- rm -f $(SO) $(LIB) $(OBJ) $(DEP)
ifeq ($(IMPERAS_UNAME),Windows)
	- rm -f lib$(LIBNAME).def lib$(LIBNAME).a
endif
realclean: clean
	- rm -rf $(OBJD) $(BUILDROOT)
