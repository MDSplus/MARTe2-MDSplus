#
# nimseries/Makefile
#
# $DateTime: 2006/10/26 17:39:13 $
#

#
# Root source directory
#
EXAMPLE_SOURCE_DIR := .

#
# Directory where the examples and objects are built
# 
BUILD_DIR := .

#
#  DMA Library location
#
DMA_LIBRARY_DIR := ../nimhddk_dma

#
#  OS Interface location
#
#OSINTERFACE_DIR := ../nimhddk_visa
#OSINTERFACE_MAKEFILE := win-visa.mak
#OSINTERFACE_DIR := ../nimhddk_LinuxDevMem
#OSINTERFACE_MAKEFILE := LinuxDevMem.mak
OSINTERFACE_DIR := ../nimhddk_linux26
OSINTERFACE_MAKEFILE := linux26.mak

#
#  Define include directories, objects and targets
#

VPATH += \
    $(EXAMPLE_SOURCE_DIR)/ChipObjects \
    $(EXAMPLE_SOURCE_DIR)/Examples \

CXX_INCLUDES += -g \
    $(INCLUDE_FLAG)$(EXAMPLE_SOURCE_DIR)/ChipObjects \
    $(INCLUDE_FLAG)$(EXAMPLE_SOURCE_DIR)/Examples \
    $(INCLUDE_FLAG)/usr/local/mdsplus/include

OBJECTS += \
    $(BUILD_DIR)/osiBus$(OBJ_SUFFIX) \
    $(BUILD_DIR)/osiUserCode$(OBJ_SUFFIX) \
    $(BUILD_DIR)/tTIO$(OBJ_SUFFIX) \
    $(BUILD_DIR)/tMSeries$(OBJ_SUFFIX) \
    $(BUILD_DIR)/ai$(OBJ_SUFFIX) \
    $(BUILD_DIR)/ao$(OBJ_SUFFIX)\
    $(BUILD_DIR)/common$(OBJ_SUFFIX)\
    $(BUILD_DIR)/scale$(OBJ_SUFFIX)\
    $(BUILD_DIR)/main$(OBJ_SUFFIX)

TARGETS += \
    $(BUILD_DIR)/aiex1$(PRG_SUFFIX) \
	$(BUILD_DIR)/aiex1a$(PRG_SUFFIX) \
	$(BUILD_DIR)/aiex1b$(PRG_SUFFIX) \
    $(BUILD_DIR)/aiex2$(PRG_SUFFIX) \
    $(BUILD_DIR)/aoex1$(PRG_SUFFIX) \
    $(BUILD_DIR)/aoex1a$(PRG_SUFFIX) \
    $(BUILD_DIR)/aoex2$(PRG_SUFFIX) \
    $(BUILD_DIR)/aoex2a$(PRG_SUFFIX) \
    $(BUILD_DIR)/aoex3$(PRG_SUFFIX) \
    $(BUILD_DIR)/aoex4$(PRG_SUFFIX) \
    $(BUILD_DIR)/dioex1$(PRG_SUFFIX) \
    $(BUILD_DIR)/dioex2$(PRG_SUFFIX) \
    $(BUILD_DIR)/gpctex1$(PRG_SUFFIX) \
    $(BUILD_DIR)/gpctex1a$(PRG_SUFFIX) \
    $(BUILD_DIR)/gpctex2$(PRG_SUFFIX) \
    $(BUILD_DIR)/gpctex3$(PRG_SUFFIX) \
    $(BUILD_DIR)/serialNumber$(PRG_SUFFIX)

#
# DMA Support
#
#    include only if the dma library path is defined
#
ifneq (,$(strip $(DMA_LIBRARY_DIR)))

    #
    # add DMA library objects
    #
    include $(DMA_LIBRARY_DIR)/dma.mak

    #
    # examples that use DMA
    #
    TARGETS += \
	$(BUILD_DIR)/aiex1c$(PRG_SUFFIX) \
	$(BUILD_DIR)/aiex1d$(PRG_SUFFIX) \
        $(BUILD_DIR)/aiex3$(PRG_SUFFIX) \
		$(BUILD_DIR)/aiex3a$(PRG_SUFFIX) \
        $(BUILD_DIR)/aiex4$(PRG_SUFFIX) \
		$(BUILD_DIR)/aiex5$(PRG_SUFFIX) \
        $(BUILD_DIR)/aoex5$(PRG_SUFFIX) 

endif

#
#  OS specific rules and variables
#
include $(OSINTERFACE_DIR)/$(OSINTERFACE_MAKEFILE)
