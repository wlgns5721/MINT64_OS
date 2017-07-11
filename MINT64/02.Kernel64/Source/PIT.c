#include "PIT.h"
#include "AssemblyUtility.h"
#include "Console.h"

void kInitializePIT(WORD wCount, BOOL bPeriodic) {
	//������ �ֱ�� ���°��� �ƴϸ� ���0
	kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_ONCE);
	//������ �ֱ�� ���� Ÿ�̸Ӷ�� ���2
	if (bPeriodic==TRUE) {
		kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_PERIODIC);
	}
	//�ʱ� ī��Ʈ �� ����
	kOutPortByte(PIT_PORT_COUNTER0, wCount);
	kOutPortByte(PIT_PORT_COUNTER0, wCount>>8);
}

WORD kReadCounter0() {
	BYTE bHighByte, bLowByte;
	WORD wTemp = 0;
	//latchĿ�ǵ带 �����ؼ� ī����0�� �ִ� ���� �е��� ��
	//���� ����Ʈ ���� �а�, ���� ����Ʈ�� �о�� ��
	kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_LATCH);

	bLowByte = kInPortByte(PIT_PORT_COUNTER0);
	bHighByte = kInPortByte(PIT_PORT_COUNTER0);

	wTemp = bHighByte;
	wTemp = (bHighByte<<8) | bLowByte;
	return wTemp;
}

void kWaitUsingDirectPIT(WORD wCount) {
	//PIT���� �ִ� 2����Ʈ���� ���� �� �����Ƿ� WORD�� ����
	WORD wLastCounter0;
	WORD wCurrentCounter0;

	kInitializePIT(0,TRUE);
	wLastCounter0 = kReadCounter0();
	while(1) {
		wCurrentCounter0 = kReadCounter0();
		//Counter�� ���� 1�� �����ϱ� ������, ���� ������ �ؾ� ��
		if(((wLastCounter0 - wCurrentCounter0)&0xffff)>=wCount)
			break;

	}
}

