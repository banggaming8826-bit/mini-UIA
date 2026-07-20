[BITS 16];
global start;

start:
	xor ax, ax;
	mov ds, ax;
	mov es, ax;
	mov ss, ax;
	mov sp, 0x7C00;

	; cmm nha bios, luoi it thoi!
	mov ah, 0x02;
	mov al, 75;
	mov ch, 0;
	mov cl, 2;
	mov dh, 0;
	mov bx, 0x7E00;
	int 0x13;

	; ... ;
	mov edi, 0x8000;
	xor ebx, ebx;
	xor bp, bp;
.getram_byte:
	mov eax, 0xE820;
	mov edx, 0x534D4150;
	mov ecx, 24;
	int 0x15;

	jc .next;
	cmp eax, 0x534D4150;
	jne .next;

	add edi, 24;
	inc bp;
	test ebx, ebx;
	jne .getram_byte;
.next:
	mov [0x7000], bp;
	cli;

	; Protected Mode (x32):
	lgdt [gdt_des];
	mov eax, cr0;
	or eax, 1;
	mov cr0, eax;

	jmp 0x08:alo_x32; cung cha khac me
[BITS 32];
alo_x32:
	mov ax, 0x10;
	mov ds, ax;
	mov ss, ax;
	mov es, ax;
	mov fs, ax;
	mov gs, ax;

	; Long Mode (x64):
	mov edi, 0x1000;
	mov cr3, edi;
	xor eax, eax;
	mov ecx, 3072;
	rep stosd;

	mov dword [0x1000], 0x2007;
	mov dword [0x2000], 0x3007;
	mov dword [0x3000], 0x87;

	;PAE ?! Peak!
	mov eax, cr4;
	or eax, 1 << 5;
	mov cr4, eax;
	;What the Long mode and EFER MSR?
	mov edi, 0x1000;
	mov cr3, edi;

	mov ecx, 0xC0000080;
	rdmsr;
	or eax, 1 << 8;
	wrmsr;
	;Paging in cr0? wtf!
	mov eax, cr0;
	or eax, 1 << 31;
	mov cr0, eax;

	lgdt [gdt_des];
	jmp 0x18:alo_x64;

	; extern C/C++
	extern kmain;
[BITS 64];
alo_x64:
	xor ax, ax;
	mov ds, ax;
	mov es, ax;
	mov fs, ax;
	mov gs, ax;
	mov ss, ax;

	mov rsp, 0x20000;

	; "sse, sso"
	mov rax, cr0;
	and ax, 0xFFFB;
	or ax, 0x0002;
	mov cr0, rax;

	mov rax, cr4;
	or eax, 1 << 9;
	or eax, 1 << 10;
	mov cr4, rax;

	sub rsp, 8;
	call kmain;
	hlt;
	jmp $;

align 8;
gdt_start:
	dq 0;
gdt_code:
	dq 0x00CF9A000000FFFF;
gdt_data:
	dq 0x00CF92000000FFFF;
gdt_code64:
	dq 0x00209A0000000000;
gdt_usr_data64:
	dq 0x0000F20000000000;
gdt_usr_code64:
	dq 0x0020FA0000000000;
gdt_end:;,
gdt_des:
	dw gdt_end - gdt_start - 1;
	dq gdt_start;

times 510 - ($ - $$) db 0;
dw 0xAA55;
