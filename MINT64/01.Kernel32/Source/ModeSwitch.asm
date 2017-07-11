[BITS 32]

global kReadCPUID, kSwitchAndExecute64bitKernel

SECTION .text
;파라미터 : DWORD dwEAX, DWORD* pdwEAX, pdwEBX, pdwECX, pdwEDX

kReadCPUID:
	push ebp
	mov ebp, esp
	push eax
	push ebx
	push ecx
	push edx
	push esi

	;리턴값을 파라미터에 저장
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
	;CR4 컨트롤 레지스터의 PAE 비트를 1로 설정, OSXMMEXCPT, OSFXSR비트도 1로 설정
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov eax, cr4
	or eax, 0x620
	mov cr4, eax

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;CR3 먼트롤 레지스터에 PML4 테이블의 어드레스와 캐시 활성화
	;;;;;;;;;;;;;;;;;;;
	;;;;;;;;;;;;;;
	mov eax, 0x100000
	mov cr3, eax

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;IA32_EFER.LME를 1로 설정하여 IA-32e 모드 활성화
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov ecx, 0xc0000080
	rdmsr    ;ecx에 읽어올 MSR 레지스터의 address를 저장하고, eax와 edx에 읽은 값 저장 (edx는 상위, eax는 하위 비트를 읽음)

	or eax, 0x0100
	wrmsr ;rdmsr과 마찬가지로 수행

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;CR0 컨트롤 레지스터에 값 설정, NW = 0, CD = 0, PG = 1로 설정
	;TS = 1, EM = 0, MP = 1로 설정하여 FPU활성화
	mov eax, cr0
	or eax, 0xe000000e
	xor eax, 0x60000004
	mov cr0, eax

	jmp 0x08:0x200000

	jmp $
