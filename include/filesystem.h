#ifndef __FILESYSTEM__
#define __FILESYSTEM__

#include "kdefine.h"
#include "kmanager.h"

// A. AHCI/SATA

struct hba_port 
{
	// "cuu toi voi!":)
	uint32b clb,
		clbu,
		fb,
		fbu,
		is,
		ie,
		cmd,
		reserved0,
		tfd,
		sig,
		ssts,
		sctl,
		serr,
		sact,
		ci,
		sntf,
		fbs,
		reserved1[11],
		vendor[4];
} __attribute__((packed));
struct hba_mem
{
	uint32b cap,
		ghc,
		is,
		pi,
		vs,
		ccc_ctl,
		ccc_pts,
		em_loc,
		em_ctl,
		cap2,
		bohc;
	uint8b reserved		[116],
	       vendor		[96];
	struct hba_port ports	[32];
} __attribute__((packed));
struct ahci_prdt_element
{
	uint32b dba;
	uint32b dbau;
	uint32b reserved;
	uint32b dbc;
} __attribute__((packed));
struct ahci_cmd_header
{
	uint8b 	cfl		: 5;
	uint8b 	a		: 1,
	       	w		: 1,
		p		: 1,
		r		: 1,
		b		: 1,
		c		: 1,
		reserved0	: 1;
	uint8b 	pmp		: 4;
	uint16b prdtl;
	uint32b prdbc,
		ctba,
		ctbau,
		reserved[4];
} __attribute__((packed));
struct ahci_cmd_table
{
	uint8b cfis[64],
	       acmd[16],
	       reserved[48];
	struct ahci_prdt_element prdt_entry[1];
} __attribute__((packed));
struct mbr_partition
{
	uint8b bootok, // 0x80 hay NULL?
	       start_chs[3],
	       os_type,
	       end_chs  [3];
	uint32b start_lba,
		total_sector;
} __attribute__((packed));

void ahci_portinit(struct hba_port*, uint64b, uint64b, uint64b, uint64b);
void ahci_portstart(struct hba_port*);
void ahci_portstop(struct hba_port*);

uint64b fs_allocproc(uint64b vaddr)
{
	uint64b paddr = 0;
	asm volatile(
		"\tmov $7, %%rax\n"
		"\tmov %1, %%rsi\n"
		"\tint $0x80\n"
		"\tmov %%rax, %0\n"
		: "=r"(paddr)
		: "r"(vaddr)
		: "rax", "rsi", "memory"
	);
	return paddr;
}

// a. o dia

