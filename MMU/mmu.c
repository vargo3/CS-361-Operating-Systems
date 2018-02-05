#include "mmu.h"
#include <stdio.h>

#define NULL_ADDRESS	(0ul)
#define PHYS_MASK	(0xffful)
#define ENTRY_MASK	(0x1fful)
#define GB_MASK		(0x3ffffffful)
#define MB_MASK		(0x1ffffful)
#define STRIP(a,b) ~(~((ADDRESS)a) | b)

void StartNewPageTable(CPU *cpu)
{
	//Create cr3 all the way!

	//Take memory 1/2 up:
	ADDRESS p = (ADDRESS)&cpu->memory[cpu->mem_size / 2];
	cpu->cr3 = p + 0x1000 - (p & 0xfff);
	//fprintf(stderr, "mem start: %lx  end: %lx\n", (unsigned long)cpu->memory, (unsigned long)&cpu->memory[cpu->mem_size]);
}

//Write:
// virt_to_phys
// map
// unmap
ADDRESS virt_to_phys(CPU *cpu, ADDRESS virt)
{
	//Convert a physical address *virt* to
	//a physical address.

	//If the MMU is off, simply return the virtual address as
	//the physical address
	if (!(cpu->cr0 & (1 << 31))) {
		return virt;
	}
	if (cpu->cr3 == 0) {
		return RET_PAGE_FAULT;
	}
	ADDRESS *pml4, *pdp, *pd, *pt, p;
	ADDRESS pml4e = (virt >> 39) & ENTRY_MASK;
	ADDRESS pdpe = (virt >> 30) & ENTRY_MASK;
	ADDRESS pde = (virt >> 21) & ENTRY_MASK;
	ADDRESS pte = (virt >> 12) & ENTRY_MASK;
	//1GB Page,
	pml4 = (ADDRESS*)cpu->cr3;
	if((ADDRESS)pml4 & 1){
		//fprintf(stderr, "pml4 present: %lx\n", (ADDRESS)pml4);
		if((ADDRESS)pml4 & (1 << 7)){  //2gb page
		}else{
			pdp = (ADDRESS*)((ADDRESS*)STRIP(pml4, PHYS_MASK))[pml4e];
			if((ADDRESS)pdp & 1){
				//fprintf(stderr, "pdp present: %lx\n", (ADDRESS)pdp);
				if((ADDRESS)pdp & (1 << 7)){	//1GB page
					p = ((ADDRESS*)STRIP(pdp, PHYS_MASK))[pdpe];
					if (p & 1){ //check if page is valid
						return STRIP(p, GB_MASK) | (virt & GB_MASK);
					}else{
						//fprintf(stderr, "page not present\n");
						return RET_PAGE_FAULT;
					}
				}else{
					pd = (ADDRESS*)((ADDRESS*)STRIP(pdp, PHYS_MASK))[pdpe];
					if((ADDRESS)pd & 1){
						//fprintf(stderr, "pd present: %lx\n", (ADDRESS)pd);
						if((ADDRESS)pd & (1 << 7)){	//2MB page
							p = ((ADDRESS*)STRIP(pd, PHYS_MASK))[pde];
							if (p & 1){
								return STRIP(p, MB_MASK) | (virt & MB_MASK);
							}else{
								//fprintf(stderr, "page not present\n");
								return RET_PAGE_FAULT;
							}
						}else{
							pt = (ADDRESS*)((ADDRESS*)STRIP(pd, PHYS_MASK))[pde];
							if((ADDRESS)pt & 1){	//4KB page
								//fprintf(stderr, "pt present: %lx\n", (ADDRESS)pt);
								p = ((ADDRESS*)STRIP(pt, PHYS_MASK))[pte];
								if (p & 1){
									return STRIP(p, PHYS_MASK) | (virt & PHYS_MASK);
								}else{
									//fprintf(stderr, "page not present\n");
									return RET_PAGE_FAULT;
								}
							}else{
								//fprintf(stderr, "pt not present\n");
								return RET_PAGE_FAULT;
							}
						}
					}else{
						//fprintf(stderr, "pd not present\n");
						return RET_PAGE_FAULT;
					}
				}
			}else{
				//fprintf(stderr, "pdp not present\n");
				return RET_PAGE_FAULT;
			}
		}
	}else{
		//fprintf(stderr, "pml4 not present\n");
		return RET_PAGE_FAULT;
	}
	return RET_PAGE_FAULT;
}

