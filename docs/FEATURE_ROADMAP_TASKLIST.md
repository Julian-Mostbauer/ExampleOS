# ExampleOS Prioritized Feature Roadmap & Tasklist

This roadmap details unimplemented operating system capabilities organized strictly by architectural priority (P0 to P4), focusing on system stability, security, memory protection, scheduling, and I/O performance.

---

## Priority Matrix Overview

| Priority | Category | Key Milestones | Primary Benefit |
|----------|----------|----------------|-----------------|
| **P0** | **Blockers & Foundation** | Fix 64-bit div link error, CPU Exception Handlers (0–31), Panic Screen, Core `memcpy`/`memset`, Bootloader size limit resolution | Stabilizes kernel build, eliminates triple-fault crashes |
| **P1** | **Memory Protection & Security** | Physical Memory Manager (PMM), Virtual Memory (Paging), Kernel Heap (`kmalloc`/`kfree`), Ring 0/3 GDT & TSS | Isolates memory, halts arbitrary physical RAM corruption |
| **P2** | **Preemption & User Space Execution** | Task Control Block (TCB/PCB), Preemptive Scheduler, Context Switch, Syscall Interface (`int 0x80`), Ring 3 User Mode | Enables multi-tasking and safe unprivileged execution |
| **P3** | **Storage & File System** | ATA/IDE PIO Driver, Virtual File System (VFS), Ext2 or TAR/FAT filesystem, ELF32 Binary Loader | Allows persistent storage and loading external programs |
| **P4** | **I/O, Drivers & Usability** | Serial Port (COM1) Debug Logger, VGA Backbuffer, CMOS/RTC Real-Time Clock, PS/2 Mouse Driver, CLI History | Enhances debuggability, graphics rendering, and user experience |

---

## Phase P0: Blockers & Kernel Stability Foundations

### Task P0.1: Resolve 64-bit Math Linker Failure (Build Blocker)
* **Goal:** Allow building the kernel with 64-bit integer support under `-nostdlib`.
* **Action Items:**
  1. Update `Makefile` to link `libgcc` via `$(CC) $(CFLAGS) ... -lgcc` or dynamic discovery using `$(shell $(CC) -m32 -print-libgcc-file-name)`.
  2. Alternatively, implement custom bitwise 64-bit division (`__udivdi3`, `__umoddi3`) in `src/kernel/lib/math.c`.
* **Security & Performance Impact:** Zero performance cost; resolves immediate build obstruction.

### Task P0.2: Implement CPU Exception Handlers (ISR 0–31) & Kernel Panic
* **Goal:** Replace silent hardware triple-fault reboots with diagnostic crash dumps.
* **Action Items:**
  1. In `src/kernel/kernel_entry.asm`, generate 32 interrupt service stubs for vectors `0x00`–`0x1F`. Push dummy error codes (`0`) for exceptions that do not provide hardware error codes, ensuring uniform stack frames.
  2. Implement `struct registers` containing all saved general-purpose registers, segment selectors, error codes, and EIP/CS/EFLAGS.
  3. Create `src/kernel/cpu/panic.c` with formatted register dump, human-readable exception names (e.g. `#GP General Protection Fault`, `#PF Page Fault`), and halt state.
  4. Register all 32 exception gates in `idt_init()`.
* **Security & Performance Impact:** High security diagnostic value; prevents unhandled fault state execution.

### Task P0.3: Core Memory Primitives (`memcpy`, `memset`, `memmove`, `memcmp`)
* **Goal:** Provide fast, safe, standard memory manipulation functions.
* **Action Items:**
  1. Create `src/kernel/lib/memory.c` and `src/kernel/include/memory.h`.
  2. Implement byte-aligned fallback alongside dword (32-bit) aligned copying using inline assembly (`rep movsl`, `rep stosl`).
  3. Replace manual C loops in `vga.c`, `keyboard.c`, and string utilities with standard primitives.
* **Security & Performance Impact:** Massive performance boost for screen scrolling, buffer operations, and struct initialization.

### Task P0.4: Eliminate 15.5 KB Bootloader Limitation
* **Goal:** Support kernel sizes exceeding 31 disk sectors.
* **Action Items:**
  1. Option A: Upgrade `src/boot/boot.asm` to read sectors using BIOS LBA extensions (`int 0x13, ah=0x42`) or compute track/head/cylinder bounds dynamically.
  2. Option B: Adopt the Multiboot specification (`multiboot.h` header and assembly entry), delegating booting and RAM mapping to GRUB or QEMU `-kernel`.
