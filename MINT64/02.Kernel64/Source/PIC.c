#include "PIC.h"
#include "AssemblyUtility.h"
void kInitializePIC() {

	//ICW1, IC4 = 1, SNGL = 0, LTIM = 0
	//마스터-슬레이브방식, 엣지트리거, icw4 사용여부를 설정
	kOutPortByte(PIC_MASTER_PORT1,0x11);

	//ICW2, 인터럽트 벡터(0x20)
	//인터럽트의 오프셋을 전달
	kOutPortByte(PIC_MASTER_PORT2,PIC_IRQSTARTVECTOR);

	//ICW3
	//슬레이브 PIC 컨트롤러가 연결되는 위치
	//2번 핀이므로 0b100인 4로 설정
	kOutPortByte(PIC_MASTER_PORT2,0x4);

	//ICW4
	kOutPortByte(PIC_MASTER_PORT2, 0x01);

	//슬레이브 PIC 컨트롤러 초기화

	//ICW1
	kOutPortByte(PIC_SLAVE_PORT1, 0x11);

	//ICW2
	kOutPortByte(PIC_SLAVE_PORT2, PIC_IRQSTARTVECTOR + 8);

	//ICW3
	kOutPortByte(PIC_SLAVE_PORT2,0x02);

	//ICW4
	kOutPortByte(PIC_SLAVE_PORT2,0x01);
}

void kMaskPICInterrupt(WORD wIRQBitmask) {
	//OCW1  IRQ0~7
	kOutPortByte(PIC_MASTER_PORT2,(BYTE)wIRQBitmask);

	//OCW1 IRQ8~15
	kOutPortByte(PIC_MASTER_PORT2,(BYTE)(wIRQBitmask>>8));
}

void kSendEOIToPIC(int iIRQNumber) {
	//인터럽트 처리 루틴 완료 후에 처리가 완료되었다고 PIC에 알려주는 부분
	kOutPortByte(PIC_MASTER_PORT1,0x20);

	//마스터-슬레이브로 설정되어 있을 경우, 슬레이브 PIC 컨트롤러의 인터럽트라도
	//마스터 PIC 컨트롤러에 EOI를 전송해야함
	if (iIRQNumber >= 8)
		kOutPortByte(PIC_SLAVE_PORT1,0x20);
}

