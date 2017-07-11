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

	//ó�� ��ƾ ���� �� EOI ����
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

	//���� Ÿ�̸� ���ͷ�Ʈ�� ��Ȱ��ȭ�� ���, kTestTask�� ���������� ��µ��� ���� ���̴�
	//������ kTestTask ���ο����� kSchedule()�Լ��� �־ �½�ũ�� ����Ī�� ������,
	//���� �½�ũ�� ������ �� �½�ũ�� ����Ī ���ִ� �ڵ尡 �����Ƿ� kTestTask�� ����ؼ� ���� ����Ʈ���� �����ϰ�
	//���� ������ �ȴ�. ���� ���ͷ�Ʈ Ȱ��ȭ ���� ���������� �����ϴ°��� ���� �ʹٸ� �Ʒ��� ���� �ϸ� �ȴ�
	//while(1) kSchedule();
	//�̷����ϸ� ���������� �½�ũ ����Ī�� ���ֹǷ� ���ϴ´�� �����Ѵ�.
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

	//TS�� 0���� �����Ͽ� ���ܰ� �߻����� �ʰ� ��
	kClearTS();

	qwLastFPUTaskID = kGetLastFPUUsedTaskID();
	pstCurrentTask = kGetRunningTask();

	//FPU�� ����  �ֱٿ� ����� �½�ũ�� �����̸� �ƹ��͵� ����
	if(qwLastFPUTaskID == pstCurrentTask->stLink.qwID)
		return;

	//FPU�� ����� �½�ũ�� ������ FPU ���¸� ����
	//�ٸ� �½�ũ�� FPU�� ����� ���� FPU���ؽ�Ʈ ����Ī�� �߻�
	else if(qwLastFPUTaskID != TASK_INVALIDID) {
		pstFPUTask = kGetTCBInTCBPool(GETTCBOFFSET(qwLastFPUTaskID));
		if((pstFPUTask!=NULL)&&(pstFPUTask->stLink.qwID == qwLastFPUTaskID))
			kSaveFPUContext(pstFPUTask->vqwFPUContext);
	}

	//�ش� �½�ũ�� FPU�� ó�� ����ϴ°��̶��, �ʱ�ȭ �۾��� ��ħ
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
		//ù��° pata��Ʈ�� ���ͷ�Ʈ �߻� ���θ� true�� ����
		kSetHDDInterruptFlag(TRUE,TRUE);
	}
	else {
		//�ι�° pata��Ʈ�� ���ͷ�Ʈ �߻� ���θ� true�� ����
		kSetHDDInterruptFlag(FALSE,TRUE);
	}

	//eoi ����
	kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}