* **Security & Performance Impact:** Prevents silent truncation and corruption as kernel binary scales.

---

## Phase P1: Memory Management & Security Architecture

### Task P1.1: Physical Memory Manager (PMM)
* **Goal:** Track allocation and deallocation of 4 KB physical memory frames.
* **Action Items:**
  1. Retrieve BIOS physical memory map (`int 0x15, eax=0xE820`) in bootloader and pass pointer to kernel, or read Multiboot memory map structure.
  2. Implement bitmap-based physical frame allocator (`pmm_alloc_page()`, `pmm_free_page()`).
  3. Reserve physical ranges: Real Mode lower memory (`0x00000000`–`0x00100000`), kernel code/data image, and IDT/GDT structures.
* **Security Impact:** Prevents memory exhaustion and accidental overwriting of reserved hardware memory regions.

### Task P1.2: Virtual Memory Manager (VMM) & Paging
* **Goal:** Enable hardware address translation, page-level permissions, and memory isolation.
* **Action Items:**
  1. Create Page Directory and Page Tables mapped to 4 KB pages (`CR0.PG = 1`).
  2. Identity-map low memory / kernel image with Supervisor-only (`U/S = 0`), Read/Write (`R/W = 1`) permissions.
  3. Map high-memory kernel space (`0xC0000000` Higher-Half Kernel) to separate user address space from kernel internals.
  4. Implement `vmm_map_page(pd, vaddr, paddr, flags)` and `vmm_unmap_page(pd, vaddr)`.
  5. Setup Page Fault Handler (`#PF`, Vector 14) to inspect faulting address from `CR2` and error code.
* **Security & Performance Impact:** Enforces kernel/user isolation; catches null pointer dereferences and out-of-bounds execution instantly.

### Task P1.3: Dynamic Kernel Heap Allocator (`kmalloc` / `kfree`)
* **Goal:** Allow runtime dynamic memory allocation within the kernel.
* **Action Items:**
  1. Implement block-header allocator (e.g. Free-List or Doug Lea / Slab allocator) mapped across heap virtual address space.
  2. Support boundary tags, memory coalescing on `kfree`, and alignment constraints (e.g., 8-byte and 4096-byte page alignment).
* **Security & Performance Impact:** Mitigates fixed-buffer limitations; reduces static BSS footprint.

### Task P1.4: GDT Expansion with User Segments & Task State Segment (TSS)
* **Goal:** Prepare hardware privilege levels for unprivileged execution (Ring 3).
* **Action Items:**
  1. Expand GDT in C code:
     * Kernel Code (Ring 0, Exec/Read)
     * Kernel Data (Ring 0, Read/Write)
     * User Code (Ring 3, DPL 3, Exec/Read)
     * User Data (Ring 3, DPL 3, Read/Write)
     * Task State Segment (TSS Descriptor)
  2. Allocate and initialize hardware TSS structure, setting `ESP0` and `SS0` for privilege transitions.
  3. Load TSS selector into Task Register via `ltr` assembly instruction.
* **Security Impact:** Prerequisite for CPU-enforced hardware privilege ring separation.

---

## Phase P2: Preemptive Multitasking & User Mode Isolation

### Task P2.1: Process & Task Control Block (PCB/TCB)
* **Goal:** Represent distinct threads/processes with isolated CPU context and memory spaces.
* **Action Items:**
  1. Define `struct task`: PID, state (READY, RUNNING, SLEEPING, ZOMBIE), registers, kernel stack pointer, user stack pointer, and page directory pointer (`CR3`).
  2. Implement kernel thread creation (`task_create()`).

### Task P2.2: Context Switching & Timer Preemption
* **Goal:** Enable concurrent task execution without voluntary yielding.
* **Action Items:**
  1. Write assembly routine `switch_to(old_task, new_task)` saving callee-saved registers (`EBX`, `ESI`, `EDI`, `EBP`, `ESP`) and switching stacks.
  2. Connect scheduler to PIT timer interrupt (IRQ 0) to invoke `schedule()` on quantum expiration.
  3. Implement Round-Robin scheduler algorithm.

### Task P2.3: Software Interrupt System Call Interface
* **Goal:** Controlled gateway for user programs to request kernel services.
* **Action Items:**
  1. Register IDT Gate `0x80` with DPL 3 (`type_attr = 0xEE`), enabling invocation from Ring 3 without faulting.
  2. Define register-based calling convention (e.g. `EAX` = syscall number, `EBX`, `ECX`, `EDX` = arguments).
  3. Implement core syscalls: `sys_exit`, `sys_write`, `sys_read`, `sys_sleep`, `sys_yield`.
  4. Perform strict pointer and memory boundary validation: check whether caller-provided buffers reside entirely in user-space pages.
