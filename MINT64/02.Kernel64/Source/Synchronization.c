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
	//bLockFlag�� �ι�°����(FALSE)�� �񱳸� �Ͽ���, ���� ���� ���̸� ù��° ������ ���� ����° ���ڰ�(TRUE)�� �ȴ�.
	//��, �Ʒ� �ڵ忡���� bLockFlag�� FALSE�� ��� bLockFlag�� ���� TRUE�� �ٲ� ��, �Լ��� TRUE�� �����Ѵ�.
	//���� TRUE�� ��쿡�� bLockFlag�� ���� ������ �ʰ� �Լ��� FALSE�� ����
	if(kTestAndSet(&(pstMutex->bLockFlag),0,1)==FALSE) { //flag�� true�� ���
		//�̹� ����� ���� ��, ������ ID�� �½�ũ�� �ٽ� �ѹ� ��۷��� �Ѵٸ�, ī��Ʈ�� ������Ŵ
		if(pstMutex->qwTaskID==kGetRunningTask()->stLink.qwID) {
			pstMutex->dwLockCount++;
			return;
		}
		//��������� ������ ����ؼ� �ٸ� �½�ũ���� �纸
		while(kTestAndSet(&(pstMutex->bLockFlag),0,1)==FALSE)
			kSchedule();
	}
	pstMutex->dwLockCount = 1;
	pstMutex->qwTaskID = kGetRunningTask()->stLink.qwID;
}

void kUnlock(MUTEX* pstMutex) {
	//�̹� Ǯ���ְų�, ���ý��� ��� �½�ũ�� �ƴ� �ٸ� �½�ũ�� �����ҷ��� �ϸ� ����
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
