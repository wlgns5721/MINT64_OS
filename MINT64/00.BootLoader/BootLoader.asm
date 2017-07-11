[ORG 0x00]
[BITS 16]
SECTION .text

jmp 0x07c0:START ;cs ���׸�Ʈ �������Ϳ� 0x07c0�� ���� �� start���̺�� �̵�


;;;;;;;;;;;;;;;;;;;;;;;;;
;OS�� ���õ� ������
;;;;;;;;;;;;;;;;;;;;;;;;;


TOTALSECTORCOUNT:	dw	0x02  ;��Ʈ�δ� ������ OS �̹��� ũ��
KERNEL32SECTORCOUNT: dw 0x02

START:
	mov ax, 0x07c0   ;��Ʈ�δ��� ���� �κ��� 0x07c00���� ����
	mov ds, ax
	mov ax, 0xb800   ; ���� �޸� address�� 0xb8000���� ����
	mov es, ax

	;���� ����
	mov ax, 0x000    ;���� ���� address, ũ��� 0x10000
	mov ss, ax
	mov sp, 0xfffe
	mov bp, 0xfffe

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;ȭ�� ���� �� �Ӽ��� ����
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov si, 0

.SCREENCLEARLOOP:
	mov byte[es:si],0
	mov byte[es:si + 1],0x0a
	add si, 2
	cmp si, 80*25*2
	jl .SCREENCLEARLOOP

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;ȭ���ܿ� ���� �޽��� ���
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	push MESSAGE1
	push 0
	push 0
	call PRINTMESSAGE      ;�Ķ���� 3���� ���ÿ� ���� �� �Լ� ȣ��
	add sp, 6     ;���� ����

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;OS �̹����� �ε��Ѵٴ� �޽��� ���
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	push IMAGELOADINGMESSAGE
	push 1
	push 0
	call PRINTMESSAGE
	add sp, 6

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;��ũ���� OS �̹��� �ε�
	;��ũ�� �б� ���� ���� ����
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
RESETDISK:               ;��ũ ���� �ڵ� �κ�
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;BIOS reset Function ȣ��
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov ax, 0     ;���� ��ȣ 0 , ����̺� ��ȣ 0(floppy)
	mov dl, 0
	int 0x13
	jc HANDLEDISKERROR   ;������ ó�� ��ƾ���� �̵�



	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;��ũ���� ���͸� ����
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov si,0x1000
	mov es,si
	mov bx,0x0000

	mov di, word[TOTALSECTORCOUNT] ;������ OS�̹����� ���� ���� ����

READDATA:
	cmp di,0
	je READEND
	sub di,0x1

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;BIOS READ Function ȣ��
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
	;������ ��巹���� Ʈ��, ���,���� ��巹�� ���
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	add si, 0x0020
	mov es, si

	mov al, byte[SECTORNUMBER]
	add al, 0x01
	mov byte[SECTORNUMBER], al
	cmp al,19   ;������ ����(18)���� �о����� üũ
	jl READDATA   ;�ƴϸ� READDATA�� ����

	xor byte[HEADNUMBER], 0x01
	mov byte[SECTORNUMBER], 0x01

	;head�� ���� 1���� 0���� ��ȯ�Ǹ� ������ ��� ���� ���̹Ƿ�
	;track�� ���� ������Ŵ
	cmp byte[HEADNUMBER] ,0x00  ;��带 �ι� �о��� ��� ���� 0�� ��
	jne READDATA
	add byte[TRACKNUMBER], 0x01  ;Ʈ�� ���� 1 ���� ��Ŵ
	jmp READDATA

READEND:
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;OS �̹����� �Ϸ�Ǿ��ٴ� �޽��� ���
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	push LOADINGCOMPLETEMESSAGE
	push 1
	push 20
	call PRINTMESSAGE
	add sp,6

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;�ε��� ���� OS �̹��� ����
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	jmp 0x1000:0x0000


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;�Լ� �ڵ� ����
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
HANDLEDISKERROR:
	push DISKERRORMESSAGE
	push 1
	push 20
	call PRINTMESSAGE

	jmp $

;�޽��� ��� �Լ�
;PARAM: x��ǥ, y��ǥ, ���ڿ�
PRINTMESSAGE:
	push bp
	mov bp, sp
	push es    ;���� �������� ����, �Լ� ���� �� pop�����ν� �������Ͱ� ����
	push si
	push di
	push ax
	push cx
	push dx

	;es�� ���� ��� ��巹�� ����
	mov ax, 0xb800
	mov es,ax

	;���� ��巹�� ���
	mov ax, word[bp+6]  ;16bit�̹Ƿ� 2����Ʈ�� ���, ax�� y��ǥ ����
	mov si, 160
	mul si
	mov di,ax  ;���� column���� di�� ����

	mov ax, word[bp+4] ;x��ǥ�� ����
	mov si, 2
	mul si
	add di, ax

	mov si, word[bp+8]   ;���ڿ� �Ķ����

.MESSAGELOOP:
	mov cl, byte[si]   ;���ڿ� �ѱ��� ����
	cmp cl,0  ;���ڿ� ������ �˻�
	je .MESSAGEEND
	mov byte[es:di], cl   ;���� ���
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
;������ ����
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
