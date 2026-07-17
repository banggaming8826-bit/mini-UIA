# 📖 MINI-UIA OPERATING SYSTEM ARCHITECTURE (64-BIT)

This document provides a comprehensive technical specification of the core components of the Mini-UIA operating system kernel, developed by Bang (aka nickname: **BangGaming8826**) The system operates on 64-bit x86_64 Long Mode, running bare-metal on physical or emulated hardware.

---

## CORE SOURCE TREE
* `include/kdefine.h`: Defines system constants, bit-width types, and core macros;
* `include/kmanager.h`: Declares and implements memory management, Radix Tree structures, processes, and scheduling;
* `kernel/kernel.c`: Handles core initialization (GDT, IDT, PMM, VMM, LAPIC) and houses the kernel entry point (`kmain`);
* `kernel/interrupts.asm`: Written in Assembly; handles hardware interrupts, software traps, and raw process context switching;
* `kernel/bootloader.s`: Switch CPU Mode (Real (x16) -> Protected (x32) -> Long Mode (x64)) (BIOS Boot type);

---

## CHAPTER I: MEMORY MANAGEMENT ARCHITECTURE

Mini-UIA implements a strict tiered memory management model consisting of a Physical Memory Manager (PMM) and a Virtual Memory Manager (VMM), optimized with high-performance data structures.

### 1. Physical Memory Manager (PMM)
The PMM is responsible for tracking and allocating physical memory pages (Page Frames) of standard $4096 \text{ bytes}$;
* **Core Mechanism:** Driven by a bitmap array (`pmm_bitaddr`). Each bit represents the allocation state of one physical page frame: bit `0` for free/available pages, and bit `1` for allocated or hardware-reserved areas;

* **Initialization:** During boot, the `pmm_bitinit` function parses the `E820` memory map provided by the Bootloader to accurately calculate the boundary of available RAM and protect reserved system zones;

* **Allocation Algorithm (`pmm_bitalloc_pgs`):** Scans the bitmap sequentially to find a contiguous block of `ip` free pages. Once found, those bits are set to `1` (allocated), and the physical base address ($i \times 4096$) is returned;

### 2. Virtual Memory Manager (VMM)
To provide isolated virtual address spaces for each process, Mini-UIA implements standard 4-Level Paging of the x86_64 architecture:
1. **PML4 (P4 - Page Map Level 4):** Shifts 39 bits, managing a 256 TB region;
2. **PDPT (Page Directory Pointer Table):** Shifts 30 bits, managing a 512 GB region;
3. **PD (Page Directory):** Shifts 21 bits, managing a 1 GB region;
4. **PT (Page Table):** Shifts 12 bits, managing 2 MB composed of 512 physical pages;

When mapping a virtual address via `vmm_mappg`, the kernel checks whether the intermediate page directories exist (`PAGE_PRESENT`). If missing, it requests a new page from the PMM, clears its content to `0`, and hooks the child directory to the parent table with proper privilege flags (`PAGE_USER`, `PAGE_WRITE`). Finally, the `invlpg` instruction is executed to flush the Translation Lookaside Buffer (TLB).

### 3. VMA & Radix Tree
To track allocated Virtual Memory Areas (`vma_struct`) for User Mode spaces, the kernel implements a high-performance **Radix Tree** instead of a traditional linked list.
* Each node (`radix_node`) branches into 16 children (`RADIX_CBUTTON = 16`), masked using a `0xF` bitmask (4-bit chunks);
* During a lookup (`radix_lookup`), the virtual address from bit 44 down to bit 12 is split into 4-bit indices to traverse child nodes. This ensures near-constant $O(1)$ lookup complexity, optimizing address space queries;

---

## CHAPTER II: PRIVILEGE SEGMENTATION & TASK STATE SEGMENT (TSS)

Mini-UIA separates execution privileges into Ring 0 (Kernel Mode) and Ring 3 (User Mode) to guarantee data security.

### 1. Global Descriptor Table (GDT)
The GDT (`gdt_table`) is configured with five static descriptors alongside one system TSS descriptor:
* `null` (0x00): Required by x86 hardware;
* `kcode` (0x08) & `kdata` (0x10): Ring 0 privileges for executing Kernel code and memory access;
* `udata` (0x20) & `ucode` (0x28): Ring 3 privileges (`DPL = 3`) for running user-space processes;

