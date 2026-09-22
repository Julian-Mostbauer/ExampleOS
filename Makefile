# Toolchain configuration
CC      := gcc
CFLAGS  := -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -Wall -Wextra
LD      := ld
LDFLAGS := -m elf_i386 -T src/kernel/linker.ld --oformat binary
NASM    := nasm
QEMU    := qemu-system-x86_64

# Directory configuration
SRC_DIR   := src
BUILD_DIR := build

# Output targets
OS_BIN := $(BUILD_DIR)/os.bin

all: $(OS_BIN)

# Ensure build directory exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 1. 16-bit Bootloader
$(BUILD_DIR)/boot.bin: $(SRC_DIR)/boot/boot.asm | $(BUILD_DIR)
	$(NASM) -f bin $< -o $@

# 2. Kernel Assembly Entry
$(BUILD_DIR)/kernel_entry.o: $(SRC_DIR)/kernel/kernel_entry.asm | $(BUILD_DIR)
	$(NASM) -f elf32 $< -o $@

# 3. Kernel C code
$(BUILD_DIR)/kernel.o: $(SRC_DIR)/kernel/kernel.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 4. Link Kernel binary
$(BUILD_DIR)/kernel.bin: $(BUILD_DIR)/kernel_entry.o $(BUILD_DIR)/kernel.o $(SRC_DIR)/kernel/linker.ld | $(BUILD_DIR)
	$(LD) $(LDFLAGS) $(BUILD_DIR)/kernel_entry.o $(BUILD_DIR)/kernel.o -o $@

# 5. Combine into raw bootable disk image (padded to 16KB)
$(OS_BIN): $(BUILD_DIR)/boot.bin $(BUILD_DIR)/kernel.bin | $(BUILD_DIR)
	cat $(BUILD_DIR)/boot.bin $(BUILD_DIR)/kernel.bin > $@
	truncate -s 16K $@

# Run in QEMU
run: $(OS_BIN)
	$(QEMU) -drive format=raw,file=$(OS_BIN)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all run clean
