#include "PIT.h"
#include "AssemblyUtility.h"
#include "Console.h"

void kInitializePIT(WORD wCount, BOOL bPeriodic) {
	//일정한 주기로 도는것이 아니면 모드0
	kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_ONCE);
	//일정한 주기로 도는 타이머라면 모드2
	if (bPeriodic==TRUE) {
		kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_PERIODIC);
	}
	//초기 카운트 값 설정
	kOutPortByte(PIT_PORT_COUNTER0, wCount);
	kOutPortByte(PIT_PORT_COUNTER0, wCount>>8);
}

WORD kReadCounter0() {
	BYTE bHighByte, bLowByte;
	WORD wTemp = 0;
	//latch커맨드를 전송해서 카운터0에 있는 값을 읽도록 함
	//하위 바이트 먼저 읽고, 상위 바이트를 읽어야 함
	kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_LATCH);

	bLowByte = kInPortByte(PIT_PORT_COUNTER0);
	bHighByte = kInPortByte(PIT_PORT_COUNTER0);

	wTemp = bHighByte;
	wTemp = (bHighByte<<8) | bLowByte;
	return wTemp;
}

void kWaitUsingDirectPIT(WORD wCount) {
	//PIT에서 최대 2바이트까지 읽을 수 있으므로 WORD로 선언
	WORD wLastCounter0;
	WORD wCurrentCounter0;

	kInitializePIT(0,TRUE);
	wLastCounter0 = kReadCounter0();
	while(1) {
		wCurrentCounter0 = kReadCounter0();
		//Counter의 값은 1씩 감소하기 때문에, 빼기 연산을 해야 함
		if(((wLastCounter0 - wCurrentCounter0)&0xffff)>=wCount)
			break;

	}
}

