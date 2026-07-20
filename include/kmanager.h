#ifndef __KMANAGER__
#define __KMANAGER__

#include "kdefine.h"

short* 	vga_buffer 		= (short*)0xB8000;
int	vga_index  		= 0;
uint64b total_byteram 		= 0;
uint64b syscall_kstack;

// "extern struct"
struct tss_struct
{
	uint32b reserved0;
	uint64b rsp0;	// con tro stack rsp ring 0
	uint64b rsp1;
	uint64b rsp2;
	uint64b reserved1;
	uint64b ist[7];
	uint64b reserved2;
	uint16b reserved3;
	uint16b iomap;
} __attribute__((packed));

// unlitity tools
// (ko dung kstatus_t, chi dung void)
size_t kstrlen(const char* str)
{
	if (!str) { return 0; }
	size_t i = 0;
	while (*(str++)) { i++; }
	return i;
}
static inline void kwriteio(uint16b cong, uint8b gt) {
	asm volatile ("outb %0, %1" : : "a"(gt), "Nd"(cong));
}
static inline uint8b kreadio(uint16b p) {
	uint8b ret;
	asm volatile("inb %w1, %0" : "=a"(ret) : "Nd"(p));
	return ret;
}
static inline uint64b krdmsr(uint32b m) {
	uint32b low, high;
	asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(m));
	return ((uint64b)high << 32) | low;
}
static inline void kwrmsr(uint32b m, uint64b v) {
	uint32b low = (uint32b)v, high = (uint32b)(v >> 32);
	asm volatile("wrmsr" : : "c"(m), "a"(low), "d"(high));
}
static inline void kwrlapic(uint32b thanh_ghi, uint32b gt) {
	volatile uint32b* addr = (volatile uint32b*)(LAPIC_BASE + thanh_ghi);
	*addr = gt;
}
static inline uint32b krdlapic(uint32b thanh_ghi) {
	volatile uint32b* addr = (volatile uint32b*)(LAPIC_BASE + thanh_ghi);
	return *addr;
}
static inline void kwriteiol(uint16b cong, uint32b gt) {
	asm volatile("outl %0, %1" : : "a"(gt), "Nd"(cong));
}
static inline uint32b kreadiol(uint16b cong) {
	uint32b ret;
	asm volatile("inl %w1, %0" : "=a"(ret) : "Nd"(cong));
	return ret;
}
void keoi_lapic(void) { kwrlapic(LAPIC_EOI, 0); }

void kclear(int color) {
	for (int i = 0; i < VGA_HEIGHT * VGA_WEIGHT; i++) {
		vga_buffer[i] = (color << 8) | ' ';
	}
}
void kprint(const char* mess, int color) 
{
	size_t len = kstrlen(mess);
	for (int i = 0; i < len; i++) 
	{
		if (mess[i] == '\n') { 
			vga_index = (vga_index / VGA_WEIGHT + 1) * VGA_WEIGHT; 
			continue;
		}
		else if (vga_index >= VGA_HEIGHT * VGA_WEIGHT) {
			break;
		}
		vga_buffer[vga_index++] = (color << 8) | mess[i];
	}
}
void kprint_hex(unsigned long long x, int color)
{
	if (!x) {
		kprint("0x0", color);
		return;
	}
	static const char thuvien[] = "0123456789ABCDEF";
	static char buffer[30];
	int i = 29;
	buffer[29] = '\0';
	while (x > 0) {
		i--;
		buffer[i] = thuvien[x & 0xF];
		x >>= 4;
	}
	kprint("0x", color);
	kprint(&buffer[i], color);
}

// Ram && Bo nho
// (?) de lay so luong ram
struct E820r_struct
{
	uint64b base_addr;	// dia chi co so
	uint64b total;		// (*) do dai/lon/tong byte ram
	uint32b type;		// (?) cho nay cua ai?
	uint32b attribute;	// (?) what?
} __attribute__((packed));

