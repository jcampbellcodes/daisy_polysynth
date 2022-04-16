# Project Name
TARGET = pluck

# Sources
CPP_SOURCES = pluck.cpp

# Library Locations
LIBDAISY_DIR ?= ../DaisyExamples/libdaisy
DAISYSP_DIR ?= ../DaisyExamples/DaisySP

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

