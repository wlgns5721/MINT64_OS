#include "PIC.h"
#include "AssemblyUtility.h"
void kInitializePIC() {

	//ICW1, IC4 = 1, SNGL = 0, LTIM = 0
	//������-�����̺���, ����Ʈ����, icw4 ��뿩�θ� ����
	kOutPortByte(PIC_MASTER_PORT1,0x11);

	//ICW2, ���ͷ�Ʈ ����(0x20)
	//���ͷ�Ʈ�� �������� ����
	kOutPortByte(PIC_MASTER_PORT2,PIC_IRQSTARTVECTOR);

	//ICW3
	//�����̺� PIC ��Ʈ�ѷ��� ����Ǵ� ��ġ
	//2�� ���̹Ƿ� 0b100�� 4�� ����
	kOutPortByte(PIC_MASTER_PORT2,0x4);

	//ICW4
	kOutPortByte(PIC_MASTER_PORT2, 0x01);

	//�����̺� PIC ��Ʈ�ѷ� �ʱ�ȭ

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
	//���ͷ�Ʈ ó�� ��ƾ �Ϸ� �Ŀ� ó���� �Ϸ�Ǿ��ٰ� PIC�� �˷��ִ� �κ�
	kOutPortByte(PIC_MASTER_PORT1,0x20);

	//������-�����̺�� �����Ǿ� ���� ���, �����̺� PIC ��Ʈ�ѷ��� ���ͷ�Ʈ��
	//������ PIC ��Ʈ�ѷ��� EOI�� �����ؾ���
	if (iIRQNumber >= 8)
		kOutPortByte(PIC_SLAVE_PORT1,0x20);
}

