#
#  dma.mak
#
#  DMA support for MHDDK
#
#  $DateTime: 2006/10/24 23:40:45 $ 
#

VPATH += \
    $(DMA_LIBRARY_DIR)

CXX_INCLUDES += \
    $(INCLUDE_FLAG)$(DMA_LIBRARY_DIR)/..

OBJECTS += \
    $(BUILD_DIR)/tDMAChannel$(OBJ_SUFFIX) \
    $(BUILD_DIR)/tLinearDMABuffer$(OBJ_SUFFIX) \
    $(BUILD_DIR)/tDMABuffer$(OBJ_SUFFIX) \
    $(BUILD_DIR)/tMITE$(OBJ_SUFFIX)
