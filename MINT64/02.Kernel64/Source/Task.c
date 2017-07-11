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

	//RSP, RBP를 설정. 스택 최상위를 가리키는 것이 아니라 최상위-8을 가리키는 이유는
	//스택 최상위 address에 kExitTask의 address로 채울 예정이기 때문이다.
	pstTCB->stContext.vqRegister[TASK_RSPOFFSET] = (QWORD)pvStackAddress + qwStackSize - 8;
	pstTCB->stContext.vqRegister[TASK_RBPOFFSET] = (QWORD)pvStackAddress + qwStackSize - 8;
	//스택 최상위에 kExitTask 함수 address 삽입
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

	//테스크 풀의 어드레스 지정 및 초기화
	gs_stTCBPoolManager.pstStartAddress = (TCB*)TASK_TCBPOOLADDRESS;
	kMemSet(TASK_TCBPOOLADDRESS,0,sizeof(TCB)*TASK_MAXCOUNT);

	//TCB에 ID 할당한다. TCB의 필드에 있는 stLink는 LISTLINK 구조체
	//단지 ID만 할당할 뿐, 실제로 해당 TCB를 할당하려면 qwID의 상위 32비트 값이 0이 아닌값이 들어가야 한다.
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
		//비어있는 TCB를 찾는다
		//ID의 상위 32비트가 0이면 할당되지 않은 TCB
		if((gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID>>32)==0) {
			pstEmptyTCB = &(gs_stTCBPoolManager.pstStartAddress[i]);
			break;
		}
	}

	//상위 32비트를 0이 아닌 값으로 설정해서 할당된 TCB로 설정
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
	//TCB 초기화
	kMemSet(&(gs_stTCBPoolManager.pstStartAddress[i].stContext),0,sizeof(CONTEXT));
	//ID 설정
	gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;
	gs_stTCBPoolManager.iUseCount--;
}

TCB* kCreateTask(QWORD qwFlags, void* pvMemoryAddress, QWORD qwMemorySize,
		QWORD qwEntryPointAddress) {
	TCB* pstTask, * pstProcess;
	void* pvStackAddress;
	BOOL bPreviousFlag;

	//임계영역 시작
	bPreviousFlag = kLockForSystemData();

	//TCB pool에서 빈 공간을 찾아서 그 부분을 할당해준다.
	pstTask = kAllocateTCB();
	if(pstTask==NULL) {
		//임계 영역 끝
		kUnlockForSystemData(bPreviousFlag);
		return NULL;
	}

	//현재 테스크의 부모 프로세스를 리턴(만약 현재 태스크가 프로세스라면 본인 리턴)
	pstProcess = kGetProcessByThread(kGetRunningTask());
	if(pstProcess==NULL) {
		kFreeTCB(pstTask->stLink.qwID);
		kUnlockForSystemData(bPreviousFlag);
	}
	//쓰레드라면 부모 프로세스의 자식 스레드 리스트에 연결함
	if(qwFlags & TASK_FLAGS_THREAD) {
		pstTask->qwParentProcessID = pstProcess->stLink.qwID;
		pstTask->pvMemoryAddress = pstProcess->pvMemoryAddress;
		pstTask->qwMemorySize = pstProcess->qwMemorySize;

		kAddListToTail(&(pstProcess->stChildThreadList),&(pstTask->stThreadLink));
	}

	//프로세스일 경우 파라미터로 받은 값대로 설정
	else {
		pstTask->qwParentProcessID = pstProcess->stLink.qwID;
		pstTask->pvMemoryAddress = pvMemoryAddress;
		pstTask->qwMemorySize = qwMemorySize;
	}
	//thread의 id를 태스크 id와 동일하게 설정
	pstTask->stThreadLink.qwID = pstTask->stLink.qwID;
	kUnlockForSystemData(bPreviousFlag);

	//사용할 스택의 위치는 qwID의 값을 통해 인덱싱해서 할당
	pvStackAddress = (void*)(TASK_STACKPOOLADDRESS + (TASK_STACKSIZE * GETTCBOFFSET(pstTask->stLink.qwID)));


	//TCB를 설정한 후 레디 리스트에 삽입
	kSetUpTask(pstTask, qwFlags, qwEntryPointAddress, pvStackAddress, TASK_STACKSIZE);

	//자식 쓰레드 초기화
	kInitializeList(&(pstTask->stChildThreadList));

	bPreviousFlag = kLockForSystemData();
	//태스크를 ready list에 삽입
	kAddTaskToReadyList(pstTask);
	kUnlockForSystemData(bPreviousFlag);

	return pstTask;
}

