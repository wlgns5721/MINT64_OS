
#ifndef __TASK_H__
#define __TASK_H__

#include "Types.h"
#include "List.h"
////////////////////////////////////////////////////////////////////////////////
//
// 매크로
//
////////////////////////////////////////////////////////////////////////////////
// SS, RSP, RFLAGS, CS, RIP + ISR에서 저장하는 19개의 레지스터
#define TASK_REGISTERCOUNT     ( 5 + 19 )
#define TASK_REGISTERSIZE       8

// Context 자료구조의 레지스터 오프셋
#define TASK_GSOFFSET           0
#define TASK_FSOFFSET           1
#define TASK_ESOFFSET           2
#define TASK_DSOFFSET           3
#define TASK_R15OFFSET          4
#define TASK_R14OFFSET          5
#define TASK_R13OFFSET          6
#define TASK_R12OFFSET          7
#define TASK_R11OFFSET          8
#define TASK_R10OFFSET          9
#define TASK_R9OFFSET           10
#define TASK_R8OFFSET           11
#define TASK_RSIOFFSET          12
#define TASK_RDIOFFSET          13
#define TASK_RDXOFFSET          14
#define TASK_RCXOFFSET          15
#define TASK_RBXOFFSET          16
#define TASK_RAXOFFSET          17
#define TASK_RBPOFFSET          18
#define TASK_RIPOFFSET          19
#define TASK_CSOFFSET           20
#define TASK_RFLAGSOFFSET       21
#define TASK_RSPOFFSET          22
#define TASK_SSOFFSET           23

#define TASK_TCBPOOLADDRESS     0x800000
#define TASK_MAXCOUNT           1024

// 스택 풀과 스택의 크기
#define TASK_STACKPOOLADDRESS   ( TASK_TCBPOOLADDRESS + sizeof( TCB ) * TASK_MAXCOUNT )
// 스택의 기본 크기를 64Kbyte로 변경
#define TASK_STACKSIZE          8192

// 유효하지 않은 태스크 ID
#define TASK_INVALIDID          0xFFFFFFFFFFFFFFFF

// 태스크가 최대로 쓸 수 있는 프로세서 시간(5 ms)
#define TASK_PROCESSORTIME      5

//ready list의 수
#define TASK_MAXREADYLISTCOUNT 5

//태스크의 우선순위
#define TASK_FLAGS_HIGHEST	0
#define TASK_FLAGS_HIGH		1
#define TASK_FLAGS_MEDIUM	2
#define TASK_FLAGS_LOW		3
#define TASK_FLAGS_LOWEST	4
#define TASK_FLAGS_WAIT		0xff

//wait list를 위한 flag
#define TASK_FLAGS_ENDTASK	0x8000000000000000
#define TASK_FLAGS_SYSTEM	0x4000000000000000
#define TASK_FLAGS_PROCESS	0x2000000000000000
#define TASK_FLAGS_THREAD	0x1000000000000000
#define TASK_FLAGS_IDLE		0x0800000000000000

#define GETPRIORITY(x)	((x)&0xff)
#define SETPRIORITY(x,priority) ((x)=((x) & 0xffffffffffffff00)|\
		(priority))
#define GETTCBOFFSET(x) ((x)&0xffffffff)
#define GETTCBFROMTHREADLINK(x)	(TCB*)((QWORD)(x)-offsetof(TCB, \
		stThreadLink))

// 1바이트로 정렬
#pragma pack( push, 1 )

// 콘텍스트에 관련된 자료구조
typedef struct kContextStruct
{
    QWORD vqRegister[ TASK_REGISTERCOUNT ];
} CONTEXT;

// 태스크(프로세스 및 스레드)의 상태를 관리하는 자료구조
// FPU 콘텍스트가 추가되었기 때문에 자료구조의 크기가 16의 배수로 정렬되어야 함
typedef struct kTaskControlBlockStruct
{
	LISTLINK stLink;
	QWORD qwFlags;


    void* pvMemoryAddress;
    QWORD qwMemorySize;

    LISTLINK stThreadLink;

    QWORD qwParentProcessID;

    QWORD vqwFPUContext[64];

    LIST stChildThreadList;

    CONTEXT stContext;

    // 프로세스 메모리 영역의 시작과 크기
    void* pvStackAddress;
    QWORD qwStackSize;

    BOOL bFPUUsed;

    char vcPadding[11];

} TCB;

typedef struct kTCBPoolManagerStruct {
	TCB* pstStartAddress;
	int iMaxCount;
	int iUseCount;

	//TCB가 할당된 횟수
	int iAllocatedCount;
}TCBPOOLMANAGER;

typedef struct kSchedulerStruct {
	TCB* pstRunningTask;
	int iProcessorTime;
	LIST vstReadyList[TASK_MAXREADYLISTCOUNT];
	LIST stWaitList;

	int viExecuteCount[TASK_MAXREADYLISTCOUNT];
	//프로세서 부하를 계산하기 위한 필드
	QWORD qwProcessorLoad;

	//idle task에서 사용한 프로세서 시간
	QWORD qwSpendProcessorTimeInIdleTask;

	QWORD qwLastFPUUsedTaskID;

}SCHEDULER;

#pragma pack( pop )

static void kSetUpTask(TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress,
		void *pvStackAddress , QWORD qwStackSize);
static void kInitializeTCBPool( void );
static TCB* kAllocateTCB( void );
static void kFreeTCB( QWORD qwID );
TCB* kCreateTask( QWORD qwFlags,void* pvMemoryAddress,
		QWORD qwMemorySize, QWORD qwEntryPointAddress);

//==============================================================================
//  스케줄러 관련
//==============================================================================
void kInitializeScheduler( void );
void kSetRunningTask(TCB* pstTask );
TCB* kGetRunningTask();
static TCB* kGetNextTaskToRun();
void kSchedule( void );
BOOL kScheduleInInterrupt( void );
void kDecreaseProcessorTime();
BOOL kIsProcessorTimeExpired();
static BOOL kAddTaskToReadyList(TCB* pstTask);
static TCB* kRemoveTaskFromReadyList(QWORD qwTaskID);
BOOL kChangePriority(QWORD qwID, BYTE bPriority);
BOOL kEndTask(QWORD qwTaskID);
void kExitTask();
int kGetReadyTaskCount();
int kGetTaskCount();
TCB* kGetTCBInTCBPool(int iOffset);
BOOL kIsTaskExist(QWORD qwID);
QWORD kGetProcessorLoad();
static TCB* kGetProcessByThread(TCB* pstThread);

//////////////////////////////////////////////////////////////////////
//idle task 관련
//////////////////////////////////////////////////////////////////////
void kIdleTask();
void kHaltProcessorByLoad();

///////////////////////////////////
//FPU관련
///////////////////////////////////
QWORD kGetLastFPUUsedTaskID();
void kSetLastFPUUsedTaskID(QWORD qwTaskID);

#endif /*__TASK_H__*/