void ahci_fport(struct hba_mem* mem) 
{
	uint32b pi = mem->pi;
	for (int i = 0; i < 32; i++)
	{
		if (pi & (1 << i))
		{
			struct hba_port* port = &(mem->ports[i]);
			uint32b ssts = port->ssts;
			uint8b dets = ssts & 0x0F;
			if (dets == 3) 
			{
				if (port->sig == FSAHCI_DEV_SATA) 
				{
					uint64b vclb = 0x90080070 + (i * 0x2000);
					uint64b vfb  = vclb + 0x1000;
					uint64b vctba= vclb + 0x2000;
					uint64b pclb = fs_allocproc(vclb);
					uint64b pfb  = fs_allocproc(vfb);
					uint64b pctba= fs_allocproc(vctba);
					if (pclb && pfb && pctba) {
						ahci_portinit(port,
							pclb,pfb,
							pctba, vclb);
					}
				}
				else if (port->sig == FSAHCI_DEV_SATAPI) {}
			}
		}
	}
}
void ahci_portstop(struct hba_port* port)
{
	port->cmd &= ~0x0001; // ~1  = 0
	port->cmd &= ~0x0010; // ~16 = -17
	while (1) {
		if (port->cmd & 0x8000 || port->cmd & 0x4000) { continue; }
		break;
	}
}
void ahci_portstart(struct hba_port* port) {
	while (port->cmd & 0x8000) {} // (bit.CR & 0x8000 == 0) ? break : continue;
	port->cmd |= 0x0010; // 16
	port->cmd |= 0x0001; // 1
}
void ahci_portinit(struct hba_port* port, uint64b clb_phys, uint64b fb_phys,
		   uint64b ctba_phys, uint64b clbv)
{
	ahci_portstop(port);
	port->clb  = (uint32b)(clb_phys & 0xFFFFFFFF),
	port->clbu = (uint32b)(clb_phys >> 32),
	port->fb   = (uint32b)(fb_phys  & 0xFFFFFFFF),
	port->fbu  = (uint32b)(fb_phys  >> 32),
	port->is   = 0xFFFFFFFF;

	struct ahci_cmd_header* cmdhdr = (struct ahci_cmd_header*)clbv;
	cmdhdr[0].ctba  = (uint32b)(ctba_phys & 0xFFFFFFFF);
	cmdhdr[0].ctbau = (uint32b)(ctba_phys >> 32);
	cmdhdr[0].cfl   = 5;

	ahci_portstart(port);
}
// doc va ghi
kstatus_t ahci_read(struct hba_port* port, uint64b startlba, uint16b count, uint64b bufp,
		    uint64b clbva, uint64b ctbava)
{
	if (!port) { return KSTATUS_ERR; }
	port->is = 0xFFFFFFFF;
	int slot = 0; // "thich thi them"
	struct ahci_cmd_header* cmdhdr = (struct ahci_cmd_header*)clbva;

	cmdhdr[slot].cfl 	= 5,
	cmdhdr[slot].w	 	= 0,
	cmdhdr[slot].prdtl 	= 1; // 0
	
	struct ahci_cmd_table* cmdtbl = (struct ahci_cmd_table*)ctbava;

	cmdtbl->prdt_entry[0].dba  = (uint32b)(bufp  & 0xFFFFFFFF);
	cmdtbl->prdt_entry[0].dbau = (uint32b)(bufp >> 32);
	cmdtbl->prdt_entry[0].dbc  = (uint32b)(count * 512) - 1;

	uint8b* goitin = cmdtbl->cfis;
	int so_goitin = 0;
	// a. Goi tin MS. 0x27
	goitin[so_goitin++] = 0x27;
	goitin[so_goitin++] = 0x80;
	goitin[so_goitin++] = 0x25;
	// aw. Dia chi LBA -> Sector can doc
	so_goitin++;
	goitin[so_goitin++] = (uint8b)(startlba & 0xFF);
	goitin[so_goitin++] = (uint8b)((startlba >> 8 ) & 0xFF);
	goitin[so_goitin++] = (uint8b)((startlba >> 16) & 0xFF);
	goitin[so_goitin++] = (uint8b)(0x40);
	goitin[so_goitin++] = (uint8b)((startlba >> 24) & 0xFF);
	goitin[so_goitin++] = (uint8b)((startlba >> 32) & 0xFF);
	goitin[so_goitin++] = (uint8b)((startlba >> 40) & 0xFF);
	// aa. Nap sector can read
	so_goitin++;
	goitin[so_goitin++] = (uint8b)(count & 0xFF);
	goitin[so_goitin++] = (uint8b)((count >> 8) & 0xFF);
	// b. ban da sang sang chua?
	size_t timedko = 0;
	while ((port->tfd & (0x80 | 0x08)) && timedko < 1000000) { timedko++; }
	if (timedko >= 1000000) { return KSTATUS_ERR; }
	port->ci = (1 << slot);
	while (1) {
		if ((port->ci & (1 << slot)) == 0) { break; }
		else if (port->is & (1 << 30)) { return KSTATUS_ERR; }
	}
	return KSTATUS_OK;
}

// aw. phan vung

kstatus_t partition_pmbr(
		struct hba_port* port, uint64b clbva, uint64b ctbava,
		struct mbr_partition* partret
)
{
	if (!port) { return KSTATUS_ERR; }
	void* sectbuf = pmm_bitalloc_pg();
	if (!sectbuf) { return KSTATUS_ERR; }
	kstatus_t readstatus = ahci_read(
		port, 0, 1, 
		(uint64b)sectbuf, clbva, ctbava
	);
	if (readstatus == KSTATUS_ERR) {
		pmm_bitfree_pg(sectbuf);
		return KSTATUS_ERR;
	}
	// thu thach ngan dong
	uint8b* mbr_partb = (uint8b*)sectbuf;
	struct mbr_partition* partition = (struct mbr_partition*)(&mbr_partb[446]);
	for (int i = 0; i < 4; i++) { partret[i] = partition[i]; }
	return pmm_bitfree_pg(sectbuf);
} // parse
#endif
