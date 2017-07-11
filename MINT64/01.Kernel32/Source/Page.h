#ifndef __PAGE_H__
#define __PAGE_H__

#include "Types.h"

#define PAGE_FLAGS_P	0x1 //Present
#define PAGE_FLAGS_RW	0x2 //Read/Write
#define PAGE_FLAGS_US	0x4 //Users/Supervisor
#define PAGE_FLAGS_PWT	0x8 //Page Level Write-through
#define PAGE_FLAGS_PCD	0x10 //Page Level Cache Disable
#define PAGE_FLAGS_A	0x20 //Accessed
#define PAGE_FLAGS_D	0x40 //Dirty
#define PAGE_FLAGS_PS	0x80 //Page Size
#define PAGE_FLAGS_G	0x100 //Global
#define PAGE_FLAGS_PAT	0x1000 //Page Attribute Table Index
#define PAGE_FLAGS_EXB	0x80000000 //Execute Disable ∫Ò∆Æ

#define PAGE_FLAGS_DEFAULT	(PAGE_FLAGS_P | PAGE_FLAGS_RW)
#define PAGE_TABLESIZE	0X1000
#define PAGE_MAXENTRYCOUNT	512
#define PAGE_DEFAULTSIZE 0X200000

#pragma pack(push,1)

typedef struct kPageTableEntryStruct {
	DWORD dwAtrributeAndLowerBaseAddress;
	DWORD dwUpperBaseAddressAndEXB;
} PML4TENTRY, PDPTENTRY, PDENTRY, PTENTRY;
#pragma pack(pop)

void kSetPageEntryData(PTENTRY *pstEntry, DWORD dwUpperBaseAddress,
		DWORD dwLowerBaseAddress, DWORD dwLowerFlags, DWORD dwUpperFlags);
void kInitializePageTables();


#endif /*__PAGE_H__*/
