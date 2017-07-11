#include "InterruptHandler.h"
#include "PIC.h"
#include "Keyboard.h"
#include "Console.h"
#include "Utility.h"
#include "Task.h"
#include "Descriptor.h"
#include "AssemblyUtility.h"
#include "HardDisk.h"

void kCommonExceptionHandler(int iVectorNumber, QWORD qwErrorCode) {
	char vcBuffer[3] = {0,};
	int i;
	vcBuffer[0] = '0' + iVectorNumber / 10;
	vcBuffer[1] = '0' + iVectorNumber % 10;
	for(i=0; i<10; i++)
		kPrintStringXY(0,i,"==================================================");
	kPrintStringXY(0,10,"Exception Occur, Vector is                         ");
	kPrintStringXY(28,10,vcBuffer);
	for(i=11; i<20; i++)
		kPrintStringXY(0,i,"==================================================");

	//kSleep(2000);
	//kReboot();
	while(1);
}

void kCommonInterruptHandler(int iVectorNumber) {
	char vcBuffer[] = "[INT:   , ]";
	static int g_iCommonInterruptCount=0;

	vcBuffer[5] = '0' + iVectorNumber / 10;
	vcBuffer[6] = '0' + iVectorNumber % 10;
	vcBuffer[8] = '0' + g_iCommonInterruptCount;
	g_iCommonInterruptCount = (g_iCommonInterruptCount + 1) %10;
	kPrintStringXY(70,0,vcBuffer);

	//처리 루틴 수행 후 EOI 전송
	kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}

void kKeyboardHandler(int iVectorNumber) {
	BYTE bTemp;

	if(kIsOutputBufferFull()==TRUE) {
		bTemp = kGetKeyboardScanCode();

		kConvertScanCodeAndPutQueue(bTemp);
	}


	kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);

}

void kTimerHandler(int iVectorNumber) {
	char vcBuffer[] = "[INT:   , ]";
	static int g_iCommonInterruptCount=0;

	vcBuffer[5] = '0' + iVectorNumber / 10;
	vcBuffer[6] = '0' + iVectorNumber % 10;
	vcBuffer[8] = '0' + g_iCommonInterruptCount;
	g_iCommonInterruptCount = (g_iCommonInterruptCount + 1) %10;
	kPrintStringXY(70,0,vcBuffer);

	kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);

	g_qwTickCount++;
	kDecreaseProcessorTime();
	if(kIsProcessorTimeExpired()==TRUE)
		kScheduleInInterrupt();

	//만약 타이머 인터럽트가 비활성화될 경우, kTestTask가 정상적으로 출력되지 않을 것이다
	//이유는 kTestTask 내부에서는 kSchedule()함수가 있어서 태스크가 스위칭이 되지만,
	//정작 태스크를 생성한 후 태스크를 스위칭 해주는 코드가 없으므로 kTestTask는 계속해서 레디 리스트에만 존재하고
	//셀만 수행이 된다. 만약 인터럽트 활성화 없이 정상적으로 수행하는것을 보고 싶다면 아래와 같이 하면 된다
	//while(1) kSchedule();
	//이렇게하면 무한적으로 태스크 스위칭을 해주므로 원하는대로 동작한다.
}

void kDeviceNotAvailableHandler(int iVectorNumber) {
	TCB* pstFPUTask, * pstCurrentTask;
	QWORD qwLastFPUTaskID;

	char vcBuffer[] = "[EXC: , ]";
	static int g_iFPUInterruptCount=0;


	vcBuffer[5] = '0' + iVectorNumber / 10;
	vcBuffer[6] = '0' + iVectorNumber % 10;
	vcBuffer[8] = '0' + g_iFPUInterruptCount;
	g_iFPUInterruptCount = (g_iFPUInterruptCount + 1) %10;
	kPrintStringXY(70,0,vcBuffer);

	//TS를 0으로 세팅하여 예외가 발생하지 않게 함
	kClearTS();

	qwLastFPUTaskID = kGetLastFPUUsedTaskID();
	pstCurrentTask = kGetRunningTask();

	//FPU를 가장  최근에 사용한 태스크가 본인이면 아무것도 안함
	if(qwLastFPUTaskID == pstCurrentTask->stLink.qwID)
		return;

	//FPU를 사용한 태스크가 있으면 FPU 상태를 저장
	//다른 태스크가 FPU를 사용할 때만 FPU컨텍스트 스위칭이 발생
	else if(qwLastFPUTaskID != TASK_INVALIDID) {
		pstFPUTask = kGetTCBInTCBPool(GETTCBOFFSET(qwLastFPUTaskID));
		if((pstFPUTask!=NULL)&&(pstFPUTask->stLink.qwID == qwLastFPUTaskID))
			kSaveFPUContext(pstFPUTask->vqwFPUContext);
	}

	//해당 태스크가 FPU를 처음 사용하는것이라면, 초기화 작업을 거침
	if(pstCurrentTask->bFPUUsed==FALSE) {
		kInitializeFPU();
		pstCurrentTask->bFPUUsed = TRUE;
	}
	else
		kLoadFPUContext(pstCurrentTask->vqwFPUContext);
	kSetLastFPUUsedTaskID(pstCurrentTask->stLink.qwID);
}

kHDDHandler(int iVectorNumber) {
	char vcBuffer[] = "[INT: , ]";
	static int g_iHDDInterruptCount = 0;


	vcBuffer[5] = '0' + iVectorNumber / 10;
	vcBuffer[6] = '0' + iVectorNumber % 10;
	vcBuffer[8] = '0' + (g_iHDDInterruptCount);
	g_iHDDInterruptCount = (g_iHDDInterruptCount+1)%10;
	kPrintStringXY(70,0,vcBuffer);

	if(iVectorNumber - PIC_IRQSTARTVECTOR==14) {
		//첫번째 pata포트의 인터럽트 발생 여부를 true로 설정
		kSetHDDInterruptFlag(TRUE,TRUE);
	}
	else {
		//두번째 pata포트의 인터럽트 발생 여부를 true로 설정
		kSetHDDInterruptFlag(FALSE,TRUE);
	}

	//eoi 전송
	kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}

