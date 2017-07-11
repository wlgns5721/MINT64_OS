#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "AssemblyUtility.h"
#include "PIC.h"
#include "Console.h"
#include "ConsoleShell.h"
#include "Task.h"
#include "PIT.h"
#include "DynamicMemory.h"
#include "HardDisk.h"
#include "FileSystem.h"

void kPrintString(int iX, int iY, const char* pcString);

void Main() {
	char vcTemp[2] = {0,};
	BYTE bFlags;
	BYTE bTemp;
	int i=0;
	int iCursorX, iCursorY;
	KEYDATA stData;
	kInitializeConsole(0,10);
	kGetCursor(&iCursorX, &iCursorY);
	kPrintf("Switch To IA-32e Mode Success\n");
	kPrintf("IA-32e C Language Kernel Start\n");
	kPrintf("GDT init....\n");
	kInitializeGDTTableAndTSS();
	kLoadGDTR(GDTR_STARTADDRESS);

	kPrintf("TSS Segment Load.....\n");
	kLoadTR(GDT_TSSSEGMENT);

	kPrintf("IDT init.....");
	kInitializeIDTTables();
	kLoadIDTR(IDTR_STARTADDRESS);
	kPrintf("pass\n");
	kPrintf("Keyboard Activating and Init.....");
	if(kInitializeKeyboard()==TRUE) {
		kPrintf("pass\n");
		kChangeKeyboardLED(FALSE,FALSE,FALSE);
	}
	else {
		kPrintString(33,12,"fail\n");
		while(1);
	}


	kPrintf("PIC Controller and interrupt init......");
	kInitializePIC();
	kMaskPICInterrupt(0);
	kEnableInterrupt();
	kPrintf("pass\n");
	kPrintf("Check to Total RAM Size....");
	kCheckTotalRAMSize();
	kPrintf("%dMB\n\n",kGetTotalRAMSize());

	//동적 메모리 초기화
	kPrintf("Dynamic Memory Init.....\n");
	iCursorY++;
	kInitializeDynamicMemory();

	kPrintf("TCB Pool And Scheduler Initialize.........\n");
	iCursorY++;
	kInitializeScheduler();
	kInitializePIT(MSTOCOUNT(1),1);

	//하드 디스크 초기화
	kPrintf("HDD Initialize............");
	if(kInitializeHDD()==TRUE)
		kPrintf("Success\n");
	else
		kPrintf("Fail\n");

	//파일 시스템 추가
	kPrintf("File System Initialize........");
	if(kInitializeFileSystem()==TRUE)
		kPrintf("ok\n");
	else
		kPrintf("fail\n");

	kCreateTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD |
			TASK_FLAGS_SYSTEM | TASK_FLAGS_IDLE,0,0,(QWORD)kIdleTask);
	kStartConsoleShell();
}


void kPrintString(int iX, int iY, const char *pcString) {
	CHARACTER* pstScreen = (CHARACTER*)0xb8000;
	int i;
	pstScreen +=(iY*80) + iX;
	for (i=0; pcString[i]!=0; i++)
		pstScreen[i].bCharactor = pcString[i];
}