uint64b* pmm_bitaddr = NULL,
	 pmm_bitmax_pg = 0,
	 pmm_bitsize = 0;

// A. PMM (Bo nho thuc):

kstatus_t pmm_bitinit(uint64b msize, void* dia_chi)
{
	if (!dia_chi) { return KSTATUS_ERR; }
	pmm_bitaddr 	= (uint64b*)dia_chi;
	pmm_bitmax_pg 	= msize / PAGE_SIZE;
	pmm_bitsize 	= pmm_bitmax_pg / 64;
	for (uint64b i = 0; i < pmm_bitsize + 1; i++) {
		pmm_bitaddr[i] = 0xFFFFFFFFFFFFFFFFULL; // Moi khoi tao...
	}
	return KSTATUS_OK;
}
kstatus_t pmm_bitset_pg(uint64b i) {
	uint64b hic = i % 64;
	pmm_bitaddr[i / 64] |= (1ULL << hic);
	return KSTATUS_OK;
}
// "Sao cai cong thuc nay giong ..."
kstatus_t pmm_bitdel_pg(uint64b i) {
	uint64b hic = i % 64;
	pmm_bitaddr[i / 64] &= ~(1ULL << hic);
	return KSTATUS_OK;
}
// RAM
void* pmm_bitalloc_pg(void) // cap phat trang
{
	for (uint64b i = 0; i < pmm_bitmax_pg; i++)
	{
		uint64b j = i / 64, hic = i % 64;
		if ((pmm_bitaddr[j] & (1ULL << hic)) == 0) {
			pmm_bitset_pg(i);
			return (void*)(i * PAGE_SIZE);
		}
	}
	return NULL;
}
void* pmm_bitalloc_pgs(uint64b ip)
{
	if (!ip || ip > pmm_bitmax_pg) { return NULL; }
	for (uint64b i = 0; i <= (pmm_bitmax_pg - ip); i++) // j < (max - i + 1)
	{
		kstatus_t ok = 1;
		for (uint64b j = 0; j < ip; j++) 
		{
			uint64b cong 	= i + j;
			uint64b hmm	= cong % 64;
			if ((pmm_bitaddr[cong / 64] & (1ULL << hmm)) != 0) {
				ok = 0;
				break;
			} // trang ko trong => ok = false
		}
		if (ok) /* trong */ {
			for (uint64b j = 0; j < ip; j++) {
				pmm_bitset_pg(i + j);
			}
			return (void*)(i * PAGE_SIZE);
		}
	}
	// ko du
	return NULL;
}
kstatus_t pmm_bitfree_pg(void* ptr) {
	return pmm_bitdel_pg(((uint64b)ptr) / PAGE_SIZE);
}

// B. VMM

