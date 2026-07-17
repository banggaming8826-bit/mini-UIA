#include "../include/kmanager.h"
#include "../include/kdefine.h"

// IDT (in C/C++):
struct idt_struct
{
	uint16b offset1;
	uint16b selector;
	uint8b  ist;
	uint8b  attr;
	uint16b offset2;
	uint32b offset3;
	uint32b zero;
} __attribute__((packed));
struct idt_ptrs {
	uint16b lim;
	uint64b base;
} __attribute__((packed));

struct idt_struct idt[256];
struct idt_ptrs idt_ptr;

extern void isr_divbyzero(void);
extern void isr_deb_breakpoint(void);
extern void isr_overflow(void);
extern void isr_out_range(void);
extern void isr_invalid_opcode(void);
extern void isr_devnava(void);
extern void isr_doublefault(void);
extern void isr_whattss(void);
extern void isr_segnpre(void);
extern void isr_stackseg(void);
extern void isr_gpf(void);
extern void isr_pgfault(void);
extern void isr_timer(void);
extern void isr_keyboard(void);
extern void isr_syscall(void);

void idt_setas(int i, uint64b handler, uint8b attr) 
{
	idt[i] = (struct idt_struct){ 
		(uint16b)(handler & 0xFFFF),
		0x08,
		0,
		attr,
		(uint16b)((handler >> 16) & 0xFFFF),
		(uint32b)((handler >> 32) & 0xFFFFFFFF),
		0
	};
}
void idt_setup(void)
{
	idt_ptr.base = (uint64b)idt;
	idt_ptr.lim  = (sizeof(struct idt_struct) * 256) - 1;

	idt_setas(0  , (uint64b)isr_divbyzero, 	0x8E);
	idt_setas(3  , (uint64b)isr_deb_breakpoint, 0x8E);
	idt_setas(4  , (uint64b)isr_overflow, 	0x8E);
	idt_setas(5  , (uint64b)isr_out_range, 	0x8E);
	idt_setas(6  , (uint64b)isr_invalid_opcode, 0x8E);
	idt_setas(7  , (uint64b)isr_devnava, 	0x8E);
	idt_setas(8  , (uint64b)isr_doublefault, 	0x8E);
	idt_setas(10 , (uint64b)isr_whattss, 	0x8E);
	idt_setas(11 , (uint64b)isr_segnpre, 	0x8E);
	idt_setas(12 , (uint64b)isr_stackseg, 	0x8E);
	idt_setas(13 , (uint64b)isr_gpf, 		0x8E);
	idt_setas(14 , (uint64b)isr_pgfault, 	0x8E);
	idt_setas(32 , (uint64b)isr_timer, 		0x8E);
	idt_setas(33 , (uint64b)isr_keyboard, 	0x8E);
	idt_setas(128, (uint64b)isr_syscall, 	0xEE); // hinh nhu chi co cai nay la 0xEE (bit DPL=3)
	
	asm volatile ("lidt %0" : : "m"(idt_ptr) : "memory");
}
// dich vu isr && irq
void isr_syscall_xl(int sysnum, ...)
{
	va_list va;
	va_start(va, sysnum);
	switch(sysnum)
	{
		case SYSCALL_EXIT: {
			process_curr->status = PROCESS_LOCK;
			process_schrun();
			asm volatile("hlt");
			break;
		}
		case SYSCALL_PRINT: {
			const char* str = va_arg(va, char*);
			kprint(str, 0x0F);
			break;
		}
		case SYSCALL_HPRINT: {
			int x = va_arg(va, int);
			kprint_hex(x, 0x0F);
			break;
		}
		case SYSCALL_SEND: {
			sysipc_send(
				va_arg(va, uint64b),
				va_arg(va, uint64b),
				va_arg(va, uint64b),
				va_arg(va, uint64b)
			);
			break;
		}
		case SYSCALL_RECV: {
			sysipc_recv(va_arg(va, struct messenge_struct*));
			break;
		}
	}
}
static const char keyboard_table[128] = (const char[128])
{
	0,  27, '1', '2', '3', '4', '5', '6', '7', '8','9', '0', 
	'-', '=', '\b', 									/* Backspace */
	'\t',                     									/* Tab */
	'q', 'w', 'e', 'r',   							/* 19?! wtf */
	't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 	/* Enter key */
	0,                    									/* Control */
	'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
	'\'', '`',   0,            							/* Left shift */
	'\\', 'z', 'x', 'c', 'v', 'b', 'n',
	'm', ',', '.', '/',   0,                          			/* Right shift */
	'*',
	0,  											/* Alt */
	' '  											/* Space bar */
};
void keyboard_handler(void)
{
	uint8b k = kreadio(0x60);
	if (!(k & 0x80))
	{
		if (k < 128) 
		{
			char ch = keyboard_table[k];
			if (ch) {
				char str[] = { ch, '\0' };
				kprint(str, 0x0F);
			}
		}
	}
}

