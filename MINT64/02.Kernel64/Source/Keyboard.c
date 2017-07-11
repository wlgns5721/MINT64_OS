#include "Types.h"
#include "AssemblyUtility.h"
#include "Keyboard.h"
#include "Queue.h"
#include "Utility.h"
#include "synchronization.h"


BOOL kIsOutputBufferFull() {
	//상태 레지스터의 1번째 비트를 읽어 출력버퍼에 데이터가 존재하는지 확인
	if(kInPortByte(0x64) & 0x01)
		return TRUE;
	return FALSE;
}

BOOL kIsInputBufferFull() {
	//상태 레지스터의 2번째 비트를 읽어 input 버퍼에 데이터가 존재하는지 확인
	if(kInPortByte(0x64) & 0x02)
		return TRUE;
	return FALSE;
}

//ACK를 기다리고, ACK가 아닌 스캔코드는 변환해서 큐에 저장
BOOL kWaitForACKAndPutOtherScanCode() {
	int i, j;
	BYTE bData;
	BOOL bResult = FALSE;
	for(j=0; j<100; j++) {
		for(i=0; i<0xffff; i++) {
			if(kIsOutputBufferFull()==TRUE)
				break;
		}
		//출력 버퍼에서 데이터를 읽고, ACK라면 성공
		bData = kInPortByte(0x60);
		if(bData = 0xfa) {
			bResult = TRUE;
			break;
		}
		else
			//스캔코드를 변환한 후에 큐에 저장
			kConvertScanCodeAndPutQueue(bData);
	}
	return bResult;
}




BOOL kActivateKeyboard() {
	int i;
	int j;
	BOOL bPreviousInterrupt;
	BOOL bResult;

	//이전 인터럽트 상태를 저장한 후 인터럽트 비활성화
	bPreviousInterrupt = kSetInterruptFlag(FALSE);

	//키보드 컨트롤러에서 키보드 디바이스 활성화 커맨드 전달
	kOutPortByte(0x64, 0xAE);

	for (i=0; i<0xffff; i++) {
		//입력 버퍼가 비어있으면 커맨드 전송 가능
		if(kIsInputBufferFull()==FALSE)
			break;
	}
	//입력버퍼에 커맨드를 입력하여 키보드에서도 키보드 디바이스 활성화
	kOutPortByte(0x60, 0xf4);

	//활성화가 완료되면 ack가 전달되므로 ack가 올 때까지 기다린다
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

	//이전 인터럽트 상태 저장 후 인터럽트 비활성화
	bPreviousInterrupt = kSetInterruptFlag(FALSE);

	for(i=0; i<0xffff; i++) {
		//input버퍼를 확인하는 함수를 사용하였지만
		//책에는 출력 버퍼가 비었는지를 확인한다고 주석이...
		//출력버퍼를 확인해야할지, 입력버퍼를 확인해야할지 잘 모르겠다
		//어찌보면 입력버퍼 같기도 하고 어찌보면 출력버퍼 같기도하고...
		if(kIsInputBufferFull()==FALSE)
			break;

	}
	kOutPortByte(0x60, 0xed); //led 상태변경 커맨드 전송
	for(i=0; i<0xffff; i++) {
		//input buffer가 비어있을 경우 키보드가 커맨드를 가져간 것
		if(kIsInputBufferFull()==FALSE)
			break;
	}

	bResult = kWaitForACKAndPutOtherScanCode();
	if(bResult==FALSE) {
		kSetInterruptFlag(bPreviousInterrupt);
		return FALSE;
	}

	//LED변경 값을 전송
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

	//컨트롤 레지스터에 키보드 컨트롤러의 출력 포트 값을 읽는 커맨드 전송
	kOutPortByte(0x64,0xd0);

	for(i=0; i<0xffff;i++) {
		if(kIsOutputBufferFull()==TRUE)
			break;

	}
	bOutputPortData = kInPortByte(0x60);
	//a20 게이트 비트 설정
	bOutputPortData |= 0x01;
	for(i=0; i<0xffff; i++) {
		if(kIsInputBufferFull()==FALSE)
			break;
	}
	//커맨드 레지스터에 출력 포트 설정 커맨드를 전달
	kOutPortByte(0x64, 0xd1);
	//입력버퍼에 a20 게이트 비트가 1로 설정된 값 전달
	kOutPortByte(0x60,bOutputPortData);

}

