APP = test
APP_SRCS += test.c
APP_CFLAGS += -O3 -g

TEST_TYPE ?= quick

ifeq '$(TEST_TYPE)' 'fast'
APP_CFLAGS += -DTEST_TYPE_FAST
height = 20
endif

CONFIG_GAP_LIB_IMGIO = 1

APP_CFLAGS      += -DSAVE_PIC #-DOV5647 -DRAW8
APP_CFLAGS      += -DI3C_I2C
APP_CFLAGS      += -DLOGS

bayer2rgb:
	python bayer2rgb.py

include $(RULES_DIR)/pmsis_rules.mk
