#include "Task.h"
#include "Descriptor.h"
#include "Utility.h"
#include "List.h"
#include "AssemblyUtility.h"

static SCHEDULER gs_stScheduler;
static TCBPOOLMANAGER gs_stTCBPoolManager;


static void kSetUpTask(TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress,
		void* pvStackAddress, QWORD qwStackSize) {
	kMemSet(pstTCB->stContext.vqRegister,0,sizeof(pstTCB->stContext.vqRegister));

	//RSP, RBP�� ����. ���� �ֻ����� ����Ű�� ���� �ƴ϶� �ֻ���-8�� ����Ű�� ������
	//���� �ֻ��� address�� kExitTask�� address�� ä�� �����̱� �����̴�.
	pstTCB->stContext.vqRegister[TASK_RSPOFFSET] = (QWORD)pvStackAddress + qwStackSize - 8;
	pstTCB->stContext.vqRegister[TASK_RBPOFFSET] = (QWORD)pvStackAddress + qwStackSize - 8;
	//���� �ֻ����� kExitTask �Լ� address ����
	*(QWORD*)((QWORD)pvStackAddress + qwStackSize - 8) = (QWORD)kExitTask;

	pstTCB->stContext.vqRegister[TASK_CSOFFSET] = GDT_KERNELCODESEGMENT;
	pstTCB->stContext.vqRegister[TASK_DSOFFSET] = GDT_KERNELDATASEGMENT;
	pstTCB->stContext.vqRegister[TASK_ESOFFSET] = GDT_KERNELDATASEGMENT;
	pstTCB->stContext.vqRegister[TASK_FSOFFSET] = GDT_KERNELDATASEGMENT;
	pstTCB->stContext.vqRegister[TASK_GSOFFSET] = GDT_KERNELDATASEGMENT;
	pstTCB->stContext.vqRegister[TASK_SSOFFSET] = GDT_KERNELDATASEGMENT;

	pstTCB->stContext.vqRegister[TASK_RIPOFFSET] = qwEntryPointAddress;
	pstTCB->stContext.vqRegister[TASK_RFLAGSOFFSET] |= 0x200;
	//pstTCB->qwID = qwID;
	pstTCB->qwStackSize = qwStackSize;
	pstTCB->pvStackAddress = pvStackAddress;
	pstTCB->qwFlags = qwFlags;
}

static void kInitializeTCBPool() {
	int i;
	kMemSet(&(gs_stTCBPoolManager),0,sizeof(gs_stTCBPoolManager));

	//�׽�ũ Ǯ�� ��巹�� ���� �� �ʱ�ȭ
	gs_stTCBPoolManager.pstStartAddress = (TCB*)TASK_TCBPOOLADDRESS;
	kMemSet(TASK_TCBPOOLADDRESS,0,sizeof(TCB)*TASK_MAXCOUNT);

	//TCB�� ID �Ҵ��Ѵ�. TCB�� �ʵ忡 �ִ� stLink�� LISTLINK ����ü
	//���� ID�� �Ҵ��� ��, ������ �ش� TCB�� �Ҵ��Ϸ��� qwID�� ���� 32��Ʈ ���� 0�� �ƴѰ��� ���� �Ѵ�.
	for(i=0; i<TASK_MAXCOUNT; i++) {
		gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;
	}
	gs_stTCBPoolManager.iMaxCount = TASK_MAXCOUNT;
	gs_stTCBPoolManager.iAllocatedCount = 1;
}

static TCB* kAllocateTCB() {
	TCB* pstEmptyTCB;
	int i;
	if(gs_stTCBPoolManager.iUseCount==gs_stTCBPoolManager.iMaxCount)
		return NULL;
	for(i=0; i<gs_stTCBPoolManager.iMaxCount; i++) {
		//����ִ� TCB�� ã�´�
		//ID�� ���� 32��Ʈ�� 0�̸� �Ҵ���� ���� TCB
		if((gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID>>32)==0) {
			pstEmptyTCB = &(gs_stTCBPoolManager.pstStartAddress[i]);
			break;
		}
	}

	//���� 32��Ʈ�� 0�� �ƴ� ������ �����ؼ� �Ҵ�� TCB�� ����
	pstEmptyTCB->stLink.qwID = ((QWORD)gs_stTCBPoolManager.iAllocatedCount<<32) | i;
	gs_stTCBPoolManager.iUseCount++;
	gs_stTCBPoolManager.iAllocatedCount++;
	if(gs_stTCBPoolManager.iAllocatedCount==0) {
		gs_stTCBPoolManager.iAllocatedCount = 1;
	}
	return pstEmptyTCB;
}