// radix
struct radix_node {
	// (?) 1 dia chi bo nho co gi? Gia tri va dia chi/node tiep...
	void* value;
	struct radix_node* tre_con[RADIX_CBUTTON];
};
struct radix_node* radix_init(void)
{
	struct radix_node* a = (struct radix_node*)(pmm_bitalloc_pg());
	if (a) {
		for (int i = 0; i < RADIX_CBUTTON; i++) {
			a->tre_con[i] = NULL;
		}
		a->value = NULL;
	}
	return a;
}
kstatus_t radix_insert(struct radix_node* cay, uint64b addr, void* value)
{
	struct radix_node* curr = cay;
	for (int i = 44; i >= 12; i -= 4)
	{
		int chaian = (addr >> i) & RADIX_MASK;
		if (!(curr->tre_con[chaian])) {
			curr->tre_con[chaian] = radix_init();
		}
		curr = curr->tre_con[chaian];
	}
	curr->value = value;
	return KSTATUS_OK;
}
void* radix_lookup(struct radix_node* cay, uint64b addr)
{
	struct radix_node* curr = cay;
	for (int i = 44; i >= 12; i -= 4) {
		int suneo = (addr >> i) & RADIX_MASK;
		if (!(curr->tre_con[suneo])) { return NULL; }
		curr = curr->tre_con[suneo];
	}
	return curr->value;
}
//pages
kstatus_t vmm_mappg(uint64b* p4_ptr, uint64b addr, uint64b paddr, uint64b f)
{
	if (!p4_ptr) { return KSTATUS_ERR; }
	uint64b p4i	= (addr >> 39) & 0x1FF,
		pdpti	= (addr >> 30) & 0x1FF,
		pdi	= (addr >> 21) & 0x1FF,
		pti	= (addr >> 12) & 0x1FF;
	// duyet...
	if (!(p4_ptr[p4i] & PAGE_PRESENT))
	{
		uint64b* muoi_diem = (uint64b*)pmm_bitalloc_pg();
		for (int i = 0; i < 512; i++) { muoi_diem[i] = 0; }
		p4_ptr[p4i] = ((uint64b)muoi_diem) 
				| PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
	}
	uint64b* pdpt = (uint64b*)(p4_ptr[p4i] & PAGE_MASK);
	if (!(pdpt[pdpti] & PAGE_PRESENT)) 
	{
		uint64b* tuyen_quang = (uint64b*)pmm_bitalloc_pg();
		for (int i = 0; i < 512; i++) { tuyen_quang[i] = 0; }
		pdpt[pdpti] = ((uint64b)tuyen_quang) 
				| PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
	}
	uint64b* pd = (uint64b*)(pdpt[pdpti] & PAGE_MASK);
	if (!(pd[pdi] & PAGE_PRESENT))
	{
		uint64b* thi_lai = (uint64b*)pmm_bitalloc_pg();
		for (int i = 0; i < 512; i++) { thi_lai[i] = 0; }
		pd[pdi] = ((uint64b)thi_lai) 
				| PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
	}
	uint64b* pt = (uint64b*)(pd[pdi] & PAGE_MASK);
	pt[pti] = (paddr & PAGE_MASK) | PAGE_PRESENT | PAGE_WRITE | f;
	asm volatile("invlpg (%0)" : : "r"(addr) : "memory");
	return KSTATUS_OK;
}
kstatus_t vmm_mappgs(uint64b c, uint64b* p4_ptr, uint64b addr, uint64b paddr, uint64b f)
{
	if (!p4_ptr || !c) { return KSTATUS_ERR; }
	for (uint64b i = 0; i < c; i++) {
		uint64b ao = addr + (i * PAGE_SIZE), thuc = paddr + (i * PAGE_SIZE);
		if (vmm_mappg(p4_ptr, ao, thuc, f) == KSTATUS_ERR) { return KSTATUS_ERR; }
	}
	return KSTATUS_OK;
}
struct vma_struct;
kstatus_t vmm_unmappg(struct radix_node* cay, uint64b* p4_ptr, uint64b addr)
{
	if (!p4_ptr) { return KSTATUS_ERR; }
	uint64b p4i	= (addr >> 39) & 0x1FF,
		pdpti	= (addr >> 30) & 0x1FF,
		pdi	= (addr >> 21) & 0x1FF,
		pti	= (addr >> 12) & 0x1FF;
	if (!(p4_ptr[p4i] & PAGE_PRESENT)) { return KSTATUS_ERR; }
	uint64b* pdpt = (uint64b*)(p4_ptr[p4i] & PAGE_MASK);

	if (!(pdpt[pdpti] & PAGE_PRESENT)) { return KSTATUS_ERR; }
	uint64b* pd = (uint64b*)(pdpt[pdpti] & PAGE_MASK);

	if (!(pd[pdi] & PAGE_PRESENT)) { return KSTATUS_ERR; }
	uint64b* pt = (uint64b*)(pd[pdi] & PAGE_MASK);

	if (!(pt[pti] & PAGE_PRESENT)) { return KSTATUS_ERR; }
	uint64b paddrf = pt[pti] & PAGE_MASK;
	pmm_bitfree_pg((void*)paddrf);
	pt[pti] = 0;
	if (cay)
	{
		struct vma_struct* v = (struct vma_struct*)radix_lookup(cay, addr);
		if (v) {
			pmm_bitfree_pg((void*)v);
			radix_insert(cay, addr, NULL);
		}
	}
	asm volatile("invlpg (%0)" : : "r"(addr) : "memory");
	return KSTATUS_OK;
}
//vma
struct vma_struct 
{
	uint64b start_addr;
	uint64b end_addr;
	uint64b flags;
	const char* name; // ID?!
};
kstatus_t vmm_allocvma(
		struct radix_node* cay, uint64b* p4_ptr, 
		uint64b addr, uint64b paddr, uint64b f,
		const char* strname) 
{
	if (!strname || !cay || !p4_ptr) { return KSTATUS_ERR; }
	if (vmm_mappg(p4_ptr, addr, paddr, f) == KSTATUS_ERR) {
		return KSTATUS_ERR;
	}
	struct vma_struct* ptr = (struct vma_struct*)pmm_bitalloc_pg();
	if (!ptr) { return KSTATUS_ERR; }
	ptr->start_addr = addr,
	ptr->end_addr	= addr + PAGE_SIZE,
	ptr->flags	= f,
	ptr->name	= strname;
	if (radix_insert(cay, addr, (void*)ptr) == KSTATUS_ERR) {
		pmm_bitfree_pg((void*)ptr);
		return KSTATUS_ERR;
	}
	return KSTATUS_OK;
}
struct vma_struct* vmm_getvma(struct radix_node* cay, uint64b addr) {
	if (!cay) { return NULL; }
	struct vma_struct* v = (struct vma_struct*)radix_lookup(cay, addr);
	return (v && (addr >= v->start_addr && addr < v->end_addr)) ? v : NULL;
}
uint64b* vmm_newspace(void)
{
	uint64b* p4 = (uint64b*)pmm_bitalloc_pg();
	if (!p4) { return NULL; }
	for (int i = 0; i < 512; i++) { p4[i] = 0; }
	uint64b cr3;
	asm volatile("mov %%cr3, %0" : "=r"(cr3));
	uint64b* rp4 = (uint64b*)(cr3 & PAGE_MASK);
	for (int i = 0; i < 512; i++) { p4[i] = rp4[i]; }
	return p4;
}
kstatus_t vmm_switch_space(uint64b* page) {
	if (!page) { return KSTATUS_ERR; }
	asm volatile("mov %0, %%cr3" : : "r"((uint64b)page) : "memory");
	return KSTATUS_OK;
}

