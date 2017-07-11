#include "Page.h"


void kSetPageEntryData(PTENTRY *pstEntry, DWORD dwUpperBaseAddress,
		DWORD dwLowerBaseAddress, DWORD dwLowerFlags, DWORD dwUpperFlags) {
	//�Ӽ� ����
	pstEntry->dwAtrributeAndLowerBaseAddress = dwLowerBaseAddress | dwLowerFlags;
	pstEntry->dwUpperBaseAddressAndEXB = (dwUpperBaseAddress & 0xff) | dwUpperFlags;
}


void kInitializePageTables() {
	PML4TENTRY *pstPML4TEntry;
	PDPTENTRY *pstPDPTEntry;
	PDENTRY *pstPDEntry;
	DWORD dwMappingAddress;
	int i;

	//PML$ ���̺� ����, ù ��Ʈ���� ������ ������ �κ��� ��� 0���� �ʱ�ȭ
	pstPML4TEntry = (PML4TENTRY*)0x100000;

	//������ ���丮 ���̺� �������� base address�� 0x101000���� ����
	kSetPageEntryData(&(pstPML4TEntry[0]), 0x00, 0x101000, PAGE_FLAGS_DEFAULT, 0);
	//������ ��Ʈ���� ��� 0���� ����

	for(i=1; i<PAGE_MAXENTRYCOUNT; i++)
		kSetPageEntryData(&(pstPML4TEntry[i]),0,0,0,0);
	//������ ���͸� ������ ���̺� ����

	pstPDPTEntry = (PDPTENTRY*)0x101000;


	for(i=0; i<64; i++) {
		kSetPageEntryData(&(pstPDPTEntry[i]),0,0x102000 + (i*PAGE_TABLESIZE),PAGE_FLAGS_DEFAULT,0);
	}

	for(i=64; i<PAGE_MAXENTRYCOUNT; i++) {
		kSetPageEntryData(&(pstPDPTEntry[i]),0,0,0,0);
	}

	//������ ���͸� ���̺� ����
	//�ϳ��� ������ ���͸��� 1GB���� ���� �����ϹǷ�(2�� 10��) �̹Ƿ� 64���� ��Ʈ���� �ʿ�
	pstPDEntry = (PDENTRY*)0x102000;
	dwMappingAddress = 0;
	for(i=0; i<PAGE_MAXENTRYCOUNT * 64; i++) {
		//���� ��巹������ 32bit���� ������ �ȵǹǷ� 20bit shift�ϰ� i ���ϰ� 12bit�� shift�ؼ� �� ���� ���� ��巹���� ��
		kSetPageEntryData(&(pstPDEntry[i]),(i*(PAGE_DEFAULTSIZE >>20 ))>>12,dwMappingAddress,
				PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS,0);
		dwMappingAddress +=PAGE_DEFAULTSIZE;
	}

}