struct gdt_struct
{
	uint16b limlow;
	uint16b baselow;
	uint8b  basemid;
	uint8b  access;
	uint8b  gran;
	uint8b  basehigh;
} __attribute__((packed));
struct gdt_tss_struct // dang le ra neu dung C++ thi da `: public gdt_struct' roi
{
	uint16b limlow;
	uint16b baselow;
	uint8b  basemid;
	uint8b  access;
	uint8b  gran;
	uint8b  basehigh;
	uint32b baseupp;
	uint32b reserved;
} __attribute__((packed));
struct gdtpack_struct
{
	struct gdt_struct null,
			  kcode,
			  kdata,
			  udata,
			  ucode;
	struct gdt_tss_struct tss;
} __attribute__((packed));
struct gdtptr_struct {
	uint16b lim;
	uint64b base;
} __attribute__((packed));

struct gdtpack_struct gdt_table;
struct gdtptr_struct  gdt_ptr;
struct tss_struct     tss_var;
uint8b kstack_r0[8192];

void gdt_tss_setup(void)
{
	gdt_ptr.lim	= sizeof(struct gdtpack_struct) - 1;
	gdt_ptr.base	= (uint64b)&gdt_table;

	// Mo ta (desc.) && TSS
	for (int i = 0; i < sizeof(struct tss_struct); i++) {
		((uint8b*)&tss_var)[i] = 0;
	}
	tss_var.rsp0 = (uint64b)&kstack_r0[8192];
	uint64b tss_base = (uint64b)&tss_var;
	uint64b tss_lim  = sizeof(struct tss_struct) - 1;

	gdt_table = (struct gdtpack_struct)
	{
		.null = {
			0, 0, 0,
			0, 0, 0 
		},
		.kcode = {
			0, 0, 0,
			0x9A, 0x20, 0
		},
		.kdata = {
			0, 0, 0,
			0x92, 0x00, 0 // 0x00=0
		},
		.udata = {
			0, 0, 0,
			0xF2, 0x00, 0 // 0x00=0
		},
		.ucode = {
			0, 0, 0,
			0xFA, 0x20, 0
		},
		.tss = (struct gdt_tss_struct)
		{
			tss_lim   & 0xFFFF,
			tss_base & 0xFFFF,
			(tss_base >> 16) & 0xFF,
			0x89,
			(tss_lim >> 16) & 0x0F,
			(tss_base >> 24) & 0xFF,
			(tss_base >> 32) & 0xFFFFFFFF,
			0
		}
	};
	asm volatile(
		"\tlgdt %0\n"
		"\tpush $0x08\n"
		"\tlea 1f(%%rip), %%rax\n"
		"\tpush %%rax\n"
		"\tlretq\n"
		"\t1:\n"
		"\tmov $0x10, %%ax\n"
		"\tmov %%ax, %%ds\n"
		"\tmov %%ax, %%es\n"
		"\tmov %%ax, %%fs\n"
		"\tmov %%ax, %%gs\n"
		"\tmov %%ax, %%ss\n"
		: : "m"(gdt_ptr) : "rax"
	);
	asm volatile("ltr %%ax" : : "a"(0x28));
}
void lapic_timer_init(uint32b tick) {
	kwrlapic(LAPIC_TIMER_DC, 3);
	kwrlapic(LAPIC_LVT_TIMER, 0x20000 | 32);
	kwrlapic(LAPIC_TIMER_IC, tick);
}

