# 🚀 Mini-UIA Operating System

Hi everyone! I am Bang (nickname: BangGaming8826). I am a 5th-grade student and a self-taught systems programmer. This is my passion project: a 64-bit operating system built entirely from scratch!

## 🌟 Current Features of Mini-UIA:
- **Long Mode**: Successfully booting into 64-bit mode;
- **Memory Management (-- PMM --)**: Physical Memory Manager (PMM) using a Bitmap algorithm;
- **Memory Management (-- VMM --)**: Virtual Memory Manager (VMM) and Radix Tree;
- **Interrupts**: Configured IDT (Interrupt Descriptor Table);
- **Timer**: Local APIC Timer handling periodic interrupts perfectly;
- **Process/Scheduler && Scheduler**:  Manage Processes, Scheduler && Context Switching;
- **Keyboard Support**: PS/2 Keyboard IRQ1 driver is up and running via IO port `0x60`!
- **IPC**:  Complete multi-tasking core capable of managing independent Processes,
						Context Switching, and Task Scheduling;

## 🛠️ Tech Stack & Environment:
- **Code Editor/Environment**: Coded on a Huawei Mediapad T5 -> Termux -> Neovim;
- **Languages**: C and Assembly;

## (?) How to build?
=> Run `runner.sh` (bash scripts) to build (auto run QEMU);
=> Command to run:
`chmod +777 runner.sh`;
`./runner.sh`;
(i) Request: NASM, Clang/GCC Cross Compiler (x86_64), QEMU (qemu-system-x86_64);

## 🤝 Open for Collaboration!
Mini-UIA is a stepping stone towards my big dream of building a powerful OS for the future. I would be incredibly grateful for any advice, code reviews, or Pull Requests from senior developers worldwide to help me expand driver support (mouse, storage, network) and improve the kernel. 
