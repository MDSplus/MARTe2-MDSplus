#
#    LinuxDevMem.mak
#        
#        OS specific definitions for linux using /dev/mem interface
#
#    $DateTime: 2008/08/20 11:09:56 $
#
OBJ_SUFFIX := .o
PRG_SUFFIX := 

#
#  Compiler variales
#

CXX := g++
INCLUDE_FLAG := -I 
CXX_OUTPUT_FLAG:= -o

CXXFLAGS += -c -DkLittleEndian=1 -DkGNU=1
CXX_INCLUDES +=

#
#  Linker variables
#

LD := g++
LD_OUTPUT_FLAG := -o

LDFLAGS += 
LD_LIBRARIES +=

#
#  Add OS specific implementation source dirirectory to VPATH
#

VPATH += \
    $(OSINTERFACE_DIR) \
    $(OSINTERFACE_DIR)/LinuxDevMem

CXX_INCLUDES += \
    $(INCLUDE_FLAG)$(OSINTERFACE_DIR)

OBJECTS += \
    $(BUILD_DIR)/osiBus$(OBJ_SUFFIX) \
    $(BUILD_DIR)/osiUserCode$(OBJ_SUFFIX)

#
#    Rules
#

all: $(TARGETS)

$(TARGETS) : $(BUILD_DIR)/%$(PRG_SUFFIX) : $(BUILD_DIR)/%$(OBJ_SUFFIX) $(OBJECTS)
	$(LD) $(LDFLAGS) $(LD_LIBRARIES) $^ $(LD_OUTPUT_FLAG)$@

$(BUILD_DIR)/%$(OBJ_SUFFIX) : %.cpp
	$(CXX) $(CXXFLAGS) $(CXX_INCLUDES) $(CXX_OUTPUT_FLAG)$@ $<

.PHONY: clean
clean:
	$(RM) $(TARGETS)
	$(RM) $(foreach target,$(TARGETS),$(subst $(PRG_SUFFIX),$(OBJ_SUFFIX),$(target)))
	$(RM) $(OBJECTS)

#
#    End of linux26.mak
#
