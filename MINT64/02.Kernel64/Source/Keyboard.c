#include "Types.h"
#include "AssemblyUtility.h"
#include "Keyboard.h"
#include "Queue.h"
#include "Utility.h"
#include "synchronization.h"


BOOL kIsOutputBufferFull() {
	//���� ���������� 1��° ��Ʈ�� �о� ��¹��ۿ� �����Ͱ� �����ϴ��� Ȯ��
	if(kInPortByte(0x64) & 0x01)
		return TRUE;
	return FALSE;
}

BOOL kIsInputBufferFull() {
	//���� ���������� 2��° ��Ʈ�� �о� input ���ۿ� �����Ͱ� �����ϴ��� Ȯ��
	if(kInPortByte(0x64) & 0x02)
		return TRUE;
	return FALSE;
}

//ACK�� ��ٸ���, ACK�� �ƴ� ��ĵ�ڵ�� ��ȯ�ؼ� ť�� ����
BOOL kWaitForACKAndPutOtherScanCode() {
	int i, j;
	BYTE bData;
	BOOL bResult = FALSE;
	for(j=0; j<100; j++) {
		for(i=0; i<0xffff; i++) {
			if(kIsOutputBufferFull()==TRUE)
				break;
		}
		//��� ���ۿ��� �����͸� �а�, ACK��� ����
		bData = kInPortByte(0x60);
		if(bData = 0xfa) {
			bResult = TRUE;
			break;
		}
		else
			//��ĵ�ڵ带 ��ȯ�� �Ŀ� ť�� ����
			kConvertScanCodeAndPutQueue(bData);
	}
	return bResult;
}




BOOL kActivateKeyboard() {
	int i;
	int j;
	BOOL bPreviousInterrupt;
	BOOL bResult;

	//���� ���ͷ�Ʈ ���¸� ������ �� ���ͷ�Ʈ ��Ȱ��ȭ
	bPreviousInterrupt = kSetInterruptFlag(FALSE);

	//Ű���� ��Ʈ�ѷ����� Ű���� ����̽� Ȱ��ȭ Ŀ�ǵ� ����
	kOutPortByte(0x64, 0xAE);

	for (i=0; i<0xffff; i++) {
		//�Է� ���۰� ��������� Ŀ�ǵ� ���� ����
		if(kIsInputBufferFull()==FALSE)
			break;
	}
	//�Է¹��ۿ� Ŀ�ǵ带 �Է��Ͽ� Ű���忡���� Ű���� ����̽� Ȱ��ȭ
	kOutPortByte(0x60, 0xf4);

	//Ȱ��ȭ�� �Ϸ�Ǹ� ack�� ���޵ǹǷ� ack�� �� ������ ��ٸ���
	bResult = kWaitForACKAndPutOtherScanCode();
	kSetInterruptFlag(bPreviousInterrupt);

	return bResult;
}



BYTE kGetKeyboardScanCode() {
	while(kIsOutputBufferFull()==FALSE);

	return kInPortByte(0x60);
}

BOOL kChangeKeyboardLED(BOOL bCapsLockOn, BOOL bNumLockOn,
		BOOL bScrollLockOn)
{
	int i, j;
	BOOL bPreviousInterrupt;
	BOOL bResult;
	BYTE bData;

	//���� ���ͷ�Ʈ ���� ���� �� ���ͷ�Ʈ ��Ȱ��ȭ
	bPreviousInterrupt = kSetInterruptFlag(FALSE);

	for(i=0; i<0xffff; i++) {
		//input���۸� Ȯ���ϴ� �Լ��� ����Ͽ�����
		//å���� ��� ���۰� ��������� Ȯ���Ѵٰ� �ּ���...
		//��¹��۸� Ȯ���ؾ�����, �Է¹��۸� Ȯ���ؾ����� �� �𸣰ڴ�
		//����� �Է¹��� ���⵵ �ϰ� ����� ��¹��� ���⵵�ϰ�...
		if(kIsInputBufferFull()==FALSE)
			break;

	}
	kOutPortByte(0x60, 0xed); //led ���º��� Ŀ�ǵ� ����
	for(i=0; i<0xffff; i++) {
		//input buffer�� ������� ��� Ű���尡 Ŀ�ǵ带 ������ ��
		if(kIsInputBufferFull()==FALSE)
			break;
	}

	bResult = kWaitForACKAndPutOtherScanCode();
	if(bResult==FALSE) {
		kSetInterruptFlag(bPreviousInterrupt);
		return FALSE;
	}

	//LED���� ���� ����
	kOutPortByte(0x60,(bCapsLockOn<<2)|(bNumLockOn<<1) | bScrollLockOn);
	for(i=0; i<0xffff; i++) {
		if(kIsInputBufferFull()==FALSE)
			break;
	}

	bResult = kWaitForACKAndPutOtherScanCode();
	kSetInterruptFlag(bPreviousInterrupt);
	return bResult;
}



