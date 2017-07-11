#include "Page.h"


void kSetPageEntryData(PTENTRY *pstEntry, DWORD dwUpperBaseAddress,
		DWORD dwLowerBaseAddress, DWORD dwLowerFlags, DWORD dwUpperFlags) {
	//속성 설정
	pstEntry->dwAtrributeAndLowerBaseAddress = dwLowerBaseAddress | dwLowerFlags;
	pstEntry->dwUpperBaseAddressAndEXB = (dwUpperBaseAddress & 0xff) | dwUpperFlags;
}


void kInitializePageTables() {
	PML4TENTRY *pstPML4TEntry;
	PDPTENTRY *pstPDPTEntry;
	PDENTRY *pstPDEntry;
	DWORD dwMappingAddress;
	int i;

	//PML$ 테이블 생성, 첫 엔트리를 제외한 나머지 부분은 모두 0으로 초기화
	pstPML4TEntry = (PML4TENTRY*)0x100000;

	//페이지 디렉토리 테이블 포인터의 base address를 0x101000으로 설정
	kSetPageEntryData(&(pstPML4TEntry[0]), 0x00, 0x101000, PAGE_FLAGS_DEFAULT, 0);
	//나머지 엔트리는 모두 0으로 설정

	for(i=1; i<PAGE_MAXENTRYCOUNT; i++)
		kSetPageEntryData(&(pstPML4TEntry[i]),0,0,0,0);
	//페이지 디렉터리 포인터 테이블 생성

	pstPDPTEntry = (PDPTENTRY*)0x101000;


	for(i=0; i<64; i++) {
		kSetPageEntryData(&(pstPDPTEntry[i]),0,0x102000 + (i*PAGE_TABLESIZE),PAGE_FLAGS_DEFAULT,0);
	}

	for(i=64; i<PAGE_MAXENTRYCOUNT; i++) {
		kSetPageEntryData(&(pstPDPTEntry[i]),0,0,0,0);
	}

	//페이지 디렉터리 테이블 생성
	//하나의 페이지 디렉터리가 1GB까지 매핑 가능하므로(2의 10승) 이므로 64개의 엔트리가 필요
	pstPDEntry = (PDENTRY*)0x102000;
	dwMappingAddress = 0;
	for(i=0; i<PAGE_MAXENTRYCOUNT * 64; i++) {
		//하위 어드레스에서 32bit값을 넘으면 안되므로 20bit shift하고 i 곱하고 12bit도 shift해서 그 값을 상위 어드레스로 함
		kSetPageEntryData(&(pstPDEntry[i]),(i*(PAGE_DEFAULTSIZE >>20 ))>>12,dwMappingAddress,
				PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS,0);
		dwMappingAddress +=PAGE_DEFAULTSIZE;
	}

}

