#
# ni6723/Makefile
#
# $DateTime: 2008/02/07 15:10:08 $
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
DMA_LIBRARY_DIR := ../dma

#
#  OS Interface location
#
OSINTERFACE_DIR := ../nimhddk_linux26
OSINTERFACE_MAKEFILE := linux26.mak

#
#  Define include directories, objects and targets
#
VPATH += \
    $(EXAMPLE_SOURCE_DIR)/ChipObjects \
    $(EXAMPLE_SOURCE_DIR)/Examples \
    $(EXAMPLE_SOURCE_DIR)/Images \

 
CXX_INCLUDES += \
    $(INCLUDE_FLAG)$(EXAMPLE_SOURCE_DIR)/ChipObjects \
    $(INCLUDE_FLAG)$(EXAMPLE_SOURCE_DIR)/Examples

OBJECTS += \
    $(BUILD_DIR)/osiBus$(OBJ_SUFFIX) \
    $(BUILD_DIR)/osiUserCode$(OBJ_SUFFIX) \
    $(BUILD_DIR)/tSTC$(OBJ_SUFFIX) \
    $(BUILD_DIR)/t6723$(OBJ_SUFFIX) \
    $(BUILD_DIR)/ao$(OBJ_SUFFIX)\
    $(BUILD_DIR)/common$(OBJ_SUFFIX)\
    $(BUILD_DIR)/main$(OBJ_SUFFIX) \
    $(BUILD_DIR)/fpga6723$(OBJ_SUFFIX) \
    $(BUILD_DIR)/FPGA_6723$(OBJ_SUFFIX) \

TARGETS += \
    $(BUILD_DIR)/aoex1$(PRG_SUFFIX) \
    $(BUILD_DIR)/aoex2$(PRG_SUFFIX) \
    $(BUILD_DIR)/aoex3$(PRG_SUFFIX) \
    $(BUILD_DIR)/aoex4$(PRG_SUFFIX) \
    $(BUILD_DIR)/aoex5$(PRG_SUFFIX) \
    $(BUILD_DIR)/initialize6723$(PRG_SUFFIX)

#
# DMA Support
#
#    include only if the dma library path is defined
#
ifneq (,$(strip $(DMA_LIBRARY_DIR)))

    #
    # add DMA library objects
    #
    #include $(DMA_LIBRARY_DIR)/dma.mak

    #
    # examples that use DMA
    #
    #TARGETS += \
    #    $(BUILD_DIR)/aiex2$(PRG_SUFFIX)

endif

#
#  OS specific rules and variables
#
include $(OSINTERFACE_DIR)/$(OSINTERFACE_MAKEFILE)