void kEnableA20Gate() {
	BYTE bOutputPortData;
	int i;

	//��Ʈ�� �������Ϳ� Ű���� ��Ʈ�ѷ��� ��� ��Ʈ ���� �д� Ŀ�ǵ� ����
	kOutPortByte(0x64,0xd0);

	for(i=0; i<0xffff;i++) {
		if(kIsOutputBufferFull()==TRUE)
			break;

	}
	bOutputPortData = kInPortByte(0x60);
	//a20 ����Ʈ ��Ʈ ����
	bOutputPortData |= 0x01;
	for(i=0; i<0xffff; i++) {
		if(kIsInputBufferFull()==FALSE)
			break;
	}
	//Ŀ�ǵ� �������Ϳ� ��� ��Ʈ ���� Ŀ�ǵ带 ����
	kOutPortByte(0x64, 0xd1);
	//�Է¹��ۿ� a20 ����Ʈ ��Ʈ�� 1�� ������ �� ����
	kOutPortByte(0x60,bOutputPortData);

}

void kReboot() {
	int i;
	for(i=0; i<0xffff; i++) {
		if(kIsInputBufferFull()==FALSE)
			break;
	}
	//Ŀ�ǵ� �������Ϳ���� ��Ʈ ���� Ŀ�ǵ� ����
	kOutPortByte(0x64, 0xd1);
	//���μ����� ����
	kOutPortByte(0x60,0x00);
	while(1);
}

////////////////////////////////////////////////////////////////////////////////
//
// ��ĵ �ڵ带 ASCII �ڵ�� ��ȯ�ϴ� ��ɿ� ���õ� �Լ���
//
////////////////////////////////////////////////////////////////////////////////
// Ű���� ���¸� �����ϴ� Ű���� �Ŵ���
static KEYBOARDMANAGER gs_stKeyboardManager = { 0, };

// Ű�� �����ϴ� ť�� ���� ����
static QUEUE gs_stKeyQueue;
static KEYDATA gs_vstKeyQueueBuffer[KEY_MAXQUEUECOUNT];