void map(CPU *cpu, ADDRESS phys, ADDRESS virt, PAGE_SIZE ps)
{
	//ALL PAGE MAPS must be located in cpu->memory!!

	//This function will return the PML4 of the mapped address

	//If the page size is 1G, you need a valid PML4 and PDP
	//If the page size is 2M, you need a valid PML4, PDP, and PD
	//If the page size is 4K, you need a valid PML4, PDP, PD, and PT
	//Remember that I could have some 2M pages and some 4K pages with a smattering
	//of 1G pages!

	ADDRESS *pml4, *pdp, *pd, *pt;
	ADDRESS pml4e = (virt >> 39) & ENTRY_MASK;
	ADDRESS pdpe = (virt >> 30) & ENTRY_MASK;
	ADDRESS pde = (virt >> 21) & ENTRY_MASK;
	ADDRESS pte = (virt >> 12) & ENTRY_MASK;

	if (cpu->cr3 == 0) {
		//Nothing has been created, so start here
		StartNewPageTable(cpu);
		pml4 = (ADDRESS*)cpu->cr3;
		*(ADDRESS*)cpu->cr3 = *(ADDRESS*)cpu->cr3 | (1 << 9);
		cpu->cr3 = cpu->cr3 | 1;
	}

	pml4 = (ADDRESS *)cpu->cr3;
	//fprintf(stderr, "pml4 at: %lx\n", (unsigned long)pml4);
	pdp = (ADDRESS *)((ADDRESS *)STRIP(pml4, PHYS_MASK))[pml4e];
	//fprintf(stderr, "pdp starts at: %lx\n", (unsigned long)pdp);
	if (!((ADDRESS)pdp & 1)){ //check if pdp is valid
		//fprintf(stderr, "find pdp\n");
		/* find new address space */
		//bit 9-11 in page entries is available to me. I'm using bit 9 to mark the table the entry is in as present or not.
		//I'm using bits 10-11 to mark the size of the table the entry is in. (GB=10, MB=01, KB=00). these are cleared if the table is freed
		//only the first entry in every table will be marked with the 9-11 bits
		pdp = (ADDRESS*)STRIP(pml4, PHYS_MASK); //start at the address following the pml4
		if (ps == PS_2M) pdp = (ADDRESS*)((ADDRESS)pdp + (1 << 21) - ((ADDRESS)pdp & MB_MASK));
		else if (ps == PS_1G) pdp = (ADDRESS*)((ADDRESS)pdp + (1 << 30) - ((ADDRESS)pdp & GB_MASK));
		while ((unsigned long)(pdp) < (unsigned long)&cpu->memory[cpu->mem_size] && (*pdp & (1 << 9))){ //found a used page
			//fprintf(stderr, "pdp find miss\n");
			if (ps == PS_1G || ((ADDRESS)pdp & (1 << 11))){ //found GB page
				pdp = (ADDRESS*)((ADDRESS)pdp + (1 << 30));
			}
			else if (ps == PS_2M || ((ADDRESS)pdp & (1 << 10))){ //found MB page
				pdp = (ADDRESS*)((ADDRESS)pdp + (1 << 21));
			}
			else{ //found KB page
				pdp = (ADDRESS*)((ADDRESS)pdp + (1 << 12));
			}
		}
		if ((ps == PS_1G && ((ADDRESS)pdp + (1 << 30)) > (unsigned long)&cpu->memory[cpu->mem_size]) ||
			(ps == PS_2M && ((ADDRESS)pdp + (1 << 21)) > (unsigned long)&cpu->memory[cpu->mem_size]) ||
			(ps == PS_4K && ((ADDRESS)pdp + (1 << 12)) > (unsigned long)&cpu->memory[cpu->mem_size])){
			//fprintf(stderr, "return pdp size\n");
			return; //memory is full
		}
		*pdp = NULL_ADDRESS | (1 << 9);
		pdp = (ADDRESS*)((ADDRESS)pdp | 1 | (1 << 9));//set present bit
		((ADDRESS *)STRIP(pml4, PHYS_MASK))[pml4e] = (ADDRESS)pdp;
	}
	//fprintf(stderr, "pdp done: %lx\n", (unsigned long)pdp);

	if (ps == PS_2M || ps == PS_4K){
		pd = (ADDRESS *)((ADDRESS*)STRIP(pdp, PHYS_MASK))[pdpe];
		//fprintf(stderr, "pd starts at: %lx\n", (unsigned long)pd);
		if (!((unsigned long)pd & 1)){ //check if pdp is valid
			//find new address space
			pd = (ADDRESS*)((ADDRESS)STRIP(pml4, PHYS_MASK)); //start at the address following the pml4
			if (ps == PS_2M) pd = (ADDRESS*)((ADDRESS)pd + (1 << 21) - ((ADDRESS)pd & MB_MASK));
			else if (ps == PS_1G) pd = (ADDRESS*)((ADDRESS)pd + (1 << 30) - ((ADDRESS)pd & GB_MASK));
			//if (((unsigned long)((ADDRESS)pdp + 0x2000) < (unsigned long)&cpu->memory[cpu->mem_size])) //fprintf(stderr, "test\n");
			while ((unsigned long)(pd) < (unsigned long)&cpu->memory[cpu->mem_size] && (*pd & (1 << 9))){ //found a used page
				//fprintf(stderr, "pd find miss\n");
				if (ps == PS_1G || ((ADDRESS)pd & (1 << 11))){ //found available GB page
					pd = (ADDRESS*)((ADDRESS)pd + (1 << 30));
				}
				else if (ps == PS_2M || ((ADDRESS)pdp & (1 << 10))){ //found available MB page
					pd = (ADDRESS*)((ADDRESS)pd + (1 << 21));
				}
				else{ //found available KB page
					pd = (ADDRESS*)((ADDRESS)pd + (1 << 12));
				}
			}
			if (/*(ps == PS_1G && ((ADDRESS)pd + (1 << 30)) > (unsigned long)&cpu->memory[cpu->mem_size]) ||*/
				(ps == PS_2M && ((ADDRESS)pd + (1 << 21)) > (unsigned long)&cpu->memory[cpu->mem_size]) ||
				(ps == PS_4K && ((ADDRESS)pd + (1 << 12)) > (unsigned long)&cpu->memory[cpu->mem_size])){
				//fprintf(stderr, "return pd size\n");
				return; //memory is full
			}
			*pd = NULL_ADDRESS | (1 << 9);
			pd = (ADDRESS*)((ADDRESS)pd | 1 | (1 << 9)); //set present bit
			((ADDRESS *)STRIP(pdp, PHYS_MASK))[pdpe] = (ADDRESS)pd;
		}
		//fprintf(stderr, "pd done: %lx\n", (unsigned long)pd);

		if (ps == PS_4K){
			pt = (ADDRESS *)((ADDRESS *)STRIP(pd, PHYS_MASK))[pde];
			//fprintf(stderr, "pt starts at: %lx\n", (unsigned long)pt);
			if (!((unsigned long)pt & 1)){ //check if pdp is valid
				//find new address space
				pt = (ADDRESS*)((ADDRESS)STRIP(pml4, PHYS_MASK)); //start at the address following the pml4
				if (ps == PS_2M) pt = (ADDRESS*)((ADDRESS)pt + (1 << 21) - ((ADDRESS)pt & MB_MASK));
				else if (ps == PS_1G) pt = (ADDRESS*)((ADDRESS)pt + (1 << 30) - ((ADDRESS)pt & GB_MASK));
				//if (((unsigned long)((ADDRESS)pdp + 0x2000) < (unsigned long)&cpu->memory[cpu->mem_size])) //fprintf(stderr, "test\n");
				while ((unsigned long)(pt) < (unsigned long)&cpu->memory[cpu->mem_size] && (*pt & (1 << 9))){ //found a used page
					//fprintf(stderr, "pt find miss\n");
					if (ps == PS_1G || ((ADDRESS)pt & (1 << 11))){ //found GB page
						pt = (ADDRESS*)((ADDRESS)pt + (1 << 30));
					}
					else if (ps == PS_2M || ((ADDRESS)pdp & (1 << 10))){ //found MB page
						pt = (ADDRESS*)((ADDRESS)pt + (1 << 21));
					}
					else{ //found KB page
						pt = (ADDRESS*)((ADDRESS)pt + (1 << 12));
					}
				}
				if (/*(ps == PS_1G && ((ADDRESS)pt + (1 << 30)) > (unsigned long)&cpu->memory[cpu->mem_size]) ||
					(ps == PS_2M && ((ADDRESS)pt + (1 << 21)) > (unsigned long)&cpu->memory[cpu->mem_size]) ||*/
					(ps == PS_4K && ((ADDRESS)pt + (1 << 12)) > (unsigned long)&cpu->memory[cpu->mem_size])){
					//fprintf(stderr, "return pt size\n");
					return; //memory is full
				}
				*pt = NULL_ADDRESS | (1 << 9);
				pt = (ADDRESS*)((ADDRESS)pt | 1 | (1 << 9)); //set present bit
				((ADDRESS *)STRIP(pd, PHYS_MASK))[pde] = (ADDRESS)pt;
			}
			//fprintf(stderr, "pt done: %lx\n", (unsigned long)pt);
			((ADDRESS*)STRIP(pt,PHYS_MASK))[pte] = STRIP(phys, PHYS_MASK) | 1;
		}
		else{ //ps == PS_2M
			pd = (ADDRESS*)((unsigned long)pd | (1 << 7) | (1 << 10));
			((ADDRESS *)STRIP(pdp, PHYS_MASK))[pdpe] = (ADDRESS)pd;
			((ADDRESS*)STRIP(pd,MB_MASK))[pde] = STRIP(phys, MB_MASK) | 1;
		}
	}
	else{ //ps == PS_GB
		pdp = (ADDRESS*)((unsigned long)pdp | (1 << 7) | (1 << 11));
		((ADDRESS *)STRIP(pml4, PHYS_MASK))[pml4e] = (ADDRESS)pdp;
		((ADDRESS*)STRIP(pdp,GB_MASK))[pdpe] = STRIP(phys, GB_MASK) | 1;
	}
}

