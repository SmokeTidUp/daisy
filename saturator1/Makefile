# Project Name
TARGET ?= saturator1

DEBUG = 1
OPT = -O0

# Sources
CPP_SOURCES = ../kxmx_bluemchen/src/kxmx_bluemchen.cpp src/${TARGET}.cpp

USE_FATFS = 1

# Library Locations
LIBDAISY_DIR = ../libDaisy
DAISYSP_DIR = ../DaisySP
KXMX_DIR = ../kxmx_bluemchen

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