static void kFreeTCB(QWORD qwID) {
	int i;
	i= GETTCBOFFSET(qwID);
	//TCB �ʱ�ȭ
	kMemSet(&(gs_stTCBPoolManager.pstStartAddress[i].stContext),0,sizeof(CONTEXT));
	//ID ����
	gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;
	gs_stTCBPoolManager.iUseCount--;
}

TCB* kCreateTask(QWORD qwFlags, void* pvMemoryAddress, QWORD qwMemorySize,
		QWORD qwEntryPointAddress) {
	TCB* pstTask, * pstProcess;
	void* pvStackAddress;
	BOOL bPreviousFlag;

	//�Ӱ迵�� ����
	bPreviousFlag = kLockForSystemData();

	//TCB pool���� �� ������ ã�Ƽ� �� �κ��� �Ҵ����ش�.
	pstTask = kAllocateTCB();
	if(pstTask==NULL) {
		//�Ӱ� ���� ��
		kUnlockForSystemData(bPreviousFlag);
		return NULL;
	}

	//���� �׽�ũ�� �θ� ���μ����� ����(���� ���� �½�ũ�� ���μ������ ���� ����)
	pstProcess = kGetProcessByThread(kGetRunningTask());
	if(pstProcess==NULL) {
		kFreeTCB(pstTask->stLink.qwID);
		kUnlockForSystemData(bPreviousFlag);
	}
	//�������� �θ� ���μ����� �ڽ� ������ ����Ʈ�� ������
	if(qwFlags & TASK_FLAGS_THREAD) {
		pstTask->qwParentProcessID = pstProcess->stLink.qwID;
		pstTask->pvMemoryAddress = pstProcess->pvMemoryAddress;
		pstTask->qwMemorySize = pstProcess->qwMemorySize;

		kAddListToTail(&(pstProcess->stChildThreadList),&(pstTask->stThreadLink));
	}

	//���μ����� ��� �Ķ���ͷ� ���� ����� ����
	else {
		pstTask->qwParentProcessID = pstProcess->stLink.qwID;
		pstTask->pvMemoryAddress = pvMemoryAddress;
		pstTask->qwMemorySize = qwMemorySize;
	}
	//thread�� id�� �½�ũ id�� �����ϰ� ����
	pstTask->stThreadLink.qwID = pstTask->stLink.qwID;
	kUnlockForSystemData(bPreviousFlag);

	//����� ������ ��ġ�� qwID�� ���� ���� �ε����ؼ� �Ҵ�
	pvStackAddress = (void*)(TASK_STACKPOOLADDRESS + (TASK_STACKSIZE * GETTCBOFFSET(pstTask->stLink.qwID)));


	//TCB�� ������ �� ���� ����Ʈ�� ����
	kSetUpTask(pstTask, qwFlags, qwEntryPointAddress, pvStackAddress, TASK_STACKSIZE);

	//�ڽ� ������ �ʱ�ȭ
	kInitializeList(&(pstTask->stChildThreadList));

	bPreviousFlag = kLockForSystemData();
	//�½�ũ�� ready list�� ����
	kAddTaskToReadyList(pstTask);
	kUnlockForSystemData(bPreviousFlag);

	return pstTask;
}