// Process && Managers
//A. Cau truc

struct messenge_struct
{
	uint64b senderid;
	uint64b type;
	uint64b data1;
	uint64b data2;
} __attribute__((packed));
struct process_struct
{
	uint64b pid;
	uint64b* p4_ptr;
	kstatus_t status;
	const char* pname;
	uint64b rsp, rip, kstack, ursp;
	// thu cua tien trinh...
	struct messenge_struct msg_queue[IPC_QUEUE_MAX];
	size_t msg_head;		// "Index" lay tin nhan ra (doc)
	size_t msg_tail;		// "Index" lay tin nan vao (ghi)
	size_t msg_count;		// SL.
	struct messenge_struct* msgbuf;
};
struct driver_struct
{
	const char* 	name;
	uint64b		owner_pid;
	uint16b 	ioport_start;
	uint16b 	ioport_end;
	uint8b  	irq;
	kstatus_t 	actived;
};

//B. Method && global::var
kstatus_t	process_run(struct process_struct*);
void 		process_end(void); // hock ... VVV
__attribute__((naked)) void process_start_init(void) 
{
	asm volatile(
		"\tpop %%r15\n"
		"\tpop %%r14\n"
		"\tpop %%r13\n"
		"\tpop %%r12\n"
		"\tpop %%r11\n"
		"\tpop %%r10\n"
		"\tpop %%r9\n"
		"\tpop %%r8\n"
		"\tpop %%rbp\n"
		"\tpop %%rdi\n"
		"\tpop %%rsi\n"
		"\tpop %%rdx\n"
		"\tpop %%rcx\n"
		"\tpop %%rbx\n"
		"\tpop %%rax\n"
		"\tiretq\n" : : : "memory"
	);
}
struct process_struct* process_init(const char* name, uint64b id, void* function)
{
	struct process_struct* p = (struct process_struct*)pmm_bitalloc_pg();
	if (!p) { return NULL; }
	p->pid = id;
	p->status = PROCESS_READY;
	p->pname = name;
	p->p4_ptr = vmm_newspace();
	if (!(p->p4_ptr)) {
		pmm_bitfree_pg((void*)p);
		return NULL;
	}
	void* ustack = pmm_bitalloc_pg();
	if (!ustack) {
		pmm_bitfree_pg(p->p4_ptr);
		pmm_bitfree_pg(p);
		return NULL;
	}
	void* kstack = pmm_bitalloc_pg();
	if (!kstack) 
	{
		pmm_bitfree_pg(ustack);
		pmm_bitfree_pg(p->p4_ptr);
		pmm_bitfree_pg(p);
		return NULL;
	}
	p->kstack = (uint64b)kstack + PAGE_SIZE;

