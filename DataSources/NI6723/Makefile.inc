OBJSX=NI6723.x NI6723Module.x

PACKAGE=Components/DataSources

ROOT_DIR=../../
MAKEDEFAULTDIR=$(MARTe2_DIR)/MakeDefaults
include $(MAKEDEFAULTDIR)/MakeStdLibDefs.$(TARGET)


MAKEDEFAULTDIR=$(MARTe2_DIR)/MakeDefaults

include $(MAKEDEFAULTDIR)/MakeStdLibDefs.$(TARGET)

INCLUDES += -I.
INCLUDES += -I$(MDSPLUS_DIR)/include/
INCLUDES += -I$(MARTe2_DIR)/Source/Core/BareMetal/L0Types
INCLUDES += -I$(MARTe2_DIR)/Source/Core/BareMetal/L1Portability
INCLUDES += -I$(MARTe2_DIR)/Source/Core/BareMetal/L2Objects
INCLUDES += -I$(MARTe2_DIR)/Source/Core/BareMetal/L3Streams
INCLUDES += -I$(MARTe2_DIR)/Source/Core/BareMetal/L4Messages
INCLUDES += -I$(MARTe2_DIR)/Source/Core/BareMetal/L4Configuration
INCLUDES += -I$(MARTe2_DIR)/Source/Core/BareMetal/L5GAMs
INCLUDES += -I$(MARTe2_DIR)/Source/Core/Scheduler/L1Portability
INCLUDES += -I$(MARTe2_DIR)/Source/Core/Scheduler/L3Services
INCLUDES += -I$(MARTe2_DIR)/Source/Core/Scheduler/L4Messages
INCLUDES += -I$(MARTe2_DIR)/Source/Core/Scheduler/L5GAMs
INCLUDES += -I$(MARTe2_DIR)/Source/Core/FileSystem/L1Portability
INCLUDES += -I$(MARTe2_DIR)/Source/Core/FileSystem/L3Streams

#TODO Temporary to fix problem in mdsplus include
CPPFLAGS += -Wno-error=sign-compare 

all: $(OBJS) $(SUBPROJ) \
	$(BUILD_DIR)/NI6723$(DLLEXT)
	    echo  $(OBJS)

include $(MAKEDEFAULTDIR)/MakeStdLibRules.$(TARGET)


LIBRARIES += module/ni6723/osiBus.o 
LIBRARIES += module/ni6723/osiUserCode.o 
LIBRARIES += module/ni6723/tSTC.o
LIBRARIES += module/ni6723/t6723.o
LIBRARIES += module/ni6723/initialize6723.o
LIBRARIES += module/ni6723/fpga6723.o
LIBRARIES += module/ni6723/FPGA_6723.o
LIBRARIES += module/ni6723/ao.o
LIBRARIES += module/ni6723/common.o

INCLUDES += -I module/nimhddk_linux26
INCLUDES += -I module/ni6723/ChipObjects
INCLUDES += -I module/ni6723/Examples

 

INCLUDES += -I module/nimhddk_LinuxDevMem
INCLUDES += -I module/nimseries/ChipObjects/
INCLUDES += -I module/nimseries/Examples/
INCLUDES += -I module/nimhddk_dma/