////////////////////////////////
//�����층 ���� �Լ�
////////////////////////////////
void kInitializeScheduler() {
	int i;
	TCB* pstTask;
	kInitializeTCBPool();
	//ready ����Ʈ �ʱ�ȭ
	for (i=0; i<TASK_MAXREADYLISTCOUNT; i++) {
		kInitializeList(&(gs_stScheduler.vstReadyList[i]));
		gs_stScheduler.viExecuteCount[i] = 0;
	}
	//wait ����Ʈ �ʱ�ȭ
	kInitializeList(&(gs_stScheduler.stWaitList));

	//TCB�� �Ҵ�޾� ������ ������ �½�ũ�� Ŀ�� ������ ���μ����� ����
	pstTask = kAllocateTCB();
	gs_stScheduler.pstRunningTask = pstTask;
	pstTask->qwFlags = TASK_FLAGS_HIGHEST | TASK_FLAGS_PROCESS | TASK_FLAGS_SYSTEM;
	pstTask->qwParentProcessID = pstTask->stLink.qwID;
	pstTask->pvMemoryAddress = (void*)0x100000;
	pstTask->qwMemorySize = 0x500000;
	pstTask->pvStackAddress = (void*)0x600000;
	pstTask->qwStackSize = 0x100000;

	//���μ��� ������ ����ϴµ� ����ϴ� �ڷᱸ�� �ʱ�ȭ
	gs_stScheduler.qwSpendProcessorTimeInIdleTask = 0;
	gs_stScheduler.qwProcessorLoad = 0;

	//FPU�� ����� �½�ũ ID�� ��ȿ���� ���� ������ ����
	gs_stScheduler.qwLastFPUUsedTaskID = TASK_INVALIDID;
}

void kSetRunningTask(TCB* pstTask) {
	BOOL bPreviousFlag;

	bPreviousFlag = kLockForSystemData();

	gs_stScheduler.pstRunningTask = pstTask;

	kUnlockForSystemData(bPreviousFlag);
}

TCB* kGetRunningTask() {
	BOOL bPreviousFlag;
	TCB* pstRunningTask;

	bPreviousFlag = kLockForSystemData();
	pstRunningTask = gs_stScheduler.pstRunningTask;
	kUnlockForSystemData(bPreviousFlag);
	return pstRunningTask;
}
//�������� ������ �½�ũ�� ����
static TCB* kGetNextTaskToRun() {
	TCB* pstTarget = NULL;
	int iTaskCount, i, j;

	//ť�� ���� �����ִ� �½�ũ�� ������ ������ �켱���� ť�� �½�ũ���� �ѹ��� ����Ǿ���, ������ ť���� �½�ũ�� �纸�� ���
	//�������� ����� �½�ũ�� ���õ��� ���ϴ� ��찡 ����� ������ �׷� ��츦 ���� �Ʒ� �ڵ带 �ι� �����Ŵ���ν� �½�ũ�� ���þȵǴ� ��츦 ����
	for(j=0; j<2; j++) {
		//���� �켱�������� ���� �켱�������� ����Ʈ�� Ȯ���Ͽ� �����ٸ��� �½�ũ ����
		for(i=0; i<TASK_MAXREADYLISTCOUNT; i++) {
			iTaskCount = kGetListCount(&(gs_stScheduler.vstReadyList[i]));
			//�ش� ����Ʈ�� �½�ũ ����, �ش� ����ũ�� ���� ���� ���Ѵ�
			//��, �ش� ����Ʈ�� ��� �½�ũ�� �ѹ��� ����Ǿ������� �˻�
			if(gs_stScheduler.viExecuteCount[i]<iTaskCount) {
				pstTarget = (TCB*)kRemoveListFromHeader(&(gs_stScheduler.vstReadyList[i]));
				gs_stScheduler.viExecuteCount[i]++;
				break;
			}
			else {
				gs_stScheduler.viExecuteCount[i] = 0;
			}
		}
		if(pstTarget!=NULL)
			break;
	}
	return pstTarget;
}

static BOOL kAddTaskToReadyList(TCB* pstTask) {
	//�켱������ ���� �ش� ť�� TCB ����
	//ex) �켱������ 2��� ReadyList[2]�� ����
	BYTE bPriority;
	bPriority = GETPRIORITY(pstTask->qwFlags);
	if(bPriority>=TASK_MAXREADYLISTCOUNT)
		return FALSE;
	kAddListToTail(&(gs_stScheduler.vstReadyList[bPriority]),pstTask);
}