////////////////////////////////
//스케쥴링 관련 함수
////////////////////////////////
void kInitializeScheduler() {
	int i;
	TCB* pstTask;
	kInitializeTCBPool();
	//ready 리스트 초기화
	for (i=0; i<TASK_MAXREADYLISTCOUNT; i++) {
		kInitializeList(&(gs_stScheduler.vstReadyList[i]));
		gs_stScheduler.viExecuteCount[i] = 0;
	}
	//wait 리스트 초기화
	kInitializeList(&(gs_stScheduler.stWaitList));

	//TCB를 할당받아 부팅을 수행한 태스크를 커널 최초의 프로세스로 설정
	pstTask = kAllocateTCB();
	gs_stScheduler.pstRunningTask = pstTask;
	pstTask->qwFlags = TASK_FLAGS_HIGHEST | TASK_FLAGS_PROCESS | TASK_FLAGS_SYSTEM;
	pstTask->qwParentProcessID = pstTask->stLink.qwID;
	pstTask->pvMemoryAddress = (void*)0x100000;
	pstTask->qwMemorySize = 0x500000;
	pstTask->pvStackAddress = (void*)0x600000;
	pstTask->qwStackSize = 0x100000;

	//프로세서 사용률을 계산하는데 사용하는 자료구조 초기화
	gs_stScheduler.qwSpendProcessorTimeInIdleTask = 0;
	gs_stScheduler.qwProcessorLoad = 0;

	//FPU를 사용한 태스크 ID를 유효하지 않은 값으로 설정
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
//다음으로 실행할 태스크를 얻음
static TCB* kGetNextTaskToRun() {
	TCB* pstTarget = NULL;
	int iTaskCount, i, j;

	//큐에 아직 남아있는 태스크는 있지만 최하위 우선순위 큐의 태스크까지 한번씩 수행되었고, 최하위 큐에서 태스크를 양보할 경우
	//다음으로 실행될 태스크가 선택되지 못하는 경우가 생기기 때문에 그런 경우를 위해 아래 코드를 두번 실행시킴으로써 태스크가 선택안되는 경우를 방지
	for(j=0; j<2; j++) {
		//높은 우선순위에서 낮은 우선순위까지 리스트를 확인하여 스케줄링할 태스크 선택
		for(i=0; i<TASK_MAXREADYLISTCOUNT; i++) {
			iTaskCount = kGetListCount(&(gs_stScheduler.vstReadyList[i]));
			//해당 리스트의 태스크 수와, 해당 리스크의 실행 수를 비교한다
			//즉, 해당 리스트의 모든 태스크가 한번씩 수행되었는지를 검사
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
	//우선순위에 따라서 해당 큐에 TCB 삽입
	//ex) 우선순위가 2라면 ReadyList[2]에 삽입
	BYTE bPriority;
	bPriority = GETPRIORITY(pstTask->qwFlags);
	if(bPriority>=TASK_MAXREADYLISTCOUNT)
		return FALSE;
	kAddListToTail(&(gs_stScheduler.vstReadyList[bPriority]),pstTask);
}

//다른 태스크로 전환하는 함수
//인터럽트나 예외가 발생했을 때 호출하면 안됨
void kSchedule() {
	TCB* pstRunningTask, *pstNextTask;
	BOOL bPreviousFlag;
	//태스크가 있어야 함
	if(kGetReadyTaskCount() < 1) {
		return;
	}

	//인터럽트 비활성화
	bPreviousFlag = kLockForSystemData();
	pstNextTask = kGetNextTaskToRun();
	if(pstNextTask==NULL) {
		kUnlockForSystemData(bPreviousFlag);
		return;
	}

	//기존의 running task를 ready 큐에 넣는다
	pstRunningTask = gs_stScheduler.pstRunningTask;
	//다음 task를 runningTask로 등록
	gs_stScheduler.pstRunningTask = pstNextTask;

	//idle 태스크에서 전환되었다면 사용한 프로세서 시간을 증가시킴
	if((pstRunningTask->qwFlags & TASK_FLAGS_IDLE)==TASK_FLAGS_IDLE) {
		gs_stScheduler.qwSpendProcessorTimeInIdleTask+=TASK_PROCESSORTIME - gs_stScheduler.iProcessorTime;
	}

	gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;

	if(gs_stScheduler.qwLastFPUUsedTaskID != pstNextTask->stLink.qwID)
		kSetTS();
	else
		kClearTS();

	//만약 wait List로 옮겨진다면 굳이 context를 저장할 필요가 없음
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

//타이머 인터럽트 때 태스크 전환 작업을 수행할 함수
BOOL kScheduleInInterrupt() {
	TCB* pstRunningTask, * pstNextTask;
	char* pcContextAddress;
	BOOL bPreviousFlag;

	//임계 영역 시작
	bPreviousFlag = kLockForSystemData();

	pstNextTask = kGetNextTaskToRun();
	if(pstNextTask==NULL) {
		kUnlockForSystemData(bPreviousFlag);
		return FALSE;
	}

	pcContextAddress = (char*)IST_STARTADDRESS + IST_SIZE - sizeof(CONTEXT);

	pstRunningTask = gs_stScheduler.pstRunningTask;
	gs_stScheduler.pstRunningTask = pstNextTask;

	//Idle 태스크에서 전환되었다면 사용한 프로세서 시간을 증가시킴
	if((pstRunningTask->qwFlags & TASK_FLAGS_IDLE)==TASK_FLAGS_IDLE) {
		gs_stScheduler.qwSpendProcessorTimeInIdleTask += TASK_PROCESSORTIME;
	}

	//만약 ENDTASK Flag가 설정되었다면, 컨텍스트 저장 없이 wait list에 저장
	if(pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK) {
		kAddListToTail(&(gs_stScheduler.stWaitList),pstRunningTask);
	}
	//이외의 경우, IST에 저장되어있는 컨텍스트의 태스크를 ReadyList에 저장한다.
	else {
		kMemCpy(&(pstRunningTask->stContext),pcContextAddress,sizeof(CONTEXT));
		kAddTaskToReadyList(pstRunningTask);
	}

	//임계 영역 끝
	kUnlockForSystemData(bPreviousFlag);

	if(gs_stScheduler.qwLastFPUUsedTaskID != pstNextTask->stLink.qwID)
		kSetTS();
	else
		kClearTS();

	//IST에 전환할 태스크의 context를 덮어쓴다
	//인터럽트 루틴에서 돌아올 때, IST에 있는 context를 pop하기 때문에, 덮어쓴 context로 태스크 전환이 이루어진다.
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
	//유효하지 않은 ID일 경우
	if(GETTCBOFFSET(qwTaskID)>=TASK_MAXCOUNT)
		return NULL;
	//TCB 풀에서 해당 ID에 맞는 TCB를 찾음
	pstTarget = &(gs_stTCBPoolManager.pstStartAddress[GETTCBOFFSET(qwTaskID)]);
	if(pstTarget->stLink.qwID!=qwTaskID)
		return NULL;

	bPriority = GETPRIORITY(pstTarget->qwFlags);
	pstTarget = kRemoveList(&(gs_stScheduler.vstReadyList[bPriority]),qwTaskID);
	return pstTarget;
}

BOOL kChangePriority(QWORD qwTaskID, BYTE bPriority) {
	//우선순위를 바꾼 후에, 해당하는 리스트로 옮긴다.

	TCB* pstTarget;
	BOOL bPreviousFlag;
	if(bPriority>TASK_MAXREADYLISTCOUNT)
		return FALSE;

	//임계영역 시작
	bPreviousFlag = kLockForSystemData();

	//현재 실행중인 태스크일 경우 우선순위만 변경한다
	//이렇게할 경우 다음에 IRQ0가 발생했을 때 변경된 우선순위의 리스트로 옮겨진다.
	//왜냐하면 IRQ0가 발생할 경우 태스크 전환이 발생하는데, 우선순위 값에 따라 어떤 레디 리스트에 넣을지 달라지기 때문
	pstTarget = gs_stScheduler.pstRunningTask;
	if(pstTarget->stLink.qwID==qwTaskID) {
		SETPRIORITY(pstTarget->qwFlags,bPriority);
	}
	//만약 실행중이지 않은 태스크일 경우, 즉시 우선순위에 맞는 리스트로 옮겨준다.
	else {
		//레디 리스트에서 해당하는 ID의 태스크를 찾는다.
		pstTarget = kRemoveTaskFromReadyList(qwTaskID);
		//만약 못찾았을 경우 TCBPool에서 찾는다.
		if(pstTarget==NULL) {
			pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
			if(pstTarget!=NULL) {
				SETPRIORITY(pstTarget->qwFlags, bPriority);
			}
		}
		else {
			//레디리스트에서 해당하는 리스트를 찾을 경우, priority를 다시 설정하고
			//바뀐 priority에 따라서 해당하는 리스트에 삽입
			SETPRIORITY(pstTarget->qwFlags, bPriority);
			kAddTaskToReadyList(pstTarget);
		}
	}
	//임계 영역 끝
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
	//현재 실행중인 태스크일 경우, EndTask 비트를 설정하고 태스크를 전환
	if(pstTarget->stLink.qwID==qwTaskID){
		pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
		SETPRIORITY(pstTarget->qwFlags,TASK_FLAGS_WAIT);

		kUnlockForSystemData(bPreviousFlag);

		kSchedule();
		//태스크가 전환되므로 다음 코드는 실행될수 없다.
		while(1);
	}
	//실행중인 태스크가 아니면 ready list 또는 TCB pool에서 직접 찾아서 wait list로 옮긴다.
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
	//자신을 종료하는 함수
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
	//wait List + 현재 수행중인 태스크

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
	//프로세서 사용량을 계산하는 함수
	//처음에는 qwCurrent와 qwLast의 값이 같겠지만, kSchedule()함수를 실행하고 다른 태스크들을 수행한 후
	//다시 이 태스크로 넘어올 때는 qwCurrent의 값이 바뀔 것이다.
	//프로세서 사용량 계산방법을 간략하게 말하면, 100-(idle task가 사용한시간)/(전체 시간)*100
	TCB* pstTask, * pstChildThread, * pstProcess;
	QWORD qwLastMeasureTickCount, qwLastSpendTickInIdleTask;
	QWORD qwCurrentMeasureTickCount, qwCurrentSpendTickInIdleTask;
	BOOL bPreviousFlag;
	QWORD qwTaskID;
	int i, iCount;
	void* pstThreadLink;

	//프로세서 사용량 계산을 위해 기준 정보를 저장
	qwLastSpendTickInIdleTask = gs_stScheduler.qwSpendProcessorTimeInIdleTask;
	qwLastMeasureTickCount = kGetTickCount();

	//while문을 돌면서 프로세서 사용량을 측정하고, 측정량에 따라서 프로세서를 쉬게 함
	while(1) {

		//기준 정보를 저장
		qwCurrentMeasureTickCount = kGetTickCount();
		qwCurrentSpendTickInIdleTask = gs_stScheduler.qwSpendProcessorTimeInIdleTask;

		//프로세서 사용량을 계산
		//100 - (idle task가 사용한 프로세서 시간)*100/(시스템 전체에서 사용한 프로세서 시간)
		if(qwCurrentMeasureTickCount - qwLastMeasureTickCount==0) {
			gs_stScheduler.qwProcessorLoad = 0;
		}
		else {
			gs_stScheduler.qwProcessorLoad = 100 - (qwCurrentSpendTickInIdleTask - qwLastSpendTickInIdleTask)*
					100/(qwCurrentMeasureTickCount = qwLastMeasureTickCount);
		}
		//현재 상태를 이전 상태에 보관
		qwLastMeasureTickCount = qwCurrentMeasureTickCount;
		qwLastSpendTickInIdleTask = qwCurrentSpendTickInIdleTask;

		//프로세서 사용량에 따라서 쉬게 함
		kHaltProcessorByLoad();

		//대기 큐에 대기중인 태스크가 있으면 태스크 종료
		if(kGetListCount(&(gs_stScheduler.stWaitList))>=0) {
			while(1) {
				bPreviousFlag = kLockForSystemData();
				pstTask = kRemoveListFromHeader(&(gs_stScheduler.stWaitList));
				if(pstTask==NULL) {
					kUnlockForSystemData(bPreviousFlag);
					break;
				}

				if(pstTask->qwFlags & TASK_FLAGS_PROCESS) {
					//프로세스를 종료할 때 자식 쓰레드가 있으면 우선 스레드를 먼저종료
					//종료를 하고난 후 다시 자식 스레드 리스트에 삽입
					iCount = kGetListCount(&(pstTask->stChildThreadList));

					for(i=0; i<iCount; i++) {
						//우선 자식 스레드 링크에서 하나씩 꺼낸다.
						pstThreadLink = (TCB*)kRemoveListFromHeader(&(pstTask->stChildThreadList));
						if(pstThreadLink==NULL)
							break;
						//꺼낸 자식 스레드링크를 이용하여 그 스레드의 TCB를 구한다.
						pstChildThread = GETTCBFROMTHREADLINK(pstThreadLink);

						//꺼낸 스레드 링크를 다시 제자리에 삽입을 한다.
						//이후에 해당 스레드가 종료될 때 자식스레드가 프로세스를 찾아 스스로 리스트에서 제거되도록 함
						kAddListToTail(&(pstTask->stChildThreadList),&(pstChildThread->stThreadLink));

						//자식 스레드 종료(wait list에 보냄)
						kEndTask(pstChildThread->stLink.qwID);
					}
					//자식 스레드가 남아있을 경우 스레드가 다 종료될 때까지 기다려야하므로 다시 wait list에 삽입
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
