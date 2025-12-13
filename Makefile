# Project Name
TARGET = Swarm

# Sources
CPP_SOURCES = Swarm.cpp Filter.cpp

# Library Locations
LIBDAISY_DIR = ./libDaisy

# Core location, and generic makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

