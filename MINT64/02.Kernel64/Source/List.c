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

	//다음 데이터가 없음을 설정
	pstLink = (LISTLINK*)pvItem;
	pstLink->pvNext = NULL;

	//만약 리스트가 비었으면 pvItem = Header = Tail
	if(pstList->pvHeader==NULL) {
		pstList->pvHeader = pvItem;
		pstList->pvTail = pvItem;
		pstList->iItemCount = 1;
		return;
	}

	pstLink = (LISTLINK*)pstList->pvTail;
	//원래 리스트의 제일 마지막 부분에 pvItem을 삽입
	pstLink->pvNext = pvItem;
	//Tail을 pvItem으로 설정
	pstList->pvTail = pvItem;
	pstList->iItemCount++;
}

void kAddListToHeader(LIST* pstList, void* pvItem) {
	LISTLINK* pstLink;

	//원래 헤더의 앞부분에 pvItem을 삽입
	pstLink = (LISTLINK*)pvItem;
	pstLink->pvNext = pstList->pvHeader;

	if(pstList->pvHeader==NULL) {
		pstList->pvHeader = pvItem;
		pstList->pvTail = pvItem;
		pstList->iItemCount = 1;
		return;
	}
	//리스트의 헤더를 pvItem으로 선언
	pstList->pvHeader = pvItem;
	pstList->iItemCount++;
}

void* kRemoveList(LIST* pstList, QWORD qwID) {
	LISTLINK* pstLink;
	LISTLINK* pstPreviousLink;

	pstPreviousLink = (LISTLINK*)pstList->pvHeader;
	for(pstLink = pstPreviousLink; pstLink!=NULL; pstLink = pstLink->pvNext) {
		//ID가 일치하는지 검사
		if(pstLink->qwID==qwID) {
			//만일 데이터가 하나밖에 없을 경우
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