//�ٸ� �½�ũ�� ��ȯ�ϴ� �Լ�
//���ͷ�Ʈ�� ���ܰ� �߻����� �� ȣ���ϸ� �ȵ�
void kSchedule() {
	TCB* pstRunningTask, *pstNextTask;
	BOOL bPreviousFlag;
	//�½�ũ�� �־�� ��
	if(kGetReadyTaskCount() < 1) {
		return;
	}

	//���ͷ�Ʈ ��Ȱ��ȭ
	bPreviousFlag = kLockForSystemData();
	pstNextTask = kGetNextTaskToRun();
	if(pstNextTask==NULL) {
		kUnlockForSystemData(bPreviousFlag);
		return;
	}

	//������ running task�� ready ť�� �ִ´�
	pstRunningTask = gs_stScheduler.pstRunningTask;
	//���� task�� runningTask�� ���
	gs_stScheduler.pstRunningTask = pstNextTask;

	//idle �½�ũ���� ��ȯ�Ǿ��ٸ� ����� ���μ��� �ð��� ������Ŵ
	if((pstRunningTask->qwFlags & TASK_FLAGS_IDLE)==TASK_FLAGS_IDLE) {
		gs_stScheduler.qwSpendProcessorTimeInIdleTask+=TASK_PROCESSORTIME - gs_stScheduler.iProcessorTime;
	}

	gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;

	if(gs_stScheduler.qwLastFPUUsedTaskID != pstNextTask->stLink.qwID)
		kSetTS();
	else
		kClearTS();

	//���� wait List�� �Ű����ٸ� ���� context�� ������ �ʿ䰡 ����
	if(pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK) {
		kAddListToTail(&(gs_stScheduler.stWaitList),pstRunningTask);
		kSwitchContext(NULL,&(pstNextTask->stContext));
	}
	else {
		kAddTaskToReadyList(pstRunningTask);


		kSwitchContext(&(pstRunningTask->stContext),&(pstNextTask->stContext));


	}

	kUnlockForSystemData(bPreviousFlag);

}

//Ÿ�̸� ���ͷ�Ʈ �� �½�ũ ��ȯ �۾��� ������ �Լ�
BOOL kScheduleInInterrupt() {
	TCB* pstRunningTask, * pstNextTask;
	char* pcContextAddress;
	BOOL bPreviousFlag;

	//�Ӱ� ���� ����
	bPreviousFlag = kLockForSystemData();

	pstNextTask = kGetNextTaskToRun();
	if(pstNextTask==NULL) {
		kUnlockForSystemData(bPreviousFlag);
		return FALSE;
	}

	pcContextAddress = (char*)IST_STARTADDRESS + IST_SIZE - sizeof(CONTEXT);

	pstRunningTask = gs_stScheduler.pstRunningTask;
	gs_stScheduler.pstRunningTask = pstNextTask;

	//Idle �½�ũ���� ��ȯ�Ǿ��ٸ� ����� ���μ��� �ð��� ������Ŵ
	if((pstRunningTask->qwFlags & TASK_FLAGS_IDLE)==TASK_FLAGS_IDLE) {
		gs_stScheduler.qwSpendProcessorTimeInIdleTask += TASK_PROCESSORTIME;
	}

	//���� ENDTASK Flag�� �����Ǿ��ٸ�, ���ؽ�Ʈ ���� ���� wait list�� ����
	if(pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK) {
		kAddListToTail(&(gs_stScheduler.stWaitList),pstRunningTask);
	}
	//�̿��� ���, IST�� ����Ǿ��ִ� ���ؽ�Ʈ�� �½�ũ�� ReadyList�� �����Ѵ�.
	else {
		kMemCpy(&(pstRunningTask->stContext),pcContextAddress,sizeof(CONTEXT));
		kAddTaskToReadyList(pstRunningTask);
	}

	//�Ӱ� ���� ��
	kUnlockForSystemData(bPreviousFlag);

	if(gs_stScheduler.qwLastFPUUsedTaskID != pstNextTask->stLink.qwID)
		kSetTS();
	else
		kClearTS();

	//IST�� ��ȯ�� �½�ũ�� context�� �����
	//���ͷ�Ʈ ��ƾ���� ���ƿ� ��, IST�� �ִ� context�� pop�ϱ� ������, ��� context�� �½�ũ ��ȯ�� �̷������.
	kMemCpy(pcContextAddress,&(pstNextTask->stContext),sizeof(CONTEXT));

	gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;
	return TRUE;
}

void kDecreaseProcessorTime() {
	if(gs_stScheduler.iProcessorTime>0)
		gs_stScheduler.iProcessorTime--;
}

