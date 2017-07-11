[BITS 32]

global kReadCPUID, kSwitchAndExecute64bitKernel

SECTION .text
;�Ķ���� : DWORD dwEAX, DWORD* pdwEAX, pdwEBX, pdwECX, pdwEDX

kReadCPUID:
	push ebp
	mov ebp, esp
	push eax
	push ebx
	push ecx
	push edx
	push esi

	;���ϰ��� �Ķ���Ϳ� ����
	mov eax, dword[ebp+8]
	cpuid

	mov esi, dword[ebp+12]
	mov dword [esi], eax

	mov esi, dword[ebp+16]
	mov dword[esi], ebx

	mov esi, dword[ebp+20]
	mov dword[esi], ecx

	mov esi, dword[ebp+24]
	mov dword[esi], edx

	pop esi
	pop edx
	pop ecx
	pop ebx
	pop eax
	pop ebp
	ret

kSwitchAndExecute64bitKernel:
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;CR4 ��Ʈ�� ���������� PAE ��Ʈ�� 1�� ����, OSXMMEXCPT, OSFXSR��Ʈ�� 1�� ����
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov eax, cr4
	or eax, 0x620
	mov cr4, eax

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;CR3 ��Ʈ�� �������Ϳ� PML4 ���̺��� ��巹���� ĳ�� Ȱ��ȭ
	;;;;;;;;;;;;;;;;;;;
	;;;;;;;;;;;;;;
	mov eax, 0x100000
	mov cr3, eax

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;IA32_EFER.LME�� 1�� �����Ͽ� IA-32e ��� Ȱ��ȭ
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov ecx, 0xc0000080
	rdmsr    ;ecx�� �о�� MSR ���������� address�� �����ϰ�, eax�� edx�� ���� �� ���� (edx�� ����, eax�� ���� ��Ʈ�� ����)

	or eax, 0x0100
	wrmsr ;rdmsr�� ���������� ����

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;CR0 ��Ʈ�� �������Ϳ� �� ����, NW = 0, CD = 0, PG = 1�� ����
	;TS = 1, EM = 0, MP = 1�� �����Ͽ� FPUȰ��ȭ
	mov eax, cr0
	or eax, 0xe000000e
	xor eax, 0x60000004
	mov cr0, eax

	jmp 0x08:0x200000

	jmp $
