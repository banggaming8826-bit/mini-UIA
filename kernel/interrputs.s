; interrputs = interrupts
extern kprint;
extern kprint_hex;
section .data;

; Division by Zero (x / 0), x thuoc Z
global isr_divbyzero;
msg_div: db "(!) boxError: Division by zero!", 10, 0; (10 = '\n')
isr_divbyzero:
	push rax;
	push rdi;
	push rsi;
	
	lea rdi, [rel msg_div];
	mov rsi, 0x0C;
	call kprint;

	pop rsi;
	pop rdi;
	pop rax;

	hlt;
	iretq;

; GPT 
global isr_gpf;
msg_gpf: 
	db "(!) boxError: General Protection Fault!", 10;
	db "=> ErrorCode: ", 0; (10 = '\n'
msg_gpf2: db 10, 0;
isr_gpf:
	push rbx;
	mov rbx, [rsp + 8];
	push rax;
	push rdi;
	push rsi;

	lea rdi, [rel msg_gpf];
	mov rsi, 0x0C;
	call kprint;

	mov rdi, rbx;
	mov rsi, 0x0C;
	call kprint_hex;
	lea rdi, [rel msg_gpf2];
	mov rsi, 0x0C;
	call kprint;

	pop rsi;
	pop rdi;
	pop rax;
	pop rbx;
	add rsp, 8;

	hlt;
	iretq;

; [deb] break point (bp)
global isr_deb_breakpoint;
msg_deb_breakpoint: db "(i) boxDebug: Breakpoint hit (ed)", 10, 0; (10 = '\n')
isr_deb_breakpoint:
	push rax;
	push rdi;
	push rsi;
	
	lea rdi, [rel msg_deb_breakpoint];
	mov rsi, 0x09;
	call kprint;

	pop rsi;
	pop rdi;
	pop rax;
	iretq;
	
; overflow
global isr_overflow;
msg_overflow: db "(!) boxWarn: Overflowed!", 10, 0; (10 = '\n')
isr_overflow:
	push rax;
	push rdi;
	push rsi;
	
	lea rdi, [rel msg_overflow];
	mov rsi, 0x0E;
	call kprint;

	pop rsi;
	pop rdi;
	pop rax;
	iretq;

; out of range
global isr_out_range;
msg_out_range: db "(!) boxError: Out of Range!", 10, 0; (10 = '\n')
isr_out_range:
	push rax;
	push rdi;
	push rsi;
	
	lea rdi, [rel msg_out_range];
	mov rsi, 0x0C;
	call kprint;

	pop rsi;
	pop rdi;
	pop rax;
	iretq;

; invalid opcode
global isr_invalid_opcode;
msg_invopcode: db "(!) boxError: Invalid Opcode!", 10, 0;
isr_invalid_opcode:
	push rax;
	push rdi;
	push rsi;

	lea rdi, [rel msg_invopcode];
	mov rsi, 0x0C;
	call kprint;

	pop rsi;
	pop rdi;
	pop rax;

	hlt;
	iretq;

; device not ava. (#nm)
global isr_devnava;
msg_devnava:
	db "(!) boxError: Device not available (#NM; Unknown)", 10, 0;
isr_devnava:
	push rax;
	push rdi;
	push rsi;

	lea rdi, [rel msg_devnava];
	mov rsi, 0x0C;
	call kprint;

	pop rsi;
	pop rdi;
	pop rax;

	hlt;
	iretq;

; double fault
global isr_doublefault;
msg_doublefault: 
	db "(!) boxError: Double Fault!", 10, 0;
	db "=> ErrorCode: ", 0;
msg_doublefault2: db 10, 0;
isr_doublefault:
	push rbx;
	mov rbx, [rsp + 8];
	push rax;
	push rdi;
	push rsi;

	lea rdi, [rel msg_doublefault];
	mov rsi, 0x0C;
	call kprint;

	mov rdi, rbx;
	mov rsi, 0x0C;
	call kprint_hex;
	lea rdi, [rel msg_doublefault2];
	mov rsi, 0x0C;
	call kprint;

	pop rsi;
	pop rdi;
	pop rax;
	pop rbx;
	add rsp, 8;

	hlt;
	iretq;

; invalid task state seg.
global isr_whattss;
msg_whattss: 
	db "(!) boxError: Invalid Tss (What?)", 10, 0;
	db "=> ErrorCode: ", 0;
msg_whattss2: db 10, 0;
isr_whattss:
	push rbx;
	mov rbx, [rsp + 8];
	push rax;
	push rdi;
	push rsi;

	lea rdi, [rel msg_whattss];
	mov rsi, 0x0C;
	call kprint;

	mov rdi, rbx;
	mov rsi, 0x0C;
	call kprint_hex;
	lea rdi, [rel msg_whattss2];
	mov rsi, 0x0C;
	call kprint;

	pop rsi;
	pop rdi;
	pop rax;
	pop rbx;
	add rsp, 8;

	hlt;
	iretq;

; seg '!' present
global isr_segnpre;
msg_segp: 
	db "(!) boxError: Segment not present (What?)", 10, 0;
	db "=> ErrorCode: ", 0;
msg_segp2: db 10, 0;
isr_segnpre:
	push rbx;
	mov rbx, [rsp + 8];
	push rax;
	push rdi;
	push rsi;

	lea rdi, [rel msg_segp];
	mov rsi, 0x0C;
	call kprint;

	mov rdi, rbx;
	mov rsi, 0x0C;
	call kprint_hex;
	lea rdi, [rel msg_segp2];
	mov rsi, 0x0C;
	call kprint;

	pop rsi;
	pop rdi;
	pop rax;
	pop rbx;
	add rsp, 8;

	hlt;
	iretq;

; stack segment fault
global isr_stackseg;
msg_stackseg: 
	db "(!) boxError: Stack segment error", 10, 0;
	db "=> ErrorCode: ", 0;
msg_stackseg2: db 10, 0;
isr_stackseg:
	push rbx;
	mov rbx, [rsp + 8];
	push rax;
	push rdi;
	push rsi;

	lea rdi, [rel msg_stackseg];
	mov rsi, 0x0C;
	call kprint;

	mov rdi, rbx;
	mov rsi, 0x0C;
	call kprint_hex;
	lea rdi, [rel msg_stackseg2];
	mov rsi, 0x0C;
	call kprint;

	pop rsi;
	pop rdi;
	pop rax;
	pop rbx;
	add rsp, 8;
	
	hlt;
	iretq;
	
; page fault fault
global isr_pgfault;
msg_pgfault: 
	db "(!) boxError: Page Fault error", 10, 0;
	db "=> ErrorCode (CR2): ", 0;
msg_pgfault2: db 10, 0;
isr_pgfault:
	push rbx;
	mov rbx, [rsp + 8];
	push rax;
	push rdi;
	push rsi;

	lea rdi, [rel msg_pgfault];
	mov rsi, 0x0C;
	call kprint;

	mov rax, cr2;
	mov rdi, rax;
	mov rsi, 0x0C;
	call kprint_hex;

	lea rdi, [rel msg_pgfault2];
	mov rsi, 0x0C;
	call kprint;

	pop rsi;
	pop rdi;
	pop rax;
	pop rbx;
	add rsp, 8;
	
	hlt;
	iretq;

; SYSCALL (128)
extern isr_syscall_xl;
; int $0x80
global isr_syscall;
isr_syscall:
	push rdi;
	push rsi;
	push rax;
	push rbx;
	push rcx;
	push rdx;
	push r8;
	push r9;
	push r10;
	push r11;

	; ... ;
	mov rdi, rax;
	xor eax, eax;
	call isr_syscall_xl;

	pop r11;
	pop r10;
	pop r9;
	pop r8;
	pop rdx;
	pop rcx;
	pop rbx;
	pop rax;
	pop rsi;
	pop rdi;
	iretq;
; IRQ
extern process_curr;
extern process_schrun;
extern keoi_lapic;
; timer

global isr_timer;
isr_timer:
	push rax;
	push rbx;
	push rcx;
	push rdx;
	push rsi;
	push rdi;
	push rbp;
	push r8;
	push r9;
	push r10;
	push r11;
	push r12;
	push r13;
	push r14;
	push r15;

	call keoi_lapic;
	call process_schrun;

	pop r15;
	pop r14;
	pop r13;
	pop r12;
	pop r11;
	pop r10;
	pop r9;
	pop r8;
	pop rbp;
	pop rdi;
	pop rsi;
	pop rdx;
	pop rcx;
	pop rbx;
	pop rax;

	iretq;
; keyboard:
global isr_keyboard;
extern keyboard_handler;
isr_keyboard:
	push rax;
	push rdi;
	push rsi;
	push rdx;
	push rcx;
	push r8;
	push r9;
	push r10;
	push r11;

	call keyboard_handler;
	call keoi_lapic;

	pop r11;
	pop r10;
	pop r9;
	pop r8;
	pop rcx;
	pop rdx;
	pop rsi;
	pop rdi;
	pop rax;

	iretq;