BOOL kIsProcessorTimeExpired() {
	if(gs_stScheduler.iProcessorTime==0)
		return TRUE;
	return FALSE;
}

static TCB* kRemoveTaskFromReadyList(QWORD qwTaskID) {
	TCB* pstTarget;
	BYTE bPriority;
	//��ȿ���� ���� ID�� ���
	if(GETTCBOFFSET(qwTaskID)>=TASK_MAXCOUNT)
		return NULL;
	//TCB Ǯ���� �ش� ID�� �´� TCB�� ã��
	pstTarget = &(gs_stTCBPoolManager.pstStartAddress[GETTCBOFFSET(qwTaskID)]);
	if(pstTarget->stLink.qwID!=qwTaskID)
		return NULL;

	bPriority = GETPRIORITY(pstTarget->qwFlags);
	pstTarget = kRemoveList(&(gs_stScheduler.vstReadyList[bPriority]),qwTaskID);
	return pstTarget;
}

BOOL kChangePriority(QWORD qwTaskID, BYTE bPriority) {
	//�켱������ �ٲ� �Ŀ�, �ش��ϴ� ����Ʈ�� �ű��.

	TCB* pstTarget;
	BOOL bPreviousFlag;
	if(bPriority>TASK_MAXREADYLISTCOUNT)
		return FALSE;

	//�Ӱ迵�� ����
	bPreviousFlag = kLockForSystemData();

	//���� �������� �½�ũ�� ��� �켱������ �����Ѵ�
	//�̷����� ��� ������ IRQ0�� �߻����� �� ����� �켱������ ����Ʈ�� �Ű�����.
	//�ֳ��ϸ� IRQ0�� �߻��� ��� �½�ũ ��ȯ�� �߻��ϴµ�, �켱���� ���� ���� � ���� ����Ʈ�� ������ �޶����� ����
	pstTarget = gs_stScheduler.pstRunningTask;
	if(pstTarget->stLink.qwID==qwTaskID) {
		SETPRIORITY(pstTarget->qwFlags,bPriority);
	}
	//���� ���������� ���� �½�ũ�� ���, ��� �켱������ �´� ����Ʈ�� �Ű��ش�.
	else {
		//���� ����Ʈ���� �ش��ϴ� ID�� �½�ũ�� ã�´�.
		pstTarget = kRemoveTaskFromReadyList(qwTaskID);
		//���� ��ã���� ��� TCBPool���� ã�´�.
		if(pstTarget==NULL) {
			pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
			if(pstTarget!=NULL) {
				SETPRIORITY(pstTarget->qwFlags, bPriority);
			}
		}
		else {
			//���𸮽�Ʈ���� �ش��ϴ� ����Ʈ�� ã�� ���, priority�� �ٽ� �����ϰ�
			//�ٲ� priority�� ���� �ش��ϴ� ����Ʈ�� ����
			SETPRIORITY(pstTarget->qwFlags, bPriority);
			kAddTaskToReadyList(pstTarget);
		}
	}
	//�Ӱ� ���� ��
	kUnlockForSystemData(bPreviousFlag);
	kPrintf("Change Success\n");

	return TRUE;
}

BOOL kEndTask(QWORD qwTaskID) {
	TCB* pstTarget;
	BYTE bPriority;
	BOOL bPreviousFlag;

	bPreviousFlag = kLockForSystemData();
	pstTarget = gs_stScheduler.pstRunningTask;
	//���� �������� �½�ũ�� ���, EndTask ��Ʈ�� �����ϰ� �½�ũ�� ��ȯ
	if(pstTarget->stLink.qwID==qwTaskID){
		pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
		SETPRIORITY(pstTarget->qwFlags,TASK_FLAGS_WAIT);

		kUnlockForSystemData(bPreviousFlag);

		kSchedule();
		//�½�ũ�� ��ȯ�ǹǷ� ���� �ڵ�� ����ɼ� ����.
		while(1);
	}
	//�������� �½�ũ�� �ƴϸ� ready list �Ǵ� TCB pool���� ���� ã�Ƽ� wait list�� �ű��.
	else {
		pstTarget = kRemoveTaskFromReadyList(qwTaskID);
		if(pstTarget==NULL) {
			pstTarget=kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
			if(pstTarget!=NULL) {
				pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
				SETPRIORITY(pstTarget->qwFlags,TASK_FLAGS_WAIT);
			}
			kUnlockForSystemData(bPreviousFlag);
			return TRUE;
		}
		pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
		SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
		kAddListToTail(&(gs_stScheduler.stWaitList),pstTarget);
	}
	kUnlockForSystemData(bPreviousFlag);
	return TRUE;
}

