# ExampleOS Project Technical Review

**Date:** September 2026  
**Target Architecture:** x86 (IA-32, 32-bit Protected Mode)  
**Codebase Language:** C99, x86 Assembly (NASM)  
**Binary Output:** Flat binary image (`os.bin`), bootable ISO (`os.iso`)

---

## 1. Executive Summary

ExampleOS is a minimal x86 hobby operating system bootable from raw disk or CD-ROM (El Torito floppy emulation). It initializes 32-bit protected mode, configures the Programmable Interrupt Controller (PIC) and Interrupt Descriptor Table (IDT), captures timer and keyboard IRQs, and presents an interactive shell and VGA Mode 13h graphics application (Pong).

While the kernel demonstrates clear hardware control (VGA register manipulation, font plane backup/restore, low-power sleep via `hlt`), it currently functions as a single-privilege monolithic monitor rather than a secure operating system. There is no memory protection, no CPU exception handling, no task isolation, and no filesystem. Furthermore, the repository currently exhibits a build-breaking linker error due to 64-bit integer division without `libgcc`.

---

## 2. Architecture & Subsystem Audit

### 2.1 Bootloader & Kernel Handoff (`src/boot/boot.asm`)
* **Mechanism:** Single-sector MBR (512 bytes) loaded by BIOS at `0x7C00`.
* **Sector Loading:** Uses BIOS `int 0x13, ah=0x02` to load exactly 31 sectors (15.5 KB) to physical address `0x1000`.
* **GDT Setup:** Basic flat model with two 4 GB descriptors:
  * Code Segment: Base `0x00000000`, Limit `0xFFFFFFFF`, Ring 0, Read/Execute.
  * Data Segment: Base `0x00000000`, Limit `0xFFFFFFFF`, Ring 0, Read/Write.
* **CPU Mode Switch:** Sets bit 0 (Protection Enable) in `CR0`, executes far jump to flush the prefetch queue into 32-bit mode.
* **Stack Setup:** Initializes `ESP` to `0x90000`.

**Architectural Deficiencies:**
1. **Hardcoded Sector Count:** Loading exactly 31 sectors means any kernel exceeding 15,872 bytes is silently truncated on disk, causing unpredictable runtime corruption or crashes.
2. **Floppy/BIOS Geometry Limitations:** `boot.asm` queries cylinder 0, head 0, sector 2 through 32. Standard floppy sectors-per-track is 18. Reading sector numbers above 18 without cylinder/head stepping fails on real BIOS hardware.
3. **No Memory Detection:** Does not invoke BIOS `int 0x15, eax=0xE820` while in 16-bit real mode. The kernel is unaware of physical RAM layout, memory holes, or ACPI reserved zones.

---

### 2.2 Interrupts & CPU Exceptions (`src/kernel/cpu/idt.c`, `src/kernel/cpu/pic.c`, `src/kernel/kernel_entry.asm`)
* **PIC Configuration:** 8259 PIC master and slave remapped to vectors `0x20`–`0x27` (IRQ 0–7) and `0x28`–`0x2F` (IRQ 8–15).
* **IDT Setup:** 256 gates defined with DPL 0 (Ring 0). Only IRQ 0 (Timer, vector `0x20`) and IRQ 1 (Keyboard, vector `0x21`) are populated.
* **ISR Dispatch:** Assembly wrappers in `kernel_entry.asm` save registers via `pusha`, set segment registers to kernel data segment (`0x10`), invoke C handlers, restore registers, and execute `iret`.

**Architectural Deficiencies:**
1. **Missing CPU Exception Handlers (Vectors 0–31):** Vectors for Divide-by-Zero (`#DE`), Invalid Opcode (`#UD`), Double Fault (`#DF`), General Protection Fault (`#GP`), and Page Fault (`#PF`) are empty. Any CPU exception results in an unhandled interrupt, precipitating an instant triple-fault and machine reboot.
2. **Missing Spurious IRQ Handling:** IRQ 7 and IRQ 15 spurious interrupts are not handled or verified against the In-Service Register (ISR).
3. **No Error Code Handling:** Assembly ISR stub does not pop error codes pushed by hardware on specific exceptions (`#DF`, `#TS`, `#NP`, `#SS`, `#GP`, `#PF`, `#AC`).

---

### 2.3 Drivers

#### VGA Text & Graphics (`src/kernel/drivers/vga.c`)
* **Text Mode (03h):** 80x25 text mode mapped at physical `0xB8000`. Includes hardware cursor manipulation via CRT Controller registers (`0x3D4`/`0x3D5`), status bar rendering at line 0, and line scrolling.
* **Graphics Mode (13h):** Direct hardware register programming for 320x200 256-color planar mode mapped at `0xA0000`.
* **Font Preservation:** Clever mechanism in `vga_save_font()` and `vga_restore_font()` switches VGA sequencer and graphics controller to read/write plane 2, preserving BIOS VGA font across mode switches without external font binary dependencies.

