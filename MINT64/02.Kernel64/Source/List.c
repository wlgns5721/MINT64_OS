#include "List.h"

void kInitializeList(LIST* pstList) {
	pstList->iItemCount = 0;
	pstList->pvHeader = NULL;
	pstList->pvTail = NULL;
}

int kGetListCount(const LIST* pstList) {
	return pstList->iItemCount;
}

void kAddListToTail(LIST* pstList, void* pvItem) {
	LISTLINK *pstLink;

	//���� �����Ͱ� ������ ����
	pstLink = (LISTLINK*)pvItem;
	pstLink->pvNext = NULL;

	//���� ����Ʈ�� ������� pvItem = Header = Tail
	if(pstList->pvHeader==NULL) {
		pstList->pvHeader = pvItem;
		pstList->pvTail = pvItem;
		pstList->iItemCount = 1;
		return;
	}

	pstLink = (LISTLINK*)pstList->pvTail;
	//���� ����Ʈ�� ���� ������ �κп� pvItem�� ����
	pstLink->pvNext = pvItem;
	//Tail�� pvItem���� ����
	pstList->pvTail = pvItem;
	pstList->iItemCount++;
}

void kAddListToHeader(LIST* pstList, void* pvItem) {
	LISTLINK* pstLink;

	//���� ����� �պκп� pvItem�� ����
	pstLink = (LISTLINK*)pvItem;
	pstLink->pvNext = pstList->pvHeader;

	if(pstList->pvHeader==NULL) {
		pstList->pvHeader = pvItem;
		pstList->pvTail = pvItem;
		pstList->iItemCount = 1;
		return;
	}
	//����Ʈ�� ����� pvItem���� ����
	pstList->pvHeader = pvItem;
	pstList->iItemCount++;
}

void* kRemoveList(LIST* pstList, QWORD qwID) {
	LISTLINK* pstLink;
	LISTLINK* pstPreviousLink;

	pstPreviousLink = (LISTLINK*)pstList->pvHeader;
	for(pstLink = pstPreviousLink; pstLink!=NULL; pstLink = pstLink->pvNext) {
		//ID�� ��ġ�ϴ��� �˻�
		if(pstLink->qwID==qwID) {
			//���� �����Ͱ� �ϳ��ۿ� ���� ���
			if((pstLink==pstList->pvHeader) &&(pstLink==pstList->pvTail)) {
				pstList->pvHeader = NULL;
				pstList->pvTail = NULL;
			}
			else if (pstLink==pstList->pvHeader) {
				pstList->pvHeader = pstLink->pvNext;
			}
			else if(pstLink==pstList->pvTail) {
				pstList->pvTail = pstPreviousLink;
			}
			else {
				pstPreviousLink->pvNext = pstLink->pvNext;
			}
			pstList->iItemCount--;
			return pstLink;
		}
		pstPreviousLink = pstLink;
	}
	return NULL;
}

void* kRemoveListFromHeader(LIST* pstList) {
	LISTLINK* pstLink;
	if(pstList->iItemCount==0)
		return NULL;
	pstLink = (LISTLINK*)pstList->pvHeader;
	return kRemoveList(pstList,pstLink->qwID);
}

void* kRemoveListFromTail(LIST* pstList) {
	LISTLINK* pstLink;

	if(pstList->iItemCount==0) {
		return NULL;
	}
	pstLink = (LISTLINK*)pstList->pvTail;
	return kRemoveList(pstList, pstLink->qwID);
}

void* kFindList(const LIST* pstList, QWORD qwID) {
	LISTLINK* pstLink;
	for(pstLink = (LISTLINK*)pstList->pvHeader; pstLink!=NULL; pstLink = pstLink->pvNext) {
		if(pstLink->qwID==qwID)
			return pstLink;
	}
	return NULL;
}

void* kGetHeaderFromList(const LIST* pstList) {
	return pstList->pvHeader;
}

void* kGetTailFromList(const LIST* pstList) {
	return pstList->pvTail;
}

void* kGetNextFromList(const LIST* pstList, void* pstCurrent) {
	LISTLINK* pstLink;
	pstLink = (LISTLINK*)pstCurrent;
	return pstLink->pvNext;
}

