[ORG 0x00]
[BITS 16]
SECTION .text

jmp 0x07c0:START ;cs 세그먼트 레지스터에 0x07c0을 복사 후 start레이블로 이동


;;;;;;;;;;;;;;;;;;;;;;;;;
;OS에 관련된 설정값
;;;;;;;;;;;;;;;;;;;;;;;;;


TOTALSECTORCOUNT:	dw	0x02  ;부트로더 제외한 OS 이미지 크기
KERNEL32SECTORCOUNT: dw 0x02

START:
	mov ax, 0x07c0   ;부트로더의 시작 부분을 0x07c00으로 설정
	mov ds, ax
	mov ax, 0xb800   ; 비디오 메모리 address를 0xb8000으로 설정
	mov es, ax

	;스택 색성
	mov ax, 0x000    ;스택 시작 address, 크기는 0x10000
	mov ss, ax
	mov sp, 0xfffe
	mov bp, 0xfffe

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;화면 지운 후 속성값 설정
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov si, 0

.SCREENCLEARLOOP:
	mov byte[es:si],0
	mov byte[es:si + 1],0x0a
	add si, 2
	cmp si, 80*25*2
	jl .SCREENCLEARLOOP

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;화면상단에 시작 메시지 출력
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	push MESSAGE1
	push 0
	push 0
	call PRINTMESSAGE      ;파라미터 3개를 스택에 넣은 후 함수 호출
	add sp, 6     ;스택 정리

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;OS 이미지를 로딩한다는 메시지 출력
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	push IMAGELOADINGMESSAGE
	push 1
	push 0
	call PRINTMESSAGE
	add sp, 6

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;디스크에서 OS 이미지 로딩
	;디스크를 읽기 전에 먼저 리셋
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
RESETDISK:               ;디스크 리셋 코드 부분
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;BIOS reset Function 호출
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov ax, 0     ;서비스 번호 0 , 드라이브 번호 0(floppy)
	mov dl, 0
	int 0x13
	jc HANDLEDISKERROR   ;에러시 처리 루틴으로 이동



	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;디스크에서 섹터를 읽음
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov si,0x1000
	mov es,si
	mov bx,0x0000

	mov di, word[TOTALSECTORCOUNT] ;복사할 OS이미지의 섹터 수를 저장

READDATA:
	cmp di,0
	je READEND
	sub di,0x1

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;BIOS READ Function 호출
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov ah, 0x02
	mov al,0x1
	mov ch, byte[TRACKNUMBER]
	mov cl, byte[SECTORNUMBER]
	mov dh, byte[HEADNUMBER]
	mov dl, 0x00
	int 0x13
	jc HANDLEDISKERROR

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;복사할 어드레스와 트랙, 헤드,섹터 어드레스 계산
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	add si, 0x0020
	mov es, si

	mov al, byte[SECTORNUMBER]
	add al, 0x01
	mov byte[SECTORNUMBER], al
	cmp al,19   ;마지막 섹터(18)까지 읽었는지 체크
	jl READDATA   ;아니면 READDATA로 점프

	xor byte[HEADNUMBER], 0x01
	mov byte[SECTORNUMBER], 0x01

	;head의 값이 1에서 0으로 전환되면 양쪽을 모두 읽은 것이므로
	;track의 값을 증가시킴
	cmp byte[HEADNUMBER] ,0x00  ;헤드를 두번 읽었을 경우 값이 0이 됨
	jne READDATA
	add byte[TRACKNUMBER], 0x01  ;트랙 값을 1 증가 시킴
	jmp READDATA

READEND:
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;OS 이미지가 완료되었다는 메시지 출력
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	push LOADINGCOMPLETEMESSAGE
	push 1
	push 20
	call PRINTMESSAGE
	add sp,6

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;로딩한 가상 OS 이미지 실행
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	jmp 0x1000:0x0000


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;함수 코드 영역
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
HANDLEDISKERROR:
	push DISKERRORMESSAGE
	push 1
	push 20
	call PRINTMESSAGE

	jmp $

;메시지 출력 함수
;PARAM: x좌표, y좌표, 문자열
PRINTMESSAGE:
	push bp
	mov bp, sp
	push es    ;각종 레지스터 저장, 함수 끝날 때 pop함으로써 레지스터값 복원
	push si
	push di
	push ax
	push cx
	push dx

	;es에 비디오 모드 어드레스 설정
	mov ax, 0xb800
	mov es,ax

	;라인 어드레스 계산
	mov ax, word[bp+6]  ;16bit이므로 2바이트씩 계산, ax에 y좌표 설정
	mov si, 160
	mul si
	mov di,ax  ;계산된 column값을 di에 저장

	mov ax, word[bp+4] ;x좌표값 설정
	mov si, 2
	mul si
	add di, ax

	mov si, word[bp+8]   ;문자열 파라미터

.MESSAGELOOP:
	mov cl, byte[si]   ;문자열 한글자 복사
	cmp cl,0  ;문자열 끝인지 검사
	je .MESSAGEEND
	mov byte[es:di], cl   ;문자 출력
	add di, 2
	add si, 1
	jmp .MESSAGELOOP

.MESSAGEEND:
	pop dx
	pop cx
	pop ax
	pop di
	pop si
	pop es
	pop bp
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;
;데이터 영역
;;;;;;;;;;;;;;;;;;;;;;;;;
MESSAGE1:	db 'MINT64 Boot Loader...', 0
DISKERRORMESSAGE:	db	'Error!' ,0
IMAGELOADINGMESSAGE:	db	'OS IMAGE Loading....', 0
LOADINGCOMPLETEMESSAGE:	db	'Complete!' , 0
RESETCOMPLETEMESSAGE: db	'Reset', 0
SECTORNUMBER:	db	0x02
HEADNUMBER:	db	0x00
TRACKNUMBER:	db	0x00

times 510 - ($ - $$)	db	0x00
db 0x55
db 0xAA
