[BITS 64]


SECTION .text

global kInPortByte, kOutPortByte, kInPortWord, kOutPortWord, kLoadGDTR, kLoadTR, kLoadIDTR
global kReadRFLAGS, kEnableInterrupt, kDisableInterrupt, kReadTSC
global kSwitchContext, kHlt, kTestAndSet
global kInitializeFPU, kSaveFPUContext, kLoadFPUContext, kSetTS, kClearTS

kInPortByte:
	push rdx
	mov rdx, rdi
	mov rax, 0
	in al, dx
	pop rdx
	ret


kOutPortByte:
	push rdx
	push rax
	mov rdx, rdi
	mov rax, rsi
	out dx, al
	pop rax
	pop rdx
	ret

;GDTR �������� ����
kLoadGDTR:
	lgdt [rdi]
	ret

;TR �������� ����
kLoadTR:
	ltr di
	ret

;IDTR �������� ����
kLoadIDTR:
	lidt [rdi]
	ret

;���ͷ�Ʈ Ȱ��ȭ
kEnableInterrupt:
	sti
	ret

;���ͷ�Ʈ ��Ȱ��ȭ
kDisableInterrupt:
	cli
	ret

;RFLAGS �������� ���� ����
kReadRFLAGS:
	pushfq
	pop rax
	ret

;Ÿ�� ������ ���� �о ����
kReadTSC:
	push rdx;
	rdtsc
	shl rdx,32
	or rax, rdx
	pop rdx
	ret

;���μ����� ���� �ϱ� ���� ����
kHlt:
	hlt
	hlt
	ret

;lock�� ���� ���, ���� ��ɾ �����ϴ� ���� �ý��� ������ ��װ� �ٸ� ���μ����� �ھ �޸𸮿� ������ �� ���� ��
;cmpxchg a b �� ���, ax���� a�� ���� ���Ͽ� ���� ���� ��� mov a, b�� �����ϰ�, �ٸ� ��� mov ax, a�� �����Ѵ�.
;kTestAndSet�� �ι�° ���ڿ� ù��° ���ڸ� ���Ͽ�, �� ���� ���� ��� ù��° ������ ���� ����° ������ ������ ��ü�ϴ� ������ �Ѵ�.
kTestAndSet:
	mov rax, rsi
	lock cmpxchg byte [rdi], dl
	je .SUCCESS

.NOTSAME:
	mov rax, 0x00
	ret

.SUCCESS:
	mov rax, 0x01
	ret




%macro KSAVECONTEXT 0
	push rbp
	push rax
	push rbx
	push rcx
	push rdx
	push rdi
	push rsi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15

	mov ax, ds ;ds Segment ����. ds�� ���� ���ÿ� ���� ���ϹǷ� ���� ax�� �Űܼ� ���ÿ� ����
	push rax

	mov ax, es
	push rax
	push fs
	push gs
%endmacro

%macro KLOADCONTEXT 0
	pop gs
	pop fs
	pop rax
	mov es, ax ;es, ds�� ���� pop�� �� �����Ƿ� rax�� ���� ���� �� mov�� ���� ����
	pop rax
	mov ds, ax

	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rsi
	pop rdi
	pop rdx
	pop rcx
	pop rbx
	pop rax
	pop rbp
%endmacro

kSwitchContext:
	push rbp
	mov rbp, rsp
	pushfq ;RFLAGS ���� Ǫ���ϴµ�, ���� cmp���� �Ŀ� �÷��� ���� �ٲ� ���� �����Ƿ� �̸� ���
	cmp rdi, 0 ;rdi : RunningTask->stContext
	je .LoadContext ;Current Context�� null�� ��� �ٷ� loadContext ����
	popfq ;RFLAGS �������� ����

	;���� �׽�ũ�� ���ؽ�Ʈ�� ����
	push rax
	mov ax, ss
	mov qword[rdi+(23*8)] , rax ;rdi�� CurrentContext�� base address�̹Ƿ�, ���� ������ �κк��� ss�� ����

	mov rax, rbp;mov rbp, rsp�� ���� rbp ���� rsp���� ��
	add rax, 16 ;kSwitchContext �Լ��� ȣ���� ��, return address ��, ebp�� �����ϹǷ�, �� �κ��� ������ rsp���� ����
	mov qword[rdi + (22*8)], rax

	pushfq ;RFLAGS ���� ���� ������ �����Ƿ� rax�� ������ �ؼ� �����Ѵ�.
	pop rax
	mov qword[rdi+(21*8)], rax

	mov ax, cs
	mov qword[rdi+(20*8)], rax

	mov rax, qword[rbp+8] ;rbp+8�� return address�̹Ƿ�(64bit�ϱ� 32bit�� �򰥸��� ����) �� ���� ����
	mov qword[rdi+(19*8)], rax

	pop rax ;���� rax, rbp���� �����ؾ��ϹǷ� ���ÿ��� ������
	pop rbp

	add rdi, (19*8)
	mov rsp, rdi ; rsp�� ���� rdi, �� CurrentContext�� Ư���κ��� ����Ű�� �� ��, push�� ���� �ϳ����ϳ��� �������� �����Ѵ�.
				 ; �ڼ��� ������ Task.h�� ����
	sub rdi, (19*8) ;���� ������� �����Ѵ�.

	KSAVECONTEXT

.LoadContext:
	mov rsp, rsi ;rsp���� NextContext�� ���� �κ��� ����Ű�� �Ѵ�.
	KLOADCONTEXT ;gs���� rbp���� ���ʴ�� load�� �Ѵ�.
	iretq ; ss,RSP, RFLAGS, CS, RIP���� load

;FPU �ʱ�ȭ
kInitializeFPU:
	finit
	ret

;FPU �������͸� ���ؽ�Ʈ ���ۿ� ����
;�Ķ���ʹ� ������ ���� ��巹��
kSaveFPUContext:
	fxsave [rdi]
	ret

;FPU�������� �ε�
;�Ķ���ʹ� �ε��� ���ؽ�Ʈ ����
kLoadFPUContext:
	fxrstor [rdi]
	ret

;cr0�� TS��Ʈ�� 1�� �����Ͽ� FPU���� ����� ���� 7���� �߻��ϵ��� ��
kSetTS:
	push rax
	mov rax, cr0
	or rax,0x08
	mov cr0, rax

	pop rax
	ret

;TS��Ʈ�� 0���� ����
kClearTS:
	clts
	ret

kInPortWord:
	push rdx
	mov rdx, rdi
	mov rax, 0
	in ax, dx
	pop rdx
	ret

kOutPortWord:
	push rdx
	push rax
	mov rdx, rdi
	mov rax, rsi
	out dx, ax
	pop rax
	pop rdx
	ret
