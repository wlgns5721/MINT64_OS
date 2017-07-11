#include <stdarg.h>
#include "Console.h"
#include "Keyboard.h"

CONSOLEMANAGER gs_stConsoleManager = {0,};

void kInitializeConsole(int iX, int iY) {
	kMemSet(&gs_stConsoleManager, 0, sizeof(gs_stConsoleManager));
	kSetCursor(iX,iY);
}

void kSetCursor(int iX, int iY) {
	int iLinearValue;
	//커서 위치 계산
	iLinearValue = iY * CONSOLE_WIDTH + iX;
	//상위 커서 위치 레지스터 선택
	kOutPortByte(VGA_PORT_INDEX,VGA_INDEX_UPPERCURSOR);
	//CRTC 컨트롤 데이터 레지스터에 커서의 상위 바이트 출력
	kOutPortByte(VGA_PORT_DATA, iLinearValue>>8);
	//하위 커서 위치 레지스터 선택
	kOutPortByte(VGA_PORT_INDEX, VGA_INDEX_LOWERCURSOR);
	//하위 바이트 출력
	kOutPortByte(VGA_PORT_DATA, iLinearValue & 0xff);

	gs_stConsoleManager.iCurrentPrintOffset = iLinearValue;

}

void kGetCursor(int *piX, int *piY) {
	*piX = gs_stConsoleManager.iCurrentPrintOffset % CONSOLE_WIDTH;
	*piY = gs_stConsoleManager.iCurrentPrintOffset / CONSOLE_WIDTH;
}

void kPrintf(const char* pcFormatString, ...) {
	va_list ap;
	char vcBuffer[1024];
	int iNextPrintOffset;
	//가변 인자 리스트를 사용해서 vsprintf로 처리

	//가변인자 사용 시작, 인자로는 가변인자 바로 앞의 파라미터 이름을 받음(어드레스 계산을 위함)
	va_start(ap, pcFormatString);
	kVSPrintf(vcBuffer, pcFormatString, ap);
	va_end(ap);

	//화면에 출력
	iNextPrintOffset = kConsolePrintString(vcBuffer);

	//커서의 위치 업데이트
	kSetCursor(iNextPrintOffset % CONSOLE_WIDTH,
			iNextPrintOffset/CONSOLE_WIDTH);
}

int kConsolePrintString(const char* pcBuffer) {
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	int i,j;
	int iLength;
	int iPrintOffset;
	iPrintOffset = gs_stConsoleManager.iCurrentPrintOffset;

	iLength = kStrLen(pcBuffer);
	for(i=0; i<iLength; i++) {
		//엔터 눌렀을 때
		if(pcBuffer[i] =='\n') {
			//현재 라인에서 남은 문자열을 더해 다음 줄로 넘어감
			iPrintOffset+=(CONSOLE_WIDTH -
					(iPrintOffset % CONSOLE_WIDTH));
		}
		//탭을 눌렀을 때
		else if(pcBuffer[i]=='\t') {
			iPrintOffset+=(8-(iPrintOffset%8));
		}
		//이외의 동작
		else {
			pstScreen[iPrintOffset].bCharactor = pcBuffer[i];
			pstScreen[iPrintOffset].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
			iPrintOffset++;
		}

		//스크롤 처리
		if(iPrintOffset >= CONSOLE_WIDTH * CONSOLE_HEIGHT) {
			kMemCpy(CONSOLE_VIDEOMEMORYADDRESS,
					CONSOLE_VIDEOMEMORYADDRESS+CONSOLE_WIDTH*sizeof(CHARACTER),
					(CONSOLE_HEIGHT-1)*CONSOLE_WIDTH*sizeof(CHARACTER));
			for (j=(CONSOLE_HEIGHT-1)*CONSOLE_WIDTH;
					j<(CONSOLE_HEIGHT * CONSOLE_WIDTH); j++) {
				pstScreen[j].bCharactor = ' ';
				pstScreen[j].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
			}
			iPrintOffset = (CONSOLE_HEIGHT - 1)*CONSOLE_WIDTH;

		}
	}
	return iPrintOffset;
}


void kClearScreen() {
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	int i;

	for(i=0; i<CONSOLE_WIDTH*CONSOLE_HEIGHT; i++) {
		pstScreen[i].bCharactor = ' ';
		pstScreen[i].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
	}
	kSetCursor(0,0);
}

BYTE kGetCh() {
	KEYDATA stData;
	while(1) {
		while(kGetKeyFromKeyQueue(&stData)==FALSE);

		if(stData.bFlags & KEY_FLAGS_DOWN) {
			return stData.bASCIICode;
		}
	}
}

void kPrintStringXY(int iX, int iY, const char* pcString) {
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	int i;
	pstScreen+=(iY*80) + iX;
	for(i=0; pcString[i]!=0; i++) {
		pstScreen[i].bCharactor = pcString[i];
		pstScreen[i].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
	}
}



