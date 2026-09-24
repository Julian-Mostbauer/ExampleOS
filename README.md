# ExampleOS

A minimal 32-bit x86 hobby operating system written from scratch in C and Assembly.

## Important Sources

https://wiki.osdev.org/

## Prerequisites

Ensure you have the following installed:

- **GCC** (with 32-bit support)
- **NASM**
- **GNU Make**
- **QEMU** (`qemu-system-x86_64`)
- *(Optional)* **xorriso** (only needed for building `.iso` images)

## Build & Run

Clone the repository and run:

```bash
# Build disk image and launch in QEMU
make run
```

### Other Useful Commands

```bash
# Build raw disk image (build/os.bin)
make

# Build bootable CD-ROM ISO (build/os.iso)
make iso

# Run ISO in QEMU
make run-iso

# Clean build artifacts
make clean
```

## Quick Controls

- **F1**: Toggle keyboard layout between German (QWERTZ) and US (QWERTY) in real time.
- **Commands**: Type `help` in the shell to see available commands (`clear`, `about`, `layout`, `echo`, `shutdown`).