void kExitTask() {
	//�ڽ��� �����ϴ� �Լ�
	kEndTask(gs_stScheduler.pstRunningTask->stLink.qwID);
}

int kGetReadyTaskCount() {
	int iTotalCount=0;
	int i;
	BOOL bPreviousFlag;

	bPreviousFlag = kLockForSystemData();

	for(i=0; i<TASK_MAXREADYLISTCOUNT; i++)
		iTotalCount+=kGetListCount(&(gs_stScheduler.vstReadyList[i]));
	kUnlockForSystemData(bPreviousFlag);
	return iTotalCount;
}

int kGetTaskCount() {
	int iTotalCount;
	BOOL bPreviousFlag;

	iTotalCount = kGetReadyTaskCount();
	//wait List + ���� �������� �½�ũ

	bPreviousFlag = kLockForSystemData();
	iTotalCount += kGetListCount(&(gs_stScheduler.stWaitList)) + 1;
	kUnlockForSystemData(bPreviousFlag);
	return iTotalCount;
}

TCB* kGetTCBInTCBPool(int iOffset) {
	if((iOffset< -1) || (iOffset > TASK_MAXCOUNT))
		return NULL;
	return &(gs_stTCBPoolManager.pstStartAddress[iOffset]);
}

BOOL kIsTaskExist(QWORD qwID) {
	TCB* pstTCB;

	pstTCB = kGetTCBInTCBPool(GETTCBOFFSET(qwID));

	if((pstTCB==NULL) || (pstTCB->stLink.qwID!=qwID))
		return FALSE;
	return TRUE;
}

QWORD kGetProcessorLoad() {
	return gs_stScheduler.qwProcessorLoad;
}