// ��ĵ �ڵ带 ASCII �ڵ�� ��ȯ�ϴ� ���̺�
static KEYMAPPINGENTRY gs_vstKeyMappingTable[ KEY_MAPPINGTABLEMAXCOUNT ] =
{
    /*  0   */  {   KEY_NONE        ,   KEY_NONE        },
    /*  1   */  {   KEY_ESC         ,   KEY_ESC         },
    /*  2   */  {   '1'             ,   '!'             },
    /*  3   */  {   '2'             ,   '@'             },
    /*  4   */  {   '3'             ,   '#'             },
    /*  5   */  {   '4'             ,   '$'             },
    /*  6   */  {   '5'             ,   '%'             },
    /*  7   */  {   '6'             ,   '^'             },
    /*  8   */  {   '7'             ,   '&'             },
    /*  9   */  {   '8'             ,   '*'             },
    /*  10  */  {   '9'             ,   '('             },
    /*  11  */  {   '0'             ,   ')'             },
    /*  12  */  {   '-'             ,   '_'             },
    /*  13  */  {   '='             ,   '+'             },
    /*  14  */  {   KEY_BACKSPACE   ,   KEY_BACKSPACE   },
    /*  15  */  {   KEY_TAB         ,   KEY_TAB         },
    /*  16  */  {   'q'             ,   'Q'             },
    /*  17  */  {   'w'             ,   'W'             },
    /*  18  */  {   'e'             ,   'E'             },
    /*  19  */  {   'r'             ,   'R'             },
    /*  20  */  {   't'             ,   'T'             },
    /*  21  */  {   'y'             ,   'Y'             },
    /*  22  */  {   'u'             ,   'U'             },
    /*  23  */  {   'i'             ,   'I'             },
    /*  24  */  {   'o'             ,   'O'             },
    /*  25  */  {   'p'             ,   'P'             },
    /*  26  */  {   '['             ,   '{'             },
    /*  27  */  {   ']'             ,   '}'             },
    /*  28  */  {   '\n'            ,   '\n'            },
    /*  29  */  {   KEY_CTRL        ,   KEY_CTRL        },
    /*  30  */  {   'a'             ,   'A'             },
    /*  31  */  {   's'             ,   'S'             },
    /*  32  */  {   'd'             ,   'D'             },
    /*  33  */  {   'f'             ,   'F'             },
    /*  34  */  {   'g'             ,   'G'             },
    /*  35  */  {   'h'             ,   'H'             },
    /*  36  */  {   'j'             ,   'J'             },
    /*  37  */  {   'k'             ,   'K'             },
    /*  38  */  {   'l'             ,   'L'             },
    /*  39  */  {   ';'             ,   ':'             },
    /*  40  */  {   '\''            ,   '\"'            },
    /*  41  */  {   '`'             ,   '~'             },
    /*  42  */  {   KEY_LSHIFT      ,   KEY_LSHIFT      },
    /*  43  */  {   '\\'            ,   '|'             },
    /*  44  */  {   'z'             ,   'Z'             },
    /*  45  */  {   'x'             ,   'X'             },
    /*  46  */  {   'c'             ,   'C'             },
    /*  47  */  {   'v'             ,   'V'             },
    /*  48  */  {   'b'             ,   'B'             },
    /*  49  */  {   'n'             ,   'N'             },
    /*  50  */  {   'm'             ,   'M'             },
    /*  51  */  {   ','             ,   '<'             },
    /*  52  */  {   '.'             ,   '>'             },
    /*  53  */  {   '/'             ,   '?'             },
    /*  54  */  {   KEY_RSHIFT      ,   KEY_RSHIFT      },
    /*  55  */  {   '*'             ,   '*'             },
    /*  56  */  {   KEY_LALT        ,   KEY_LALT        },
    /*  57  */  {   ' '             ,   ' '             },
    /*  58  */  {   KEY_CAPSLOCK    ,   KEY_CAPSLOCK    },
    /*  59  */  {   KEY_F1          ,   KEY_F1          },
    /*  60  */  {   KEY_F2          ,   KEY_F2          },
    /*  61  */  {   KEY_F3          ,   KEY_F3          },
    /*  62  */  {   KEY_F4          ,   KEY_F4          },
    /*  63  */  {   KEY_F5          ,   KEY_F5          },
    /*  64  */  {   KEY_F6          ,   KEY_F6          },
    /*  65  */  {   KEY_F7          ,   KEY_F7          },
    /*  66  */  {   KEY_F8          ,   KEY_F8          },
    /*  67  */  {   KEY_F9          ,   KEY_F9          },
    /*  68  */  {   KEY_F10         ,   KEY_F10         },
    /*  69  */  {   KEY_NUMLOCK     ,   KEY_NUMLOCK     },
    /*  70  */  {   KEY_SCROLLLOCK  ,   KEY_SCROLLLOCK  },

    /*  71  */  {   KEY_HOME        ,   '7'             },
    /*  72  */  {   KEY_UP          ,   '8'             },
    /*  73  */  {   KEY_PAGEUP      ,   '9'             },
    /*  74  */  {   '-'             ,   '-'             },
    /*  75  */  {   KEY_LEFT        ,   '4'             },
    /*  76  */  {   KEY_CENTER      ,   '5'             },
    /*  77  */  {   KEY_RIGHT       ,   '6'             },
    /*  78  */  {   '+'             ,   '+'             },
    /*  79  */  {   KEY_END         ,   '1'             },
    /*  80  */  {   KEY_DOWN        ,   '2'             },
    /*  81  */  {   KEY_PAGEDOWN    ,   '3'             },
    /*  82  */  {   KEY_INS         ,   '0'             },
    /*  83  */  {   KEY_DEL         ,   '.'             },
    /*  84  */  {   KEY_NONE        ,   KEY_NONE        },
    /*  85  */  {   KEY_NONE        ,   KEY_NONE        },
    /*  86  */  {   KEY_NONE        ,   KEY_NONE        },
    /*  87  */  {   KEY_F11         ,   KEY_F11         },
    /*  88  */  {   KEY_F12         ,   KEY_F12         }
};


