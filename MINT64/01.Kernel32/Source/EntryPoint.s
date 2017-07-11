[ORG 0x00]
[BITS 16]
SECTION .text
;보호 모드커널의 가장 앞부분 코드. 보호모드 전환 및 초기화 작업 수행
START:
	mov ax,0x1000
	mov ds, ax
	mov es, ax

	;;;;;;;;;;;;;;;;;;;;;;
	;A20 gate 활성화
	;;;;;;;;;;;;;;;;;;;;;;
	;BIOS 서비스를 이용하여 A20 게이트 활성화
	mov ax, 0x2401
	int 0x15

	jc .A20GATEERROR
	jmp .A20GATESUCCESS

;BIOS 서비스를 이용한 A20게이트 활성화에 실패할 경우, 시스템 컨트롤 포트 이용
.A20GATEERROR:
	in al, 0x92
	or al, 0x02
	and al, 0xFE
	out 0x92, al

.A20GATESUCCESS:
	cli
	lgdt [GDTR]

	;보호 모드로 진입
	mov eax, 0x4000003b
	mov cr0, eax
	jmp dword 0x18: (PROTECTEDMODE - $$ + 0x10000)

[BITS 32]
PROTECTEDMODE:
	;보호 모드 커널용 데이터 세그먼트 디스크립터를 나머지 세그먼트에 저장
	mov ax, 0x20
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	;ss에도 0x10을 저장하는데... 이러면 0x100~0xffff까지가 스택영역같지만
	;책에서는 0x0000~0xffff까지가 스택 영역이라 표시하는데... 왜그럴까
	mov ss, ax
	mov esp, 0xfffe
	mov ebp, 0xfffe

	;메세지 출력
	push (SWITCHSUCCESSMESSAGE - $$ + 0x10000)
	push 2
	push 0
	call PRINTMESSAGE
	add esp, 12

	jmp dword 0x18: 0x10200

PRINTMESSAGE:
	push ebp
	mov ebp, esp
	push esi
	push edi
	push eax
	push ecx
	push edx

	mov eax, dword[ebp+12]
	mov esi, 160
	mul esi
	mov edi, eax

	mov eax, dword[ebp+8]
	mov esi, 2
	mul esi
	add edi, eax

	mov esi, dword[ebp+16]

.MESSAGELOOP:
	mov cl, byte[esi]
	cmp cl,0
	je .MESSAGEEND

	mov byte[edi+0xb8000], cl
	add esi, 1
	add edi, 2
	jmp .MESSAGELOOP

.MESSAGEEND:
	pop edx
	pop ecx
	pop eax
	pop edi
	pop esi
	pop ebp
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;
;데이터 영역
;;;;;;;;;;;;;;;;;;;;;;;;;
align 8, db 0
dw 0x0000
;GDTR 자료구조 정의
GDTR:
	dw GDTEND - GDT - 1
	dd (GDT - $$ + 0x10000)


GDT:
	NULLDescriptor:
		dw 0x0000
		dw 0x0000
		db 0x00
		db 0x00
		db 0x00
		db 0x00

	;IA-32e 커널	용 세그먼트 디스크립터
	IA_32eCODEDESCRIPTOR:
		dw 0xffff
		dw 0x0000
		db 0x00
		db 0x9a
		db 0xaf
		db 0x00
	IA_32eDATADESCRIPTOR:
		dw 0xffff
		dw 0x0000
		db 0x00
		db 0x92
		db 0xaf
		db 0x00
	;보호 모드 커널용 세그먼트 디스크립터
	CODEDESCRIPTOR:
		dw 0xffff
		dw 0x0000
		db 0x00
		db 0x9a
		db 0xcf
		db 0x00
	DATADESCRIPTOR:
		dw 0xffff
		dw 0x0000
		db 0x00
		db 0x92
		db 0xcf
		db 0x00
GDTEND:

;메시지 출력
SWITCHSUCCESSMESSAGE: db 'Switch To Protected Mode' ,0

times 512 - ($ - $$) db 0x00