void unmap(CPU *cpu, ADDRESS virt, PAGE_SIZE ps)
{
	//Simply set the present bit (P) to 0 of the virtual address page
	//If the page size is 1G, set the present bit of the PDP to 0
	//If the page size is 2M, set the present bit of the PD  to 0
	//If the page size is 4K, set the present bit of the PT  to 0

	
	if (cpu->cr3 == 0)
		return;
	
	//ADDRESS phys;
	ADDRESS *pml4, *pdp, *pd, *pt;
	ADDRESS pml4e = (virt >> 39) & ENTRY_MASK;
	ADDRESS pdpe = (virt >> 30) & ENTRY_MASK;
	ADDRESS pde = (virt >> 21) & ENTRY_MASK;
	//ADDRESS pte = (virt >> 12) & ENTRY_MASK;
	//1GB Page,
	pml4 = (ADDRESS*)cpu->cr3;
	if((ADDRESS)pml4 & 1){
		if((ADDRESS)pml4 & (1 << 7)){  //2gb page
		}else{
			pdp = (ADDRESS*)((ADDRESS*)STRIP(pml4, PHYS_MASK))[pml4e];
			if((ADDRESS)pdp & 1){
				if(((ADDRESS)pdp & (1 << 7)) || (ps == PS_1G)){	// 1gb
					((ADDRESS*)STRIP(pml4, PHYS_MASK))[pml4e] = (STRIP(pdp,1));
					*((ADDRESS*)STRIP(pdp, PHYS_MASK)) = NULL_ADDRESS;
					return;
				}else{
					pd = (ADDRESS*)((ADDRESS*)STRIP(pdp, PHYS_MASK))[pdpe];
					if((ADDRESS)pd & 1){
						if(((ADDRESS)pd & (1 << 7)) || (ps == PS_2M)){	//2MB
							((ADDRESS*)STRIP(pdp, PHYS_MASK))[pdpe] = (STRIP(pdp, 1));
							*((ADDRESS*)STRIP(pd, PHYS_MASK)) = NULL_ADDRESS;
							return;
						}else{
							pt = (ADDRESS*)((ADDRESS*)STRIP(pd, PHYS_MASK))[pde];
							if(((ADDRESS)pt & 1) || (ps == PS_4K)){	//4KB
								////fprintf(stderr, "pt present: %lx\n", (ADDRESS)pt);
								((ADDRESS*)STRIP(pd, PHYS_MASK))[pde] = (STRIP(pt, 1));
								*((ADDRESS*)STRIP(pt, PHYS_MASK)) = NULL_ADDRESS;
								return;
							}
						}
					}
				}
			}
		}
	}
	return;
}