//test function
void what(void)
{
	while (1)
	{
		asm volatile(
			"\tmov $1, %%rax\n"
			"\tmov %0, %%rsi\n"
			"\tint $0x80\n"
			: : "r"("[]") : "rax", "rsi"
		);
		// Syscall Strom = 0
		for (int i = 0; i <= (900000ULL); i++) { asm volatile("nop"); }
	}
}
void who(void)
{
	while (1)
	{
		asm volatile(
			"\tmov $1, %%rax\n"
			"\tmov %0, %%rsi\n"
			"\tint $0x80\n"
			: : "r"("#") : "rax", "rsi"
		);
		// Syscall Strom = 0
		for (int i = 0; i <= (900000ULL); i++) { asm volatile("nop"); }
	}
}

extern void kmain(void)
{
	// Setup...
	uint16b n_byteram = *(uint16b*)0x7000;
	uint64b mxram = 0;
	struct E820r_struct* n_ramof = (struct E820r_struct*)0x8000;

	for (uint64b i = 0; i < n_byteram; i++)
	{
		if (n_ramof[i].type == 1) {
			uint64b be = n_ramof[i].base_addr + n_ramof[i].total;
			if (be > mxram) { mxram = be; }
		}
	}

	void* kernel_addr = (void*)KERNEL_ADDR;
	pmm_bitinit(mxram, kernel_addr);
	for (uint64b i = 0; i < n_byteram; i++)
	{
		if (n_ramof[i].type == 1) {
			uint64b start 	= n_ramof[i].base_addr / PAGE_SIZE;
			uint64b end	= (n_ramof[i].base_addr + n_ramof[i].total) / PAGE_SIZE;
			for (uint64b j = start; j < end; j++) { pmm_bitdel_pg(j); }
		}
	}
	uint64b lowa = KERNEL_ADDR / PAGE_SIZE; // == 256
	for (uint64b i = 0; i < lowa; i++) { pmm_bitset_pg(i); }
	uint64b b_start		= lowa, 
		b_byte		= (pmm_bitsize + 1) * 8,
		b_tongpg 	= (b_byte + PAGE_SIZE - 1) / PAGE_SIZE;
	for (uint64b j = b_start; j < (b_start + b_tongpg); j++) {
		pmm_bitset_pg(j);
	}

	uint64b cr3_value;
	asm volatile ("mov %%cr3, %0" : "=r"(cr3_value));
	uint64b* p4_ptr = (uint64b*)(cr3_value & PAGE_MASK);

	// GDT/TSS && IDT
	gdt_tss_setup();
	idt_setup();

	vmm_mappg(p4_ptr, LAPIC_BASE, LAPIC_BASE, 
			PAGE_PRESENT | PAGE_WRITE);
	kwriteio(0x21, 0xFF);
	kwriteio(0xA1, 0xFF);
	kwrlapic(LAPIC_SVR, 
			krdlapic(LAPIC_SVR) | 0x100 | 0xFF);
	kwrlapic(LAPIC_TPR, 0);
	lapic_timer_init(10000000);

	asm volatile("sti");
	
	// khoi dong...
	kclear(0x0F);
	kprint("Hello Everyone ^^ XD :)\n", 0x0F);
	kprint("S = { x thuoc N  / x < 256 }\n", 0x0F);
	kprint("S = { x thuoc N* / 90 * 4 < x < (90 << 3)\n", 0x0F);
	struct process_struct
		*whatp=process_init("Nguoi an xin", 10, what),
		*whop =process_init("Ban tay", 11, who);
	process_schadd(whatp);
	process_schadd(whop);
	process_run(whatp);
	while(1) {}
}