#### Keyboard Driver (`src/kernel/drivers/keyboard.c`)
* **Interface:** PS/2 Controller data port `0x60`. Scancode Set 1 decoding.
* **Features:** Circular input queue (256 bytes), key-state table for real-time polling (supporting modifier keys, arrow keys, and game controls), dual layouts (German QWERTZ and US QWERTY) switchable dynamically via hotkey (F1) or shell command.

#### Timer Driver (`src/kernel/drivers/timer.c`)
* **Interface:** 8253/8254 PIT channel 0 operating in Mode 3 (Square Wave) at 1000 Hz.
* **Power Management:** `sleep_ms()` utilizes `__asm__ volatile("sti; hlt")`, idling the CPU in low-power state between interrupt wakeups rather than burning cycles in a spin-wait loop.

---

### 2.4 Standard Library & Utilities (`src/kernel/lib/`)
* **String Operations (`string.c`):** Basic implementations of `strcmp`, `strncmp`, `strlen`. Formatting helpers `append_string`, `append_unsigned`, `append_signed`.
* **Formatted Output (`io_helper.c`):** Custom `sprintf` supporting `%s`, `%c`, `%d`, `%u`, `%x`, `%%`.
* **Math & PRNG (`math.c`, `rand.c`):** Integer base-10 ceiling logarithm and linear congruential generator (LCG).

---

## 3. Security Evaluation

| Vulnerability Domain | Risk Level | Description & Root Cause |
|----------------------|------------|--------------------------|
| **Memory Isolation** | **Critical** | No paging enabled (`CR0.PG = 0`). Flat segmentation maps all physical memory directly to virtual memory without permission masks. Kernel code, IDT, BIOS data area, and stack are universally writable. |
| **Privilege Levels** | **Critical** | Single privilege model (Ring 0). No separation between user applications and kernel core. Applications execute instructions like `cli`, `sti`, `inb`, `outb`, and `hlt` arbitrarily. |
| **Crash Resilience** | **Critical** | Total absence of exception handlers (IDT vectors 0–31). Any standard fault (e.g. division by zero, null pointer access) triggers a silent triple-fault hardware reboot. |
| **Buffer Management** | **High** | `sprintf` in `io_helper.c` lacks maximum buffer capacity parameters (no `snprintf`), making memory adjacent to output buffers vulnerable to overrun. |
| **Command Injection / OOB** | **Medium** | Shell command parser in `src/kernel/shell/shell.c` does unsafe pointer arithmetic (`msg += 5` for `echo`) without confirming string length, allowing reads past buffer limits. |
| **Compiler Mitigations** | **Medium** | Makefile enforces `-fno-stack-protector` and `-fno-pie`. No stack canary validation is implemented to prevent stack frame smashing. |

---

## 4. Performance & Efficiency Evaluation

1. **PIT Tick Frequency Overhead:**
   * PIT timer runs at 1000 Hz (1 tick per millisecond).
   * For an operating system without preemption or process scheduling, handling 1,000 interrupts every second generates redundant context switches and bus traffic. A 100 Hz timer frequency or tickless idle model is significantly more efficient.

2. **Uncached Video Memory Access:**
   * VGA framebuffer writes in Mode 13h (`0xA0000`) and Mode 03h (`0xB8000`) write directly to memory-mapped video RAM across the legacy bus.
   * Pong implements differential redraws (erasing prior frames) to reduce tearing, but lacks an in-RAM backbuffer (double-buffering). Complex frame redraws cause visible flicker and memory bus contention.

3. **String and Memory Primitives:**
   * Complete absence of optimized block memory primitives (`memcpy`, `memset`, `memmove`).
   * VGA scrolling copies memory character by character rather than executing 32-bit dword block transfers (`rep movsd`).

4. **Compiler Optimization Flags:**
   * Makefile specifies flags `-m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -Wall -Wextra`, omitting optimization flags (`-O2` or `-Os`). The code compiles under `-O0`, producing larger binaries and redundant stack push/pop sequences.

---

## 5. Immediate Bugs & Build Blockers

### Linker Failure (`__udivdi3`, `__umoddi3`)
* **Trigger:** Commit `3e747f4` widened `uint64_t` and `int64_t` from `unsigned long` to `unsigned long long` in `src/kernel/include/types.h`.
* **Root Cause:** In 32-bit x86 mode, 64-bit integer division and modulo (`/` and `%`) in `append_unsigned` (`src/kernel/lib/string.c`) emit calls to GCC runtime helper routines `__udivdi3` and `__umoddi3`. Because `-nostdlib` is passed without linking `-lgcc` or defining internal 64-bit math routines, linking fails with:
  ```
  undefined reference to `__umoddi3'
  undefined reference to `__udivdi3'
  ```
* **Remedy:** Either link against `libgcc` (`$(CC) $(CFLAGS) ... -lgcc` or provide `$(shell $(CC) -m32 -print-libgcc-file-name)`), or implement a standalone 64-bit software divider.