### 2. Task State Segment (TSS)
In 64-bit mode, the TSS is no longer used for hardware-based multitasking but serves as a security bridge for stack swapping during privilege elevation.
* When a Ring 3 process triggers an interrupt or a system call, the CPU automatically reads the `rsp0` field inside `tss_var` to switch the stack pointer to a secure, independent Kernel Stack (`kstack`);
* This prevents user-space processes from corrupting or overflowing kernel stacks;

---

## CHAPTER III: INTERRUPT DESCRIPTOR TABLE (IDT) & EXCEPTION HANDLING

The IDT (`idt_table`) maps 256 interrupt gates. System exceptions (ISR 0 to 19) are configured as Ring 0 gates (`0x8E`), while the system call gate (ISR 128) is marked with privilege `0xEE` to allow User Mode invocation.

### 1. Hardware Exception Traps
Critical hardware traps are captured using Assembly-based Naked ISRs to prevent CPU register pollution:
* **Division by Zero (ISR 0):** Intercepts division by zero, prints a red error message (`0x0C` VGA color) via `kprint`, and halts the CPU (`hlt`);
* **General Protection Fault (ISR 13):** Catches memory protection and descriptor access violations;
* **Page Fault (ISR 14):** Triggered when an unmapped or restricted memory page is accessed. The handler (`isr_pgfault`) reads the faulting virtual address directly from the `CR2` control register, displaying diagnostic info before locking the CPU;
* ...

### 2. Peripheral Interrupts (IRQs via LAPIC)
The kernel configures the Local APIC (LAPIC) to manage timer and device interrupts:
* **APIC Timer (IRQ 32 - `isr_timer`):** Triggered periodically, saves CPU context, and jumps to the scheduler (`process_schrun`) to execute multitasking. An EOI signal (`keoi_lapic`) is sent to the LAPIC to complete the cycle;
* **Keyboard Interrupt (IRQ 33 - `isr_keyboard`):** Reads raw keyboard Scancodes from I/O port `0x60`. It maps the scancodes to characters using `keyboard_table` and writes them directly into the VGA text buffer (`0xB8000`);

---

## CHAPTER IV: PROCESS MANAGEMENT & COOPERATIVE MULTITASKING

### 1. Process Control Block (PCB)
Each task is represented by a `process_struct` containing its execution state:
* `pid`: Unique task identifier;
* `p4_ptr`: Pointer to the PML4 page directory of the process;
* `status`: Current state (`PROCESS_READY`, `PROCESS_LOCK`, `PROCESS_RECV_BLOCKED`);
* `rsp`, `rip`, `ursp`: Stored CPU stack pointer and instruction pointer of the thread when suspended;
* `kstack`: The top boundary of the process's secure Ring 0 kernel stack;
* `mailbox`, `has_mess` && `msgbuf` : To send/recv (IPC); 

### 2. Context Switching Mechanism
Multi-tasking is driven by `process_switch`. this function have naked and noinline attributes and use `inline asm` to switch.

### 3. Scheduling Algorithm
The kernel runs a cooperative **Round-Robin scheduler**. When a timer tick triggers `process_schrun`, the scheduler traverses `process_queue` to find the next task in the `PROCESS_READY` state. Once targeted, the kernel loads its PML4 address into CR3 to swap the address space and calls process_switch to hand over the CPU.

---

## CHAPTER V: INTER-PROCESS COMMUNICATION (IPC) & SYSTEM CALLS
### 1. Synchronous and Blocking IPC Messaging
Mini-UIA provides message-passing IPC using a messenge_struct delivered directly to the receiver's mailbox[.

* `sysipc_send`: Copies payload data into the target process's mailbox and wakes up the process if it is blocked, shifting its status back to `PROCESS_READY`;

* `sysipc_recv`: If a thread checks its mailbox and finds no messages `(has_mess == 0 /* false = 0 */)`, it changes its status to PROCESS_RECV_BLOCKED and yields CPU time. It sleeps until an incoming message wakes it up;

### 2. High-Performance System Calls (Syscall/Sysret)
- => Waring: the current `syscall` failed;

