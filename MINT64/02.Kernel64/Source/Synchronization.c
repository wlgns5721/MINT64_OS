#include "Synchronization.h"
#include "Utility.h"
#include "Task.h"
#include "AssemblyUtility.h"

BOOL kLockForSystemData() {
	return kSetInterruptFlag(FALSE);
}

void kUnlockForSystemData(BOOL bInterruptFlag) {
	kSetInterruptFlag(bInterruptFlag);
}

void kInitializeMutex(MUTEX* pstMutex) {
	pstMutex->bLockFlag = FALSE;
	pstMutex->dwLockCount = 0;
	pstMutex->qwTaskID = TASK_INVALIDID;
}

void kLock(MUTEX* pstMutex) {
	//bLockFlag와 두번째인자(FALSE)를 비교를 하여서, 만약 같은 값이면 첫번째 인자의 값은 세번째 인자값(TRUE)가 된다.
	//즉, 아래 코드에서는 bLockFlag가 FALSE일 경우 bLockFlag의 값을 TRUE로 바꾼 뒤, 함수는 TRUE를 리턴한다.
	//만약 TRUE일 경우에는 bLockFlag의 값이 변하지 않고 함수는 FALSE를 리턴
	if(kTestAndSet(&(pstMutex->bLockFlag),0,1)==FALSE) { //flag가 true일 경우
		//이미 잠겨져 있을 때, 동일한 ID의 태스크가 다시 한번 잠글려고 한다면, 카운트를 증가시킴
		if(pstMutex->qwTaskID==kGetRunningTask()->stLink.qwID) {
			pstMutex->dwLockCount++;
			return;
		}
		//잠금해제될 때까지 계속해서 다른 태스크에게 양보
		while(kTestAndSet(&(pstMutex->bLockFlag),0,1)==FALSE)
			kSchedule();
	}
	pstMutex->dwLockCount = 1;
	pstMutex->qwTaskID = kGetRunningTask()->stLink.qwID;
}

void kUnlock(MUTEX* pstMutex) {
	//이미 풀려있거나, 뮤택스를 잠근 태스크가 아닌 다른 태스크가 해제할려고 하면 종료
	if((pstMutex->bLockFlag==FALSE) || (pstMutex->qwTaskID!=kGetRunningTask()->stLink.qwID))
		return;

	if(pstMutex->dwLockCount>1) {
		pstMutex->dwLockCount--;
		return;
	}
	pstMutex->qwTaskID = TASK_INVALIDID;
	pstMutex->dwLockCount = 0;
	pstMutex->bLockFlag = FALSE;
}