BOOL kIsAlphabetScanCode(BYTE bScanCode) {
	if(('a'<=gs_vstKeyMappingTable[bScanCode].bNormalCode) &&
			(gs_vstKeyMappingTable[bScanCode].bNormalCode<='z'))
		return TRUE;
	return FALSE;
}

BOOL kIsNumberOrSymbolScanCode(BYTE bScanCode) {
	if((2 <= bScanCode)&& (bScanCode<=53) &&
			(kIsAlphabetScanCode(bScanCode)==FALSE))
		return TRUE;
	return FALSE;
}

BOOL kIsNumberPadScanCode(BYTE bScanCode) {
	if((71<=bScanCode) && (bScanCode<=83))
		return TRUE;
	return FALSE;
}

BOOL kIsUseCombinedCode(BOOL bScanCode) {
	BYTE bDownScanCode;
	BOOL bUseCombinedKey;
	//8bit(0x80)���� ������ up, ������ down
	bDownScanCode = bScanCode & 0x7f;

	if(kIsAlphabetScanCode(bDownScanCode)==TRUE) {
		//���� �ϳ��� ���������� ��� ����Ű ����
		if(gs_stKeyboardManager.bShiftDown
				^ gs_stKeyboardManager.bCapsLockOn)
			bUseCombinedKey = TRUE;
		else
			bUseCombinedKey = FALSE;
	}

	else if(kIsNumberOrSymbolScanCode(bDownScanCode)==TRUE) {
		if(gs_stKeyboardManager.bShiftDown==TRUE)
			bUseCombinedKey = TRUE;
		else
			bUseCombinedKey = FALSE;
	}

	else if((kIsNumberPadScanCode(bDownScanCode)==TRUE)
			&& (gs_stKeyboardManager.bExtendedCodeIn==FALSE)) {
		if(gs_stKeyboardManager.bNumLockOn==TRUE)
			bUseCombinedKey = TRUE;
		else
			bUseCombinedKey = FALSE;
	}

	return bUseCombinedKey;
}

void UpdateCombinationKeyStatusAndLED(BYTE bScanCode) {
	BOOL bDown;
	BYTE bDownScanCode;
	BOOL bLEDStatusChanged = FALSE;

	//8��° ��Ʈ�� 1�̸� up, 0�̸� down
	if(bScanCode & 0x80) {
		bDown = FALSE;
		bDownScanCode = bScanCode & 0x7f;
	}
	else {
		bDown = TRUE;
		bDownScanCode = bScanCode;
	}
	//shift�� ��ĵ�ڵ��̸� shiftŰ�� �������ٰ� ����
	if((bDownScanCode==42) || (bDownScanCode==54))
		gs_stKeyboardManager.bShiftDown = bDown;
	//caps lock ���� �� LED����
	else if((bDownScanCode==58)&& (bDown==TRUE)) {
		gs_stKeyboardManager.bCapsLockOn ^=TRUE;
		bLEDStatusChanged = TRUE;
	}
	//Num Lock ���� �� LED ����
	else if ((bDownScanCode==69) && (bDown==TRUE)) {
		gs_stKeyboardManager.bNumLockOn ^= TRUE;
		bLEDStatusChanged = TRUE;
	}
	if(bLEDStatusChanged==TRUE) {
		kChangeKeyboardLED(gs_stKeyboardManager.bCapsLockOn,
				gs_stKeyboardManager.bNumLockOn,
				gs_stKeyboardManager.bScrollLockOn);
	}
}

