#
#    linux26.mak
#        
#        OS specific definitions for linux using Native 2.6 driver - nirlpk
#
#        nirlpk is a native 2.6 kernel driver included in this ddk
#        (osinterface/Linux26/nirlpk)
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

CXXFLAGS += -c -DkLittleEndian=1 -DkGNU=1 -fPIC
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
    $(OSINTERFACE_DIR)/Linux26

CXX_INCLUDES += \
    $(INCLUDE_FLAG)$(OSINTERFACE_DIR)

OBJECTS += \
    $(BUILD_DIR)/osiBus$(OBJ_SUFFIX) \
    $(BUILD_DIR)/osiUserCode$(OBJ_SUFFIX)

LSDAQ_TARGET := $(BUILD_DIR)/lsdaq

#
#    Rules
#

.PHONY: all
all: $(TARGETS) $(LSDAQ_TARGET)

$(TARGETS) : $(BUILD_DIR)/%$(PRG_SUFFIX) : $(BUILD_DIR)/%$(OBJ_SUFFIX) $(OBJECTS)
	$(LD) $(LDFLAGS) $(LD_LIBRARIES) $^ $(LD_OUTPUT_FLAG)$@

$(BUILD_DIR)/%$(OBJ_SUFFIX) : %.cpp
	$(CXX) $(CXXFLAGS) $(CXX_INCLUDES) $(CXX_OUTPUT_FLAG)$@ $<

.PHONY: clean
clean:
	$(RM) $(TARGETS)
	$(RM) $(foreach target,$(TARGETS),$(subst $(PRG_SUFFIX),$(OBJ_SUFFIX),$(target)))
	$(RM) $(OBJECTS)
	$(RM) $(LSDAQ_TARGET)

$(LSDAQ_TARGET) : $(OSINTERFACE_DIR)/Linux26/nirlpk/lsdaq
	cp $^ $@

#
#    End of linux26.mak
#
