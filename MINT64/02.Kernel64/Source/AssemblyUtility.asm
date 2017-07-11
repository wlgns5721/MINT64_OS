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

;GDTR 레지스터 설정
kLoadGDTR:
	lgdt [rdi]
	ret

;TR 레지스터 설정
kLoadTR:
	ltr di
	ret

;IDTR 레지스터 설정
kLoadIDTR:
	lidt [rdi]
	ret

;인터럽트 활성화
kEnableInterrupt:
	sti
	ret

;인터럽트 비활성화
kDisableInterrupt:
	cli
	ret

;RFLAGS 레지스터 값을 읽음
kReadRFLAGS:
	pushfq
	pop rax
	ret

;타임 스탬프 값을 읽어서 리턴
kReadTSC:
	push rdx;
	rdtsc
	shl rdx,32
	or rax, rdx
	pop rdx
	ret

;프로세서를 쉬게 하기 위해 사용됨
kHlt:
	hlt
	hlt
	ret

;lock을 붙일 경우, 뒤의 명령어를 수행하는 동안 시스템 버스를 잠그고 다른 프로세서나 코어가 메모리에 접근할 수 없게 함
;cmpxchg a b 의 경우, ax값과 a의 값을 비교하여 만약 같을 경우 mov a, b를 수행하고, 다를 경우 mov ax, a를 수행한다.
;kTestAndSet은 두번째 인자와 첫번째 인자를 비교하여, 두 값이 같을 경우 첫번째 인자의 값을 세번째 인자의 값으로 교체하는 역할을 한다.
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

	mov ax, ds ;ds Segment 저장. ds를 직접 스택에 넣지 못하므로 값을 ax에 옮겨서 스택에 저장
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
	mov es, ax ;es, ds는 직접 pop할 수 없으므로 rax에 값을 저장 후 mov를 통해 복원
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
	pushfq ;RFLAGS 값을 푸쉬하는데, 밑의 cmp연산 후에 플래그 값이 바뀔 수가 있으므로 미리 백업
	cmp rdi, 0 ;rdi : RunningTask->stContext
	je .LoadContext ;Current Context가 null일 경우 바로 loadContext 수행
	popfq ;RFLAGS 레지스터 복원

	;현재 테스크의 컨텍스트를 저장
	push rax
	mov ax, ss
	mov qword[rdi+(23*8)] , rax ;rdi는 CurrentContext의 base address이므로, 제일 마지막 부분부터 ss를 저장

	mov rax, rbp;mov rbp, rsp에 의해 rbp 값에 rsp값이 들어감
	add rax, 16 ;kSwitchContext 함수를 호출할 때, return address 및, ebp를 삽입하므로, 그 부분을 제외한 rsp값을 저장
	mov qword[rdi + (22*8)], rax

	pushfq ;RFLAGS 값을 직접 넣을수 없으므로 rax에 저장을 해서 저장한다.
	pop rax
	mov qword[rdi+(21*8)], rax

	mov ax, cs
	mov qword[rdi+(20*8)], rax

	mov rax, qword[rbp+8] ;rbp+8은 return address이므로(64bit니까 32bit랑 헷갈리지 말자) 그 값을 저장
	mov qword[rdi+(19*8)], rax

	pop rax ;원래 rax, rbp값도 저장해야하므로 스택에서 꺼낸다
	pop rbp

	add rdi, (19*8)
	mov rsp, rdi ; rsp의 값을 rdi, 즉 CurrentContext의 특정부분을 가리키게 한 후, push를 통해 하나씩하나씩 역순으로 저장한다.
				 ; 자세한 구조는 Task.h를 참조
	sub rdi, (19*8) ;값을 원래대로 복원한다.

	KSAVECONTEXT

.LoadContext:
	mov rsp, rsi ;rsp값을 NextContext의 시작 부분을 가르키게 한다.
	KLOADCONTEXT ;gs부터 rbp까지 차례대로 load를 한다.
	iretq ; ss,RSP, RFLAGS, CS, RIP등을 load

;FPU 초기화
kInitializeFPU:
	finit
	ret

;FPU 레지스터를 컨텍스트 버퍼에 저장
;파라미터는 저장할 버퍼 어드레스
kSaveFPUContext:
	fxsave [rdi]
	ret

;FPU레지스터 로드
;파라미터는 로드할 컨텍스트 버퍼
kLoadFPUContext:
	fxrstor [rdi]
	ret

;cr0의 TS비트를 1로 세팅하여 FPU연산 수행시 예외 7번을 발생하도록 함
kSetTS:
	push rax
	mov rax, cr0
	or rax,0x08
	mov cr0, rax

	pop rax
	ret

;TS비트를 0으로 세팅
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
