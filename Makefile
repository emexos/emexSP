ARCH ?= x86_64
AS = nasm
CC = $(ARCH)-elf-gcc
LD = $(ARCH)-elf-ld
OBJCOPY = $(ARCH)-elf-objcopy
QEMU = qemu-system-$(ARCH)

# Assembler flags
NASMFLAGS = -f elf64
NASMFLAGS_BIN = -f bin

# Include directories
INC_DIRS = -I./src/include -I./src

# Compiler flags
CFLAGS = -ffreestanding -nostdlib -mno-red-zone -Wall -Wextra -O2 -mcmodel=kernel $(INC_DIRS)

# Linker flags
LDFLAGS = -T src/linker.ld -nostdlib

# Build directory
BUILD_DIR = build

# Source files
BOOT_STAGE1 = XBL2/stage1.s
BOOT_STAGE2 = XBL2/stage2.s
KERNEL_ENTRY = src/entry.s
KERNEL_C = src/kernel/kernel.c
TEXT_UTILS_C = src/include/text/text_utils.c
STRING_UTILS_C = src/include/text/string_utils.c
MEMORY_C = src/include/memory/memory.c
SHELL_C = src/shell/shell.c
KEYBOARD_C = src/drivers/keyboard/keyboard.c
DISK_DRIVER_C = src/drivers/disk/disk_driver.c

# Object files
BOOT_STAGE1_BIN = $(BUILD_DIR)/stage1.bin
BOOT_STAGE2_BIN = $(BUILD_DIR)/stage2.bin
KERNEL_ENTRY_OBJ = $(BUILD_DIR)/entry.o
KERNEL_C_OBJ = $(BUILD_DIR)/kernel.o
TEXT_UTILS_OBJ = $(BUILD_DIR)/text_utils.o
STRING_UTILS_OBJ = $(BUILD_DIR)/string_utils.o
MEMORY_OBJ = $(BUILD_DIR)/memory.o
SHELL_OBJ = $(BUILD_DIR)/shell.o
KEYBOARD_OBJ = $(BUILD_DIR)/keyboard.o
DISK_DRIVER_OBJ = $(BUILD_DIR)/disk_driver.o

# All kernel objects
KERNEL_OBJS = $(KERNEL_ENTRY_OBJ) $(KERNEL_C_OBJ) $(TEXT_UTILS_OBJ) $(STRING_UTILS_OBJ) $(MEMORY_OBJ) $(SHELL_OBJ) $(KEYBOARD_OBJ) $(DISK_DRIVER_OBJ)

# Output files
KERNEL_ELF = $(BUILD_DIR)/kernel-$(ARCH).elf
KERNEL_BIN = $(BUILD_DIR)/kernel-$(ARCH).bin
OS_IMG = $(BUILD_DIR)/emexOS3.img

all: clean compiledb $(OS_IMG) run

# Generate compile_commands.json for language servers
compiledb: | $(BUILD_DIR)
	@which compiledb > /dev/null 2>&1 || (echo "compiledb is not installed. Please install it using pip install compiledb" && exit 1)
	@echo "Generating compile_commands.json in $(BUILD_DIR)..."
	@make -Bnwk $(KERNEL_C_OBJ) $(TEXT_UTILS_OBJ) $(STRING_UTILS_OBJ) $(MEMORY_OBJ) $(SHELL_OBJ) $(KEYBOARD_OBJ) | compiledb -o $(BUILD_DIR)/compile_commands.json

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/drivers
	mkdir -p $(BUILD_DIR)/shell
	mkdir -p $(BUILD_DIR)/kernel
	mkdir -p $(BUILD_DIR)/memory

# Bootloader stage 1
$(BOOT_STAGE1_BIN): $(BOOT_STAGE1) | $(BUILD_DIR)
	$(AS) $(NASMFLAGS_BIN) $< -o $@

# Bootloader stage 2
$(BOOT_STAGE2_BIN): $(BOOT_STAGE2) | $(BUILD_DIR)
	$(AS) $(NASMFLAGS_BIN) $< -o $@

# Text utilities
$(TEXT_UTILS_OBJ): $(TEXT_UTILS_C) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# String utilities
$(STRING_UTILS_OBJ): $(STRING_UTILS_C) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Memory utilities
$(MEMORY_OBJ): $(MEMORY_C) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Keyboard driver
$(KEYBOARD_OBJ): $(KEYBOARD_C) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Disk driver
$(DISK_DRIVER_OBJ): $(DISK_DRIVER_C) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Shell
$(SHELL_OBJ): $(SHELL_C) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Kernel entry point (assembly)
$(KERNEL_ENTRY_OBJ): $(KERNEL_ENTRY) | $(BUILD_DIR)
	$(AS) $(NASMFLAGS) $< -o $@

# Main kernel
$(KERNEL_C_OBJ): $(KERNEL_C) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link kernel
$(KERNEL_ELF): $(KERNEL_OBJS) | $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $^

# Convert ELF to binary
$(KERNEL_BIN): $(KERNEL_ELF) | $(BUILD_DIR)
	$(OBJCOPY) -O binary $< $@

# Create OS image
$(OS_IMG): $(BOOT_STAGE1_BIN) $(BOOT_STAGE2_BIN) $(KERNEL_BIN)
	dd if=/dev/zero of=$(OS_IMG) bs=512 count=2880
	dd if=$(BOOT_STAGE1_BIN) of=$(OS_IMG) bs=512 seek=0 conv=notrunc
	dd if=$(BOOT_STAGE2_BIN) of=$(OS_IMG) bs=512 seek=1 conv=notrunc
	dd if=$(KERNEL_BIN) of=$(OS_IMG) bs=512 seek=4 conv=notrunc

# Clean build files
clean:
	rm -rf $(BUILD_DIR)/*.o $(BUILD_DIR)/*.bin $(BUILD_DIR)/*.img $(KERNEL_ELF)
	rm -rf $(BUILD_DIR)/drivers/*.o $(BUILD_DIR)/shell/*.o $(BUILD_DIR)/kernel/*.o $(BUILD_DIR)/memory/*.o
	rm -f $(BUILD_DIR)/compile_commands.json

# Run in QEMU
run: $(OS_IMG)
	$(QEMU) -vga std -m 128M -drive file=$(OS_IMG),format=raw -serial stdio # -S -gdb tcp::1234

# Clean and rebuild everything, then run
rerun: clean all run

# Debug target (with GDB support)
debug: $(OS_IMG)
	$(QEMU) -vga std -m 128M -drive file=$(OS_IMG),format=raw -serial stdio -S -gdb tcp::1234

# Show size of kernel components
size: $(KERNEL_OBJS)
	@echo "Kernel component sizes:"
	@size $(KERNEL_OBJS) $(KERNEL_ELF)

# Create directory structure if it doesn't exist
dirs:
	mkdir -p src/drivers
	mkdir -p src/shell
	mkdir -p $(BUILD_DIR)/drivers
	mkdir -p $(BUILD_DIR)/shell
	mkdir -p $(BUILD_DIR)/kernel
	mkdir -p $(BUILD_DIR)/memory

# Help target
help:
	@echo "Available targets:"
	@echo "  all       - Build everything and run (default)"
	@echo "  clean     - Remove build files"
	@echo "  run       - Run the OS in QEMU"
	@echo "  rerun     - Clean, build, and run"
	@echo "  debug     - Run with GDB debugging enabled"
	@echo "  size      - Show size of kernel components"
	@echo "  dirs      - Create necessary directory structure"
	@echo "  compiledb - Generate compile_commands.json"
	@echo "  help      - Show this help message"

.PHONY: all clean run rerun debug size dirs help compiledb
