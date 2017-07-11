#include "Types.h"
#include "Page.h"
#include "ModeSwitch.h"

void kPrintString(int iX, int iY, const char *pcString);
BOOL kInitializeKernel64Area();
BOOL kIsMemoryEnough();
void kCopyKernel64ImageTo2MByte();

void Main(void) {
	DWORD i;
	DWORD dwEAX, dwEBX, dwECX, dwEDX;
	char vcVendorString[13] = {0,};

	kPrintString(0,3,"protected C Kernel Start");
	//최소 메모리 크기를 만족하는 지 검사
	kPrintString(0,4,"Memory Size Check....");
	if(kIsMemoryEnough()==FALSE) {
		kPrintString(35,4,"Fail 1");
		while (1);
	}
	else {
		kPrintString(25,4,"ok");
	}

	kPrintString(0,5,"IA-32e Kernel Area Init....");

	if (kInitializeKernel64Area()==FALSE) {
		kPrintString(35,5,"Fail 2");
		while(1);
	}
	kPrintString(33,5,"ok");

	kPrintString(0,6,"PT Init......");
	kInitializePageTables();
	kPrintString(33,6,"ok");

	//프로세스 제조사 정보 읽기
	kReadCPUID(0x00, &dwEAX, &dwEBX, &dwECX, &dwEDX);
	*((DWORD*)vcVendorString) = dwEBX;
	*((DWORD*)vcVendorString+1) = dwEDX;
	*((DWORD*)vcVendorString+2) = dwECX;
	kPrintString(0,7,"Vendor string... ");
	kPrintString(20,7,vcVendorString);

	//64bit 지원 유무 확인
	kReadCPUID(0x80000001,&dwEAX, &dwEBX, &dwECX, &dwEDX);
	kPrintString(0,8,"64bit Support Check....");
	if (dwEDX & (1<<29))
		kPrintString(25,8,"yes");
	else {
		kPrintString(25,8,"no");
		while (1);
	}

	//이후 IA-32e 모드 커널로 전환하는 함수  호출
	kPrintString(0,9, "Switch to IA-32e Mode...");
	kCopyKernel64ImageTo2MByte();
	kPrintString(33,9, "yes");
	kSwitchAndExecute64bitKernel();



	while (1);
}

void kPrintString(int iX, int iY, const char *pcString) {
	CHARACTER* pstScreen = (CHARACTER*)0xb8000;
	int i;
	pstScreen +=(iY*80) + iX;
	for (i=0; pcString[i]!=0; i++)
		pstScreen[i].bCharactor = pcString[i];
}

BOOL kInitializeKernel64Area() {
	DWORD *pdwCurrentAddress;
	pdwCurrentAddress = (DWORD*) 0x100000;
	while((DWORD)pdwCurrentAddress<0x600000) {
		*pdwCurrentAddress = 0x00;
		if (*pdwCurrentAddress!=0x00)
			return FALSE;
		pdwCurrentAddress++;
	}
	return TRUE;
}

BOOL kIsMemoryEnough() {
	DWORD *pdwCurrentAddress;
	pdwCurrentAddress = (DWORD*)0x100000;

	while((DWORD)pdwCurrentAddress < 0x400000) {
		*pdwCurrentAddress = 0x12345678;
		if (*pdwCurrentAddress!=0x12345678)
			return FALSE;
		pdwCurrentAddress+=(0x100000/4);
	}
	return TRUE;
}

void kCopyKernel64ImageTo2MByte() {
	WORD wKernel32SectorCount, wTotalKernelSectorCount;
	DWORD *pdwSourceAddress, *pdwDestinationAddress;
	int i;

	wTotalKernelSectorCount = *((WORD*)0x7c05);
	wKernel32SectorCount = *((WORD*)0x7c07);

	pdwSourceAddress = (DWORD*)(0x10000 + (wKernel32SectorCount*512));
	pdwDestinationAddress = (DWORD*)0x200000;

	for (i=0; i<512 * (wTotalKernelSectorCount - wKernel32SectorCount)/4; i++) {
		*pdwDestinationAddress = *pdwSourceAddress;
		pdwDestinationAddress++;
		pdwSourceAddress++;
	}
}