	vmm_mappg(
		p->p4_ptr, 
		(uint64b)ustack, (uint64b)ustack, 
		PAGE_PRESENT | PAGE_WRITE | PAGE_USER
	);
	uint64b fpage = (uint64b)function & PAGE_MASK;
	vmm_mappg(p->p4_ptr, fpage, fpage, PAGE_USER);
	uint64b pe_page = (uint64b)process_end & PAGE_MASK;
	vmm_mappg(p->p4_ptr, pe_page, pe_page, PAGE_USER);

	// "dao lua" cpu
	uint64b* rsp = (uint64b*)((uint64b)ustack + PAGE_SIZE);
	*(--rsp) = (uint64b)process_end;
	uint64b ursp = (uint64b)rsp;

	*(--rsp) = 0x1B;
	*(--rsp) = ursp;
	*(--rsp) = 0x202;
	*(--rsp) = 0x23;
	*(--rsp) = (uint64b)function;
	for (int __ = 0; __ < 15; __++) { *(--rsp) = 0; }
	*(--rsp) = (uint64b)process_start_init;
	for (int __ = 0; __ < 6; __++) { *(--rsp) = 0; }
	p->rsp = (uint64b)(rsp);
	p->msg_count = 0;
	p->msg_head  = 0;
	p->msg_tail  = 0;

	return p;
}
__attribute__((noinline)) __attribute__((naked))
	void process_switch(uint64b*, uint64b);
