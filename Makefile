CC      := gcc
CFLAGS  := -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -Wall -Wextra -I src/kernel/include
LD      := ld
LDFLAGS := -m elf_i386 -T src/kernel/linker.ld --oformat binary
NASM    := nasm
QEMU    := qemu-system-x86_64

SRC_DIR   := src
BUILD_DIR := build

# C source files across subsystems
C_SRCS := src/kernel/kernel.c \
          src/kernel/cpu/idt.c \
          src/kernel/cpu/pic.c \
          src/kernel/drivers/vga.c \
          src/kernel/drivers/keyboard.c \
          src/kernel/lib/string.c \
          src/kernel/shell/shell.c

C_OBJS := $(patsubst src/kernel/%.c, $(BUILD_DIR)/kernel/%.o, $(C_SRCS))

OS_BIN := $(BUILD_DIR)/os.bin

all: $(OS_BIN)

# 1. 16-bit Bootloader
$(BUILD_DIR)/boot.bin: $(SRC_DIR)/boot/boot.asm
	@mkdir -p $(BUILD_DIR)
	$(NASM) -f bin $< -o $@

# 2. Kernel Assembly Entry
$(BUILD_DIR)/kernel_entry.o: $(SRC_DIR)/kernel/kernel_entry.asm
	@mkdir -p $(BUILD_DIR)
	$(NASM) -f elf32 $< -o $@

# 3. Compile C Modules
$(BUILD_DIR)/kernel/%.o: $(SRC_DIR)/kernel/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# 4. Link Kernel binary
$(BUILD_DIR)/kernel.bin: $(BUILD_DIR)/kernel_entry.o $(C_OBJS) $(SRC_DIR)/kernel/linker.ld
	$(LD) $(LDFLAGS) $(BUILD_DIR)/kernel_entry.o $(C_OBJS) -o $@

# 5. Raw bootable disk image (padded to 16KB)
$(OS_BIN): $(BUILD_DIR)/boot.bin $(BUILD_DIR)/kernel.bin
	cat $(BUILD_DIR)/boot.bin $(BUILD_DIR)/kernel.bin > $@
	truncate -s 16K $@

# 6. Bootable ISO Image (El Torito Floppy Emulation)
OS_ISO := $(BUILD_DIR)/os.iso

iso: $(OS_ISO)

$(OS_ISO): $(OS_BIN)
	@mkdir -p $(BUILD_DIR)/iso_root
	cp $(OS_BIN) $(BUILD_DIR)/iso_root/floppy.img
	truncate -s 1440K $(BUILD_DIR)/iso_root/floppy.img
	xorriso -as mkisofs -b floppy.img -o $@ $(BUILD_DIR)/iso_root
	@rm -rf $(BUILD_DIR)/iso_root

run: $(OS_BIN)
	$(QEMU) -drive format=raw,file=$(OS_BIN)

run-iso: $(OS_ISO)
	$(QEMU) -cdrom $(OS_ISO)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all run run-iso iso clean