//////////////////////////////////////////
//
//Do not touch the functions below!
//
//////////////////////////////////////////
void cpu_pfault(CPU *cpu)
{
	fprintf(stderr, "MMU: #PF @ 0x%016lx\n", cpu->cr2);
}


CPU *new_cpu(unsigned int mem_size)
{
	CPU *ret;


	ret = (CPU*)calloc(1, sizeof(CPU));
	ret->memory = (char*)calloc(mem_size, sizeof(char));
	ret->mem_size = mem_size;

#if defined(I_DID_EXTRA_CREDIT)
	ret->tlb = (TLB_ENTRY*)calloc(TLB_SIZE, sizeof(TLB_ENTRY));
#endif

	return ret;
}

void destroy_cpu(CPU *cpu)
{
	if (cpu->memory) {
		free(cpu->memory);
	}
#if defined(I_DID_EXTRA_CREDIT)
	if (cpu->tlb) {
		free(cpu->tlb);
	}
	cpu->tlb = 0;
#endif

	cpu->memory = 0;

	free(cpu);
}


int mem_get(CPU *cpu, ADDRESS virt)
{
	ADDRESS phys = virt_to_phys(cpu, virt);
	//fprintf(stderr, "get phys: %lx\n", phys);
	if (phys == RET_PAGE_FAULT || phys + 4 >= cpu->mem_size) {
		cpu->cr2 = virt;
		cpu_pfault(cpu);
		return -1;
	}
	else {
		return *((int*)(&cpu->memory[phys]));
	}
}

void mem_set(CPU *cpu, ADDRESS virt, int value)
{
	ADDRESS phys = virt_to_phys(cpu, virt);
	//fprintf(stderr, "set phys: %lx\n", phys);
	if (phys == RET_PAGE_FAULT || phys + 4 >= cpu->mem_size) {
		cpu->cr2 = virt;
		cpu_pfault(cpu);
	}
	else {
		*((int*)(&cpu->memory[phys])) = value;
	}
}

#if defined(I_DID_EXTRA_CREDIT)

void print_tlb(CPU *cpu)
{
	int i;
	TLB_ENTRY *entry;

	printf("#   %-18s %-18s Tag\n", "Virtual", "Physical");
	for (i = 0;i < TLB_SIZE;i++) {
		entry = &cpu->tlb[i];
		printf("%-3d 0x%016lx 0x%016lx %08x\n", i+1, entry->virt, entry->phys, entry->tag);
	}
}

#endif