void kIdleTask() {
	//���μ��� ��뷮�� ����ϴ� �Լ�
	//ó������ qwCurrent�� qwLast�� ���� ��������, kSchedule()�Լ��� �����ϰ� �ٸ� �½�ũ���� ������ ��
	//�ٽ� �� �½�ũ�� �Ѿ�� ���� qwCurrent�� ���� �ٲ� ���̴�.
	//���μ��� ��뷮 ������� �����ϰ� ���ϸ�, 100-(idle task�� ����ѽð�)/(��ü �ð�)*100
	TCB* pstTask, * pstChildThread, * pstProcess;
	QWORD qwLastMeasureTickCount, qwLastSpendTickInIdleTask;
	QWORD qwCurrentMeasureTickCount, qwCurrentSpendTickInIdleTask;
	BOOL bPreviousFlag;
	QWORD qwTaskID;
	int i, iCount;
	void* pstThreadLink;

	//���μ��� ��뷮 ����� ���� ���� ������ ����
	qwLastSpendTickInIdleTask = gs_stScheduler.qwSpendProcessorTimeInIdleTask;
	qwLastMeasureTickCount = kGetTickCount();

	//while���� ���鼭 ���μ��� ��뷮�� �����ϰ�, �������� ���� ���μ����� ���� ��
	while(1) {

		//���� ������ ����
		qwCurrentMeasureTickCount = kGetTickCount();
		qwCurrentSpendTickInIdleTask = gs_stScheduler.qwSpendProcessorTimeInIdleTask;

		//���μ��� ��뷮�� ���
		//100 - (idle task�� ����� ���μ��� �ð�)*100/(�ý��� ��ü���� ����� ���μ��� �ð�)
		if(qwCurrentMeasureTickCount - qwLastMeasureTickCount==0) {
			gs_stScheduler.qwProcessorLoad = 0;
		}
		else {
			gs_stScheduler.qwProcessorLoad = 100 - (qwCurrentSpendTickInIdleTask - qwLastSpendTickInIdleTask)*
					100/(qwCurrentMeasureTickCount = qwLastMeasureTickCount);
		}
		//���� ���¸� ���� ���¿� ����
		qwLastMeasureTickCount = qwCurrentMeasureTickCount;
		qwLastSpendTickInIdleTask = qwCurrentSpendTickInIdleTask;

		//���μ��� ��뷮�� ���� ���� ��
		kHaltProcessorByLoad();

		//��� ť�� ������� �½�ũ�� ������ �½�ũ ����
		if(kGetListCount(&(gs_stScheduler.stWaitList))>=0) {
			while(1) {
				bPreviousFlag = kLockForSystemData();
				pstTask = kRemoveListFromHeader(&(gs_stScheduler.stWaitList));
				if(pstTask==NULL) {
					kUnlockForSystemData(bPreviousFlag);
					break;
				}

				if(pstTask->qwFlags & TASK_FLAGS_PROCESS) {
					//���μ����� ������ �� �ڽ� �����尡 ������ �켱 �����带 ��������
					//���Ḧ �ϰ� �� �ٽ� �ڽ� ������ ����Ʈ�� ����
					iCount = kGetListCount(&(pstTask->stChildThreadList));

					for(i=0; i<iCount; i++) {
						//�켱 �ڽ� ������ ��ũ���� �ϳ��� ������.
						pstThreadLink = (TCB*)kRemoveListFromHeader(&(pstTask->stChildThreadList));
						if(pstThreadLink==NULL)
							break;
						//���� �ڽ� �����帵ũ�� �̿��Ͽ� �� �������� TCB�� ���Ѵ�.
						pstChildThread = GETTCBFROMTHREADLINK(pstThreadLink);

						//���� ������ ��ũ�� �ٽ� ���ڸ��� ������ �Ѵ�.
						//���Ŀ� �ش� �����尡 ����� �� �ڽĽ����尡 ���μ����� ã�� ������ ����Ʈ���� ���ŵǵ��� ��
						kAddListToTail(&(pstTask->stChildThreadList),&(pstChildThread->stThreadLink));

						//�ڽ� ������ ����(wait list�� ����)
						kEndTask(pstChildThread->stLink.qwID);
					}
					//�ڽ� �����尡 �������� ��� �����尡 �� ����� ������ ��ٷ����ϹǷ� �ٽ� wait list�� ����
					if(kGetListCount(&(pstTask->stChildThreadList))>0) {
						kAddListToTail(&(gs_stScheduler.stWaitList),pstTask);

						kUnlockForSystemData(bPreviousFlag);
						continue;
					}

					else {
						//
					}

				}
				else if(pstTask->qwFlags & TASK_FLAGS_THREAD) {
					pstProcess = kGetProcessByThread(pstTask);
					if(pstProcess!=NULL) {
						kPrintf("Thread is deleted\n");
						kRemoveList(&(pstProcess->stChildThreadList),pstTask->stLink.qwID);
					}
				}


				kPrintf("IDLE: Task ID[0x%q] is completely ended.\n",pstTask->stLink.qwID);
				qwTaskID = pstTask->stLink.qwID;
				kFreeTCB(qwTaskID);
				kUnlockForSystemData(bPreviousFlag);
			}
		}
		kSchedule();

	}
}

void kHaltProcessorByLoad() {
	if(gs_stScheduler.qwProcessorLoad<40) {
		kHlt();
		kHlt();
		kHlt();
	}
	else if(gs_stScheduler.qwProcessorLoad<80) {
		kHlt();
		kHlt();
	}
	else if (gs_stScheduler.qwProcessorLoad<95) {
		kHlt();
	}
}

static TCB* kGetProcessByThread(TCB* pstThread) {
	TCB* pstProcess;
	if(pstThread->qwFlags & TASK_FLAGS_PROCESS)
		return pstThread;

	pstProcess = kGetTCBInTCBPool(GETTCBOFFSET(pstThread->qwParentProcessID));

	if((pstProcess==NULL)||(pstProcess->stLink.qwID!=pstThread->qwParentProcessID))
		return NULL;
	return pstProcess;
}

QWORD kGetLastFPUUsedTaskID() {
	return gs_stScheduler.qwLastFPUUsedTaskID;
}

void kSetLastFPUUsedTaskID(QWORD qwTaskID) {
	gs_stScheduler.qwLastFPUUsedTaskID = qwTaskID;
}
