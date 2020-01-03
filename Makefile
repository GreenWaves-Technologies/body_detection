# Copyright (C) 2017 GreenWaves Technologies
# All rights reserved.

# This software may be modified and distributed under the terms
# of the BSD license.  See the LICENSE file for details.

ifndef GAP_SDK_HOME
  $(error Source sourceme in gap_sdk first)
endif

MODEL_PREFIX=body_detection

ifndef QUANTIZATION_BITS
  QUANTIZATION_BITS=16
endif

$(info Building GAP8 mode with $(QUANTIZATION_BITS) bit quantization)

# For debugging don't load an image
# Run the network with zeros
#NO_IMAGE=1

# The training of the model is slightly different depending on
# the quantization. This is because in 8 bit mode we used signed
# 8 bit so the input to the model needs to be shifted 1 bit
ifeq ($(QUANTIZATION_BITS),8)
  GAP_FLAGS += -DQUANTIZATION_8BIT
  NNTOOL_SCRIPT=model/nntool_script8
  MODEL_SUFFIX = _8BIT
else
  ifeq ($(QUANTIZATION_BITS),16)
    GAP_FLAGS += -DQUANTIZATION_16BIT
    NNTOOL_SCRIPT=model/nntool_script16
    MODEL_SUFFIX = _16BIT
  else
    $(error Don't know how to build with this bit width)
  endif
endif

include model_decl.mk

# Here we set the memory allocation for the generated kernels
# REMEMBER THAT THE L1 MEMORY ALLOCATION MUST INCLUDE SPACE
# FOR ALLOCATED STACKS!

# hram - HyperBus RAM
# qspiram - Quad SPI RAM
MODEL_L3_EXEC=hram
# hflash - HyperBus Flash
# qpsiflash - Quad SPI Flash
MODEL_L3_CONST=hflash

pulpChip = GAP
APP = body_detection
USE_PMSIS_BSP=1

PULP_APP_SRCS += main.c ImgIO.c ImageDraw.c SSDKernels.c SSDBasicKernels.c SSDParams.c $(MODEL_SRCS)

GAP_FLAGS += -O2 -s -mno-memcpy -fno-tree-loop-distribute-patterns 
GAP_FLAGS += -I. -I./helpers -I$(TILER_EMU_INC) -I$(TILER_INC) -I$(GEN_PATH) -I$(MODEL_BUILD)

ifeq ($(platform),gvsoc)
  $(info Platform is GVSOC)
  override runner_args += --config-opt=flash/fs/files=$(realpath $(MODEL_TENSORS))
  GAP_FLAGS += -DNO_BRIDGE
else
  $(info Platform is GAPUINO)
  PLPBRIDGE_FLAGS = -f $(MODEL_TENSORS)
  PLPBRIDGE_FLAGS += -fileIO 5
endif

#####Here we add cutom kernels that are not yet integrated into AT libraries

SSD_MODEL_GEN = SSDKernels
SSD_MODEL_GEN_C = $(addsuffix .c, $(SSD_MODEL_GEN))
SSD_MODEL_GEN_CLEAN = $(SSD_MODEL_GEN_C) $(addsuffix .h, $(SSD_MODEL_GEN))

GenSSDTile: SSDModel.c
	gcc -g -o GenSSDTile -I"$(TILER_INC)" SSDModel.c $(TILER_LIB)

$(SSD_MODEL_GEN_C): GenSSDTile
	./GenSSDTile

SSD_model: $(SSD_MODEL_GEN_C)
	cd SSDParamsGenerator && $(MAKE) all run

#################


# all depends on the model
all:: SSD_model model

clean:: #clean_model
	rm -rf BUILD
	cd SSDParamsGenerator && $(MAKE) clean
	rm -rf SSDParams.c SSDParams.h
	rm -rf GenSSDTile $(SSD_MODEL_GEN_CLEAN)

clean_all: clean clean_model #clean_train


.PHONY: clean_all

include model_rules.mk
include $(RULES_DIR)/pmsis_rules.mk