void kReboot() {
	int i;
	for(i=0; i<0xffff; i++) {
		if(kIsInputBufferFull()==FALSE)
			break;
	}
	//커맨드 레지스터에출력 포트 설정 커맨드 전달
	kOutPortByte(0x64, 0xd1);
	//프로세서를 리셋
	kOutPortByte(0x60,0x00);
	while(1);
}

////////////////////////////////////////////////////////////////////////////////
//
// 스캔 코드를 ASCII 코드로 변환하는 기능에 관련된 함수들
//
////////////////////////////////////////////////////////////////////////////////
// 키보드 상태를 관리하는 키보드 매니저
static KEYBOARDMANAGER gs_stKeyboardManager = { 0, };

// 키를 저장하는 큐와 버퍼 정의
static QUEUE gs_stKeyQueue;
static KEYDATA gs_vstKeyQueueBuffer[KEY_MAXQUEUECOUNT];

// 스캔 코드를 ASCII 코드로 변환하는 테이블
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
	//8bit(0x80)값이 있으면 up, 없으면 down
	bDownScanCode = bScanCode & 0x7f;

	if(kIsAlphabetScanCode(bDownScanCode)==TRUE) {
		//둘중 하나만 눌려져있을 경우 조합키 리턴
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

	//8번째 비트가 1이면 up, 0이면 down
	if(bScanCode & 0x80) {
		bDown = FALSE;
		bDownScanCode = bScanCode & 0x7f;
	}
	else {
		bDown = TRUE;
		bDownScanCode = bScanCode;
	}
	//shift의 스캔코드이면 shift키가 눌려졌다고 갱신
	if((bDownScanCode==42) || (bDownScanCode==54))
		gs_stKeyboardManager.bShiftDown = bDown;
	//caps lock 상태 및 LED갱신
	else if((bDownScanCode==58)&& (bDown==TRUE)) {
		gs_stKeyboardManager.bCapsLockOn ^=TRUE;
		bLEDStatusChanged = TRUE;
	}
	//Num Lock 상태 및 LED 갱신
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

    // 이전에 Pause 키가 수신되었다면, Pause의 남은 스캔 코드를 무시
    if( gs_stKeyboardManager.iSkipCountForPause > 0 )
    {
        gs_stKeyboardManager.iSkipCountForPause--;
        return FALSE;
    }

    // Pause 키는 특별히 처리
    if( bScanCode == 0xE1 )
    {
        *pbASCIICode = KEY_PAUSE;
        *pbFlags = KEY_FLAGS_DOWN;
        gs_stKeyboardManager.iSkipCountForPause = KEY_SKIPCOUNTFORPAUSE;
        return TRUE;
    }
    // 확장 키 코드가 들어왔을 때, 실제 키 값은 다음에 들어오므로 플래그 설정만 하고 종료
    else if( bScanCode == 0xE0 )
    {
        gs_stKeyboardManager.bExtendedCodeIn = TRUE;
        return FALSE;
    }
    // 조합된 키를 반환해야 하는가?
    bUseCombinedKey = kIsUseCombinedCode( bScanCode );

    // 키 값 설정
    if( bUseCombinedKey == TRUE )
    {
        *pbASCIICode = gs_vstKeyMappingTable[ bScanCode & 0x7F ].bCombinedCode;
    }
    else
    {
        *pbASCIICode = gs_vstKeyMappingTable[ bScanCode & 0x7F ].bNormalCode;
    }

    // 확장 키 유무 설정
    if( gs_stKeyboardManager.bExtendedCodeIn == TRUE )
    {
        *pbFlags = KEY_FLAGS_EXTENDEDKEY;
        gs_stKeyboardManager.bExtendedCodeIn = FALSE;
    }
    else
    {
        *pbFlags = 0;
    }

    // 눌러짐 또는 떨어짐 유무 설정
    if( ( bScanCode & 0x80 ) == 0 )
    {
        *pbFlags |= KEY_FLAGS_DOWN;
    }

    // 조합 키 눌림 또는 떨어짐 상태를 갱신
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