* **Security Impact:** Eliminates arbitrary hardware I/O instructions from unprivileged code; strictly validates input pointers.

### Task P2.4: Ring 3 Transition (`enter_usermode`)
* **Goal:** Drop CPU execution from Ring 0 to Ring 3.
* **Action Items:**
  1. Push user data selector (`0x23`), user stack pointer, `EFLAGS` (with IF enabled), user code selector (`0x1B`), and entry point `EIP`.
  2. Execute `iret` to drop CPU into CPL 3.
* **Security Impact:** Confines external programs to restricted sandbox.

---

## Phase P3: Storage, File System & Program Loading

### Task P3.1: ATA PIO Mode Hard Disk Driver
* **Goal:** Non-volatile read/write disk block access.
* **Action Items:**
  1. Write IDE controller driver targeting primary bus (`0x1F0`–`0x1F7`) and control port (`0x3F6`).
  2. Implement 28-bit / 48-bit Logical Block Addressing (LBA) sector read and write functions.
  3. Provide sector caching to avoid repeated bus roundtrips.
* **Performance Impact:** Dramatically outperforms BIOS floppy read speeds.

### Task P3.2: Virtual File System (VFS) Layer
* **Goal:** Abstract file operations across different storage mechanisms.
* **Action Items:**
  1. Define standard file abstractions: `vfs_node`, `vfs_read`, `vfs_write`, `vfs_open`, `vfs_close`, `vfs_readdir`.
  2. Provide mount points and root tree structure.

### Task P3.3: Filesystem Implementation (Initrd / FAT / Ext2)
* **Goal:** Structured directory tree and named files.
* **Action Items:**
  1. Option 1: Initial RAM Disk (Initrd/tarfs) embedded into kernel image or loaded as a boot module.
  2. Option 2: Read-only Ext2 or FAT16 driver for disk partitions.
* **Security & Performance Impact:** Decouples user applications, assets, and config files from raw kernel source.

### Task P3.4: ELF32 Binary Loader
* **Goal:** Load and execute compiled native binaries.
* **Action Items:**
  1. Parse ELF header, verify magic bytes (`0x7F 'E' 'L' 'F'`) and architecture (`EM_386`).
  2. Iterate program headers (`PT_LOAD`), allocate virtual pages, and copy segments into place.
  3. Set up user stack, pass `argc`/`argv`, and transfer execution to ELF entry point.

---

## Phase P4: Drivers, Usability & System Enhancements

### Task P4.1: Serial Port Driver (UART 16550) for Kernel Logging
* **Goal:** Offload kernel diagnostics to serial output (`COM1` at `0x3F8`).
* **Action Items:**
  1. Initialize UART baud rate divisor (115200 baud), FIFO control, and line control registers.
  2. Implement `kprintf()` routing directly to serial port.
  3. Run QEMU with `-serial stdio` to view kernel logs in host terminal independently of VGA state.
* **Performance Impact:** Enables debugging graphics-mode code without disturbing the VGA screen buffer.

### Task P4.2: VGA Graphics Double Buffering (RAM Backbuffer)
* **Goal:** Eliminate screen tearing and improve graphical rendering speed.
* **Action Items:**
  1. Allocate a 64,000-byte offscreen frame buffer in system RAM.
  2. Direct all `put_pixel`, `draw_rect`, and font rendering to the RAM buffer.
  3. Implement `vga_blit()` to copy the entire 64 KB buffer to `0xA0000` via fast 32-bit `rep movsd` during vertical retrace (VBLANK, port `0x3DA` bit 3).
* **Performance Impact:** 100% flicker-free graphics and faster primitive rasterization.

### Task P4.3: Real-Time Clock (RTC) / CMOS Driver
* **Goal:** Provide real date and time rather than arbitrary tick counters.
* **Action Items:**
  1. Read CMOS registers `0x70`/`0x71` for seconds, minutes, hours, day, month, and year.
  2. Handle BCD to binary decoding and update time during RTC interrupt (IRQ 8) or on-demand.

### Task P4.4: Hardened String & I/O Library (`snprintf`)
* **Goal:** Prevent buffer overflow vulnerabilities across all shell and driver operations.
* **Action Items:**
  1. Replace unbounded `sprintf` with capacity-checked `vsnprintf(char *buf, size_t size, const char *fmt, va_list args)`.
  2. Audit `src/kernel/shell/shell.c` input processing to validate command argument bounds.
