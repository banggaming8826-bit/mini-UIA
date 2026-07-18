#ifndef __KDEFINE__
#define __KDEFINE__

// A. Chung chung //

typedef unsigned char  	 	uint8b;
typedef unsigned short 	 	uint16b;
typedef unsigned int   	 	uint32b;
typedef unsigned long  	 	uint64b;
typedef char		 	kstatus_t;
typedef unsigned long  	 	size_t;
typedef long		 	ssize_t;
typedef __builtin_va_list 	va_list;

// B. <kmanager.h> //

#define NULL 					0
#define VGA_HEIGHT 				24
#define VGA_WEIGHT 				80
#define KSTATUS_ERR 				((char)(-1))
#define KSTATUS_OK  				((char)(0))
#define KERNEL_ADDR 				0x100000
#define PAGE_SIZE 				4096
#define PAGE_PRESENT				(1ULL << 0)
#define PAGE_WRITE				(1ULL << 1)
#define PAGE_USER				(1ULL << 2)
#define PAGE_MASK				0x000FFFFFFFFFF000ULL
#define RADIX_CBUTTON				16
#define RADIX_MASK				0xF
#define PROCESS_RUN				(0)
#define PROCESS_READY				(1)
#define PROCESS_LOCK				(2)
#define PROCESS_RECV_BLOCKED			(3)
#define PROCESS_SEND_BLOCKED			(4)
#define PROCESS_MAXCO				(16) /* ko thuc process::status */
#define va_start(v,l)  			 	__builtin_va_start(v,l)
#define va_arg(v,l)     			__builtin_va_arg(v,l)
#define va_end(v)       			__builtin_va_end(v)
#define SYSCALL_EXIT				(0)
#define SYSCALL_PRINT				(1)
#define SYSCALL_HPRINT				(2)
#define SYSCALL_SEND				(3)
#define SYSCALL_RECV				(4)
#define SYSCALL_READIO				(5)
#define SYSCALL_WRITEIO				(6)
#define SYSCALL_MALLOC_PG			(7)
#define SYSCALL_FREE_PG				(8)
#define MSR_EFER				0xC0000080
#define MSR_STAR				0xC0000081
#define MSR_LSTAR				0xC0000082
#define MSR_SFMASK				0xC0000084
#define LAPIC_BASE				0xFEE00000ULL
#define LAPIC_ID				0x0020
#define LAPIC_VER				0x0030
#define LAPIC_TPR				0x0080
#define LAPIC_EOI				0x00B0
#define LAPIC_SVR				0x00F0
#define LAPIC_ESR				0x0280
#define LAPIC_LVT_TIMER				0x0320
#define LAPIC_TIMER_IC				0x0380
#define LAPIC_TIMER_CC				0x0390
#define LAPIC_TIMER_DC				0x03E0
#define DRIVER_MAXC				64
#define IPC_QUEUE_MAX				(24)

// C. <filesystem.h>

#define FSAHCI_DEV_SATA				0x00000101 // SATA binh thuong
#define FSAHCI_DEV_SATAPI			0xEB140101 // CD/DVD
#define FSAHCI_DEV_SEMB				0xC33C0101 // Quan ly
#define FSAHCI_DEV_PM				0x96690101 // "Cong da luong"

#endif
