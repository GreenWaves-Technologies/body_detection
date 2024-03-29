# Copyright (C) 2017 GreenWaves Technologies
# All rights reserved.

# This software may be modified and distributed under the terms
# of the BSD license.  See the LICENSE file for details.


## Mute printf in source code
#SILENT=1

## Enable image grub from camera and disaply output to lcd
#FROM_CAMERA=1

## For GAP9_EVK, please connect ili9341 Adafruit lcd screen via 
## SPI bus to MikroE bus on the chip
## CS    <=>  CS0 (PI_PAD_034)
## MISO  <=>  MISO (PI_PAD_039)
## MOSI  <=>  MOSI (PI_PAD_038)
## SCK   <=>  SCK (PI_PAD_033)
## DC3V3 <=>  TX (PI_PAD_045)
## POWER <=>  3V3 (PI_PAD_035)
## GND   <=>  GND

## This is used for Jenkins test to use input form .h header file
# not used in this project
#NO_BRIDGE=1


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
# FC stack size:
MAIN_STACK_SIZE=5120
# Cluster PE0 stack size:
CLUSTER_STACK_SIZE=4096
# Cluster PE1-PE7 stack size:
CLUSTER_SLAVE_STACK_SIZE=1024
TOTAL_STACK_SIZE=$(shell expr $(CLUSTER_STACK_SIZE) \+ $(CLUSTER_SLAVE_STACK_SIZE) \* 8)
ifeq ('$(TARGET_CHIP_FAMILY)','GAP9')
  FREQ_CL?=400
  FREQ_FC?=400
  MODEL_L1_MEMORY=$(shell expr 125000 \- $(TOTAL_STACK_SIZE))  # L1 memory - 125KB
  MODEL_L2_MEMORY=200000 # L2 memory - 1.5MB
  MODEL_L3_MEMORY=8388608
  APP_CFLAGS += -DGAP9_EVK=1
else
  ifeq ('$(TARGET_CHIP)','GAP8_V3')
    FREQ_CL?=175
  else
    FREQ_CL?=50
  endif
  FREQ_FC?=250
  MODEL_L1_MEMORY=$(shell expr 60000 \- $(TOTAL_STACK_SIZE))
  MODEL_L2_MEMORY=200000
  MODEL_L3_MEMORY=8388608
endif

FLASH_TYPE ?= DEFAULT
RAM_TYPE   ?= DEFAULT

ifeq '$(FLASH_TYPE)' 'HYPER'
  MODEL_L3_FLASH=AT_MEM_L3_HFLASH
else ifeq '$(FLASH_TYPE)' 'MRAM'
  MODEL_L3_FLASH=AT_MEM_L3_MRAMFLASH
  READFS_FLASH = target/chip/soc/mram
else ifeq '$(FLASH_TYPE)' 'QSPI'
  MODEL_L3_FLASH=AT_MEM_L3_QSPIFLASH
else ifeq '$(FLASH_TYPE)' 'OSPI'
  MODEL_L3_FLASH=AT_MEM_L3_OSPIFLASH
else ifeq '$(FLASH_TYPE)' 'DEFAULT'
  MODEL_L3_FLASH=AT_MEM_L3_DEFAULTFLASH
endif

ifeq '$(RAM_TYPE)' 'HYPER'
  MODEL_L3_RAM=AT_MEM_L3_HRAM
else ifeq '$(RAM_TYPE)' 'QSPI'
  MODEL_L3_RAM=AT_MEM_L3_QSPIRAM
else ifeq '$(RAM_TYPE)' 'OSPI'
  MODEL_L3_RAM=AT_MEM_L3_OSPIRAM
else ifeq '$(RAM_TYPE)' 'DEFAULT'
  MODEL_L3_RAM=AT_MEM_L3_DEFAULTRAM
endif

 
# MODEL_L3_CONST=AT_MEM_L3_HFLASH

pulpChip = GAP
APP = body_detection

APP_CFLAGS += -g -w #-DNORM_ROUND
APP_CFLAGS += -O2 -s -mno-memcpy -fno-tree-loop-distribute-patterns
APP_CFLAGS += -I. -I./helpers -I$(TILER_EMU_INC) -I$(TILER_INC) -I$(GEN_PATH) -I$(MODEL_BUILD) $(CNN_LIB_INCLUDE)
APP_CFLAGS += -DCLUSTER_STACK_SIZE=$(CLUSTER_STACK_SIZE) -DCLUSTER_SLAVE_STACK_SIZE=$(CLUSTER_SLAVE_STACK_SIZE)
APP_CFLAGS += -DFREQ_CL=$(FREQ_CL) -DFREQ_FC=$(FREQ_FC)
APP_CFLAGS += -D__$(MODEL_L3_FLASH)__ -D__$(MODEL_L3_RAM)__
ifeq ($(SILENT),1)
  APP_CFLAGS += -DSILENT=1
endif


ifeq ($(FROM_CAMERA),1)
  APP_CFLAGS += -DFROM_CAMERA=1
  ifeq ('$(TARGET_CHIP_FAMILY)','GAP9')
  CONFIG_ILI9341 = 1
  CONFIG_OV9281 = 1
  CONFIG_DISPLAY = 1
  APP_CFLAGS += -DWcam=320 -DHcam=240 
  APP_CFLAGS += -DCAM_FORMAT=PI_CAMERA_QVGA
  APP_SRCS += mipi_camera.c
  else
  APP_CFLAGS += -DWcam=324 -DHcam=244
  endif
endif

ifeq ($(NO_BRIDGE),1)
  APP_CFLAGS += -DNO_BRIDGE=1
endif

APP_SRCS += main.c ImgIO.c ImageDraw.c SSDKernels.c SSDBasicKernels.c SSDParams.c $(MODEL_GEN_C) $(CNN_LIB) 

ifeq ($(platform),gvsoc)
  $(info Platform is GVSOC)
  READFS_FILES=$(MODEL_TENSORS)
else
  $(info Platform is GAPUINO)
  PLPBRIDGE_FLAGS = -f
  READFS_FILES=$(MODEL_TENSORS)
endif

io=host

#####Here we add cutom kernels that are not yet integrated into AT libraries

SSD_MODEL_GEN = SSDKernels
SSD_MODEL_GEN_C = $(addsuffix .c, $(SSD_MODEL_GEN))
SSD_MODEL_GEN_CLEAN = $(SSD_MODEL_GEN_C) $(addsuffix .h, $(SSD_MODEL_GEN))
SSD_MODEL_GEN_FLAGS = -DMODEL_L3_RAM=$(MODEL_L3_RAM) -DMODEL_L3_FLASH=$(MODEL_L3_FLASH)

GenSSDTile: SSDModel.c
	gcc -g -o GenSSDTile -I"$(TILER_INC)" $(SSD_MODEL_GEN_FLAGS)  SSDModel.c $(TILER_LIB)

$(SSD_MODEL_GEN_C): GenSSDTile 
	./GenSSDTile

SSDParams.c: $(SSD_MODEL_GEN_C)
	cd SSDParamsGenerator && $(MAKE) all run

SSD_model: SSDParams.c

#################

# build depends on the model
build:: SSD_model model

clean:: #clean_model
	rm -rf BUILD
	rm -rf GenTile
	cd SSDParamsGenerator && $(MAKE) clean
	rm -rf SSDParams.c SSDParams.h
	rm -rf GenSSDTile $(SSD_MODEL_GEN_CLEAN)

clean_all: clean clean_model 

.PHONY: clean_all

include model_rules.mk
include $(RULES_DIR)/pmsis_rules.mk  # gap_sdk/utils/rules/