BOOL kConvertScanCodeToASCIICode( BYTE bScanCode, BYTE* pbASCIICode, BOOL* pbFlags )
{
    BOOL bUseCombinedKey;

    // ������ Pause Ű�� ���ŵǾ��ٸ�, Pause�� ���� ��ĵ �ڵ带 ����
    if( gs_stKeyboardManager.iSkipCountForPause > 0 )
    {
        gs_stKeyboardManager.iSkipCountForPause--;
        return FALSE;
    }

    // Pause Ű�� Ư���� ó��
    if( bScanCode == 0xE1 )
    {
        *pbASCIICode = KEY_PAUSE;
        *pbFlags = KEY_FLAGS_DOWN;
        gs_stKeyboardManager.iSkipCountForPause = KEY_SKIPCOUNTFORPAUSE;
        return TRUE;
    }
    // Ȯ�� Ű �ڵ尡 ������ ��, ���� Ű ���� ������ �����Ƿ� �÷��� ������ �ϰ� ����
    else if( bScanCode == 0xE0 )
    {
        gs_stKeyboardManager.bExtendedCodeIn = TRUE;
        return FALSE;
    }
    // ���յ� Ű�� ��ȯ�ؾ� �ϴ°�?
    bUseCombinedKey = kIsUseCombinedCode( bScanCode );

    // Ű �� ����
    if( bUseCombinedKey == TRUE )
    {
        *pbASCIICode = gs_vstKeyMappingTable[ bScanCode & 0x7F ].bCombinedCode;
    }
    else
    {
        *pbASCIICode = gs_vstKeyMappingTable[ bScanCode & 0x7F ].bNormalCode;
    }

    // Ȯ�� Ű ���� ����
    if( gs_stKeyboardManager.bExtendedCodeIn == TRUE )
    {
        *pbFlags = KEY_FLAGS_EXTENDEDKEY;
        gs_stKeyboardManager.bExtendedCodeIn = FALSE;
    }
    else
    {
        *pbFlags = 0;
    }

    // ������ �Ǵ� ������ ���� ����
    if( ( bScanCode & 0x80 ) == 0 )
    {
        *pbFlags |= KEY_FLAGS_DOWN;
    }

    // ���� Ű ���� �Ǵ� ������ ���¸� ����
    UpdateCombinationKeyStatusAndLED( bScanCode );
    return TRUE;
}


BOOL kInitializeKeyboard() {
	kInitializeQueue(&gs_stKeyQueue, gs_vstKeyQueueBuffer, KEY_MAXQUEUECOUNT, sizeof(KEYDATA));
	return kActivateKeyboard();
}

BOOL kConvertScanCodeAndPutQueue( BYTE bScanCode )
{
    KEYDATA stData;
    BOOL bResult = FALSE;
    BOOL bPreviousInterrupt;
    char temp[2] = {0};

    stData.bScanCode = bScanCode;
    if( kConvertScanCodeToASCIICode( bScanCode, &( stData.bASCIICode ),
            &( stData.bFlags ) ) == TRUE )
    {
        bPreviousInterrupt = kLockForSystemData();

        bResult = kPutQueue( &gs_stKeyQueue, &stData );

        kUnlockForSystemData(bPreviousInterrupt);
    }
    return bResult;
}

BOOL kGetKeyFromKeyQueue(KEYDATA* pstData) {
	BOOL bResult;
	BOOL bPreviousInterrupt;
	int temp[2] = {0,0};
	if(kIsQueueEmpty(&gs_stKeyQueue)==TRUE) {
		return FALSE;
	}

	bPreviousInterrupt = kLockForSystemData();
	bResult = kGetQueue(&gs_stKeyQueue, pstData);
	kUnlockForSystemData(bPreviousInterrupt);
	return bResult;
}
