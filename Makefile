CC = gcc

# Project directories
BUILD_DIR = build
FREERTOS_DIR = ..
KERNEL_DIR = $(FREERTOS_DIR)/Source

# Include directories
INCLUDES = -Iinclude
INCLUDES += -I.
INCLUDES += -Iuser_tasks
INCLUDES += -I$(KERNEL_DIR)/include
INCLUDES += -I$(KERNEL_DIR)/portable/ThirdParty/GCC/Posix
INCLUDES += -I$(KERNEL_DIR)/portable/ThirdParty/GCC/Posix/utils

# Our application source files
SOURCE_FILES = src/main.c
SOURCE_FILES += src/frame_calculator.c
SOURCE_FILES += src/cyclic_executive.c
SOURCE_FILES += src/scheduler.c
SOURCE_FILES += user_tasks/user_tasks.c 

# FreeRTOS kernel source files
SOURCE_FILES += $(wildcard $(KERNEL_DIR)/*.c)

# Memory management
SOURCE_FILES += $(KERNEL_DIR)/portable/MemMang/heap_3.c

# FreeRTOS POSIX port
SOURCE_FILES += $(KERNEL_DIR)/portable/ThirdParty/GCC/Posix/port.c
SOURCE_FILES += $(KERNEL_DIR)/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c

# Compiler flags
CFLAGS = -Wall -Wextra -ggdb3 -O0
CPPFLAGS = $(INCLUDES)
CPPFLAGS += -DprojCOVERAGE_TEST=0
CPPFLAGS += -DprojENABLE_TRACING=0
CPPFLAGS += -D_WINDOWS_

# Linker flags
LDFLAGS = -pthread

# Object files
OBJ_FILES = $(SOURCE_FILES:%.c=$(BUILD_DIR)/%.o)

# Executable
TARGET = $(BUILD_DIR)/cyclic_executive

all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	mkdir -p $(@D)
	$(CC) $^ $(LDFLAGS) -o $@

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -c $< -o $@

-include $(OBJ_FILES:.o=.d)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