struct process_struct* process_queue[PROCESS_MAXCO];
size_t process_num 			= 0;
size_t process_currin			= 0;
struct process_struct* process_curr	= NULL;
extern struct tss_struct tss_var;
kstatus_t process_run(struct process_struct* p)
{
	if (!p) { return KSTATUS_ERR; }
	process_curr = p;
	tss_var.rsp0 = p->kstack;
	syscall_kstack = p->kstack;

	vmm_switch_space(p->p4_ptr);
	uint64b rac;
	process_switch(&rac, p->rsp);
	return KSTATUS_OK;
}
void process_end(void) {
	asm volatile(
		"\tmov $0, %%rax\n"
		"\tint $0x80\n" : : : "rax"
	);
}
kstatus_t process_schadd(struct process_struct* p)
{
	if (!p || process_num >= PROCESS_MAXCO) { return KSTATUS_ERR; }
	process_queue[process_num++] = p;
	return KSTATUS_OK;
}
__attribute__((noinline)) __attribute__((naked))
void process_switch(uint64b* orsp, uint64b nrsp)
{
	asm volatile(
		"\tpush %%rbp\n"
		"\tpush %%rbx\n"
		"\tpush %%r12\n"
		"\tpush %%r13\n"
		"\tpush %%r14\n"
		"\tpush %%r15\n"

		"\tmov %%rsp, (%%rdi)\n"
		"\tmov %%rsi, %%rsp\n"

		"\tpop %%r15\n"
		"\tpop %%r14\n"
		"\tpop %%r13\n"
		"\tpop %%r12\n"
		"\tpop %%rbx\n"
		"\tpop %%rbp\n"

		"\tret\n"
		: : : "memory"
	);
}
void process_schrun(void)
{
	if (process_num <= 1) { return; }
	struct process_struct* ptr = process_curr;

	int i = (process_currin + 1) % process_num;
	while (process_queue[i]->status != PROCESS_READY) {
		i = (i + 1) % process_num;
		if (i == process_currin) { return; }
	}

	process_currin = i;
	process_curr = process_queue[process_currin];

	tss_var.rsp0 = process_curr->kstack;
	syscall_kstack = process_curr->kstack;

	vmm_switch_space(process_curr->p4_ptr);
	process_switch(&(ptr->rsp), process_curr->rsp);
}
struct process_struct* process_getbyid(uint64b pid) 
{
	for (size_t i = 0; i < process_num; i++) {
		if (process_queue[i]->pid == pid) { return process_queue[i]; }
	}
	return NULL;
}
kstatus_t sysipc_send(uint64b pid_dich, uint64b type, uint64b data1, uint64b data2) 
{
	struct process_struct* pdich = process_getbyid(pid_dich);
	if (!pdich || pdich->msg_count >= IPC_QUEUE_MAX) { return KSTATUS_ERR; }

	size_t tail = pdich->msg_tail;
	pdich->msg_queue[tail].senderid	= process_curr->pid;
	pdich->msg_queue[tail].type	= type;
	pdich->msg_queue[tail].data1	= data1;
	pdich->msg_queue[tail].data2	= data2;
	pdich->msg_tail = (tail + 1) % IPC_QUEUE_MAX;
	pdich->msg_count++;

	if (pdich->status == PROCESS_RECV_BLOCKED)
	{
		pdich->status = PROCESS_READY;
		struct process_struct* ptr = process_curr;
		process_curr 	= pdich;
		tss_var.rsp0 	= pdich->kstack;
		syscall_kstack 	= pdich->kstack;
		vmm_switch_space(pdich->p4_ptr);
		process_switch(&(ptr->rsp), pdich->rsp);
	}
	return KSTATUS_OK;
}
kstatus_t sysipc_recv(struct messenge_struct* m)
{
	if (!m) { return KSTATUS_ERR; }
	if (process_curr->msg_count > 0)
	{
		size_t head = process_curr->msg_head;
		*m = process_curr->msg_queue[head];
		process_curr->msg_head = (head + 1) % IPC_QUEUE_MAX;
		process_curr->msg_count--;
	}
	else
	{
		process_curr->status = PROCESS_RECV_BLOCKED;
		process_curr->msgbuf = m;
		process_schrun();
		if (process_curr->msg_count > 0)
		{
			size_t head = process_curr->msg_head;
			struct messenge_struct p = process_curr->msg_queue[head];
			m->senderid	= p.senderid;
			m->type		= p.type;
			m->data1	= p.data1;
			m->data2	= p.data2;
			process_curr->msg_head = (head + 1) % IPC_QUEUE_MAX;
			process_curr->msg_count--;
		}
	}
	return KSTATUS_OK;
}

struct driver_struct driver_table[DRIVER_MAXC];
size_t driver_count = 0;
kstatus_t driver_sub(
	const char* name, uint64b id, uint16b start, uint16b end, uint8b irq) 
{
	if (driver_count >= DRIVER_MAXC) { return KSTATUS_ERR; }
	struct driver_struct* ptr = &driver_table[driver_count++];
	ptr->name 		= name;
	ptr->owner_pid 		= id;
	ptr->ioport_start 	= start;
	ptr->ioport_end 	= end;
	ptr->irq		= irq;
	ptr->actived 		= 0;

	return KSTATUS_OK;
}

#endif // __KMANAGER__ // 
