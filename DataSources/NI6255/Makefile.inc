OBJSX=NI6255.x NI6255Module.x

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
	$(BUILD_DIR)/NI6255$(DLLEXT)
	    echo  $(OBJS)

include $(MAKEDEFAULTDIR)/MakeStdLibRules.$(TARGET)

LIBRARIES += module/nimseries/osiBus.o 
LIBRARIES += module/nimseries/osiUserCode.o 
LIBRARIES += module/nimseries/tTIO.o 
LIBRARIES += module/nimseries/tMSeries.o
LIBRARIES += module/nimseries/ai.o 
LIBRARIES += module/nimseries/ao.o 
LIBRARIES += module/nimseries/common.o 
LIBRARIES += module/nimseries/scale.o
LIBRARIES += module/nimseries/tDMABuffer.o
LIBRARIES += module/nimseries/tDMAChannel.o
LIBRARIES += module/nimseries/tLinearDMABuffer.o
LIBRARIES +=module/nimseries/tMITE.o


INCLUDES += -I module/nimhddk_linux26
INCLUDES += -I module/nimhddk_LinuxDevMem
INCLUDES += -I module/nimseries/ChipObjects/
INCLUDES += -I module/nimseries/Examples/
INCLUDES += -I module/nimhddk_dma/



