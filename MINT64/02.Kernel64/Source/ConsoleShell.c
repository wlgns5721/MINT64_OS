#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"
#include "PIT.h"
#include "RTC.h"
#include "AssemblyUtility.h"
#include "Synchronization.h"
#include "DynamicMemory.h"
#include "HardDisk.h"
#include "FileSystem.h"

SHELLCOMMANDENTRY gs_vstCommandTable[]= {
		{ "help", "Show Help", kHelp },
		{ "cls", "Clear Screen", kCls },
		{ "totalram", "Show Total RAM Size", kShowTotalRAMSize },
		{ "shutdown", "Shutdown And Reboot OS", kShutdown },
		{"wait","Wait ms Using PIT ex)wait 100(ms)", kWaitUsingPIT},
		{"settimer", "Set PIT Controller Counter0, ex)settimer 10(ms) 1(periodic)",kSetTimer},
		{"rdtsc","Read Time Stamp Counter",kReadTimeStampCounter},
		{"cpuspeed", "Measure processor speed,", kMeasureProcessorSpeed},
		{"date", "Show Date And Time",kShowDateAndTime},
		{"createtask","Create Task", kCreateTestTask},
		{"changepriority", "Change Task Priority, ex) changepriority 1(ID) 2(priority)",kChangeTaskPriority},
		{"tasklist","Show Task list",kShowTaskList},
		{"killtask","End Task, ex)killtask 1(ID)",kKillTask},
		{"cpuload","Show Processor Load", kCPULoad},
		{"testmutex","Test Mutex Function",kTestMutex},
		{"testthread", "Test Thread And Process Function", kTestThread},
		{"showmatrix","show matrix screen", kShowMatrix},
		{"testpie", "TestPIE Calculation", kTestPIE},
		{"dynamicmeminfo","show dynamic memory info",kShowDynamicMemoryInformation},
		{"testseqalloc","test sequential alloc & free",kTestSequentialAllocation},
		{"testranalloc","test random alloc & free",kTestRandomAllocation},
		{"hddinfo","show hdd information", kShowHDDInformation},
		{"readsector", "read hdd sector ex) readsector 0(LBA) 10(count)",kReadSector},
		{"writesector", "write hdd sector, ex) writesector 0 10", kWriteSector},
		{"testcode", "Test Code", TestCode},
		{"mounthdd", "Mount HDD", kMountHDD},
		{"formathdd", "Format HDD", kFormatHDD},
		{"filesysteminfo", "Show File System Information", kShowFileSystemInformation},
		{"createfile", "Create File, ex) createfile a.txt", kCreateFileInRootDirectory},
		{"dir", "Show Directory", kShowRootDirectory},
		{"deletefile", "Delete File, ex)deletefile a.txt", kDeleteFileInRootDirectory},
		{ "writefile", "Write Data To File, ex) writefile a.txt", kWriteDataToFile },
		{ "readfile", "Read Data From File, ex) readfile a.txt", kReadDataFromFile },
		{"testfileio", "Test File I/O Function", kTestFileIO}
};

static TCB gs_vstTask[2] = {0,};
static QWORD gs_vstStack[1024] = {0,};
static MUTEX gs_stMutex;
static volatile QWORD gs_qwAdder;
static volatile QWORD gs_qwRandomValue = 0;

QWORD kRandom() {
	gs_qwRandomValue = (gs_qwRandomValue * 412153 + 5571031)>>16;
	return gs_qwRandomValue;
}

void kStartConsoleShell() {
	char vcCommandBuffer[CONSOLESHELL_MAXCOMMANDBUFFERCOUNT];
	int iCommandBufferIndex = 0;
	BYTE bKey;
	int iCursorX, iCursorY;

	kPrintf(CONSOLESHELL_PROMPTMESSAGE);
	while(1) {
		bKey = kGetCh();
		if(bKey==KEY_BACKSPACE) {
			if(iCommandBufferIndex > 0) {
				kGetCursor(&iCursorX,&iCursorY); //현재 커서의 위치를 얻음
				kPrintStringXY(iCursorX-1, iCursorY, " ");//현재 커서 위치에 string을 write
				kSetCursor(iCursorX-1, iCursorY); //커서를 설정
				iCommandBufferIndex--;
			}
		}
		else if (bKey ==KEY_ENTER) {
			kPrintf("\n");

			if(iCommandBufferIndex>0) {
				vcCommandBuffer[iCommandBufferIndex] = '\0';
				kExecuteCommand(vcCommandBuffer);
			}

			kPrintf("%s", CONSOLESHELL_PROMPTMESSAGE);
			kMemSet(vcCommandBuffer, '\0', CONSOLESHELL_MAXCOMMANDBUFFERCOUNT);
			iCommandBufferIndex = 0;
		}
		else if (bKey==KEY_LSHIFT || bKey==KEY_RSHIFT ||
				bKey==KEY_CAPSLOCK || bKey==KEY_NUMLOCK || bKey==KEY_SCROLLLOCK);
		else {
			if(bKey==KEY_TAB)
				bKey=' ';
			if(iCommandBufferIndex < CONSOLESHELL_MAXCOMMANDBUFFERCOUNT) {
				vcCommandBuffer[iCommandBufferIndex++] = bKey;
				kPrintf("%c", bKey);

			}
		}
	}
}

void kExecuteCommand(const char* pcCommandBuffer) {
	int i, iSpaceIndex;
	int iCommandBufferLength, iCommandLength;
	int iCount;

	iCommandBufferLength = kStrLen(pcCommandBuffer);
	//占쏙옙占쏙옙占쏙옙 占쏙옙占시띰옙占쏙옙占쏙옙 커占실듸옙 占쏙옙占쏙옙
	for(iSpaceIndex = 0; iSpaceIndex<iCommandBufferLength; iSpaceIndex++) {
		if(pcCommandBuffer[iSpaceIndex]==' ')
			break;
	}

	iCount = sizeof(gs_vstCommandTable) / sizeof(SHELLCOMMANDENTRY);
	for(i=0; i<iCount; i++) {
		iCommandLength = kStrLen(gs_vstCommandTable[i].pcCommand);
		//占쏙옙치占싹댐옙 커占실듸옙 占싯삼옙
		if((iCommandLength==iSpaceIndex) &&
				(kMemCmp(gs_vstCommandTable[i].pcCommand,pcCommandBuffer,
						iSpaceIndex)==0)) {
			//占쏙옙치占싹댐옙 커占실드를 찾占싣쇽옙 占쏙옙占쏙옙
			gs_vstCommandTable[i].pfFunction(pcCommandBuffer+iSpaceIndex+1);
			break;
		}
	}
	if(i>=iCount)
		kPrintf("'%s' is not found\n",pcCommandBuffer);

}

void kInitializeParameter( PARAMETERLIST* pstList, const char* pcParameter )
{
    pstList->pcBuffer = pcParameter;
    pstList->iLength = kStrLen( pcParameter );
    pstList->iCurrentPosition = 0;
}

//  占쏙옙占쏙옙占쏙옙占쏙옙 占쏙옙占싻듸옙 占식띰옙占쏙옙占쏙옙占� 占쏙옙占쏙옙占� 占쏙옙占싱몌옙 占쏙옙환
int kGetNextParameter( PARAMETERLIST* pstList, char* pcParameter )
{
    int i;
    int iLength;

    // 占쏙옙 占싱삼옙 占식띰옙占쏙옙叩占� 占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙
    if( pstList->iLength <= pstList->iCurrentPosition )
    {
        return 0;
    }

    // 占쏙옙占쏙옙占쏙옙 占쏙옙占싱몌옙큼 占싱듸옙占싹면서 占쏙옙占쏙옙占쏙옙 占싯삼옙
    for( i = pstList->iCurrentPosition ; i < pstList->iLength ; i++ )
    {
        if( pstList->pcBuffer[ i ] == ' ' )
        {
            break;
        }
    }

    // 占식띰옙占쏙옙拷占� 占쏙옙占쏙옙占싹곤옙 占쏙옙占싱몌옙 占쏙옙환
    kMemCpy( pcParameter, pstList->pcBuffer + pstList->iCurrentPosition, i );
    iLength = i - pstList->iCurrentPosition;
    pcParameter[ iLength ] = '\0';

    // 占식띰옙占쏙옙占쏙옙占� 占쏙옙치 占쏙옙占쏙옙占쏙옙트
    pstList->iCurrentPosition += iLength + 1;
    return iLength;
}

void kHelp( const char* pcCommandBuffer )
{
    int i;
    int iCount;
    int iCursorX, iCursorY;
    int iLength, iMaxCommandLength = 0;


    kPrintf( "=========================================================\n" );
    kPrintf( "                    MINT64 Shell Help                    \n" );
    kPrintf( "=========================================================\n" );

    iCount = sizeof( gs_vstCommandTable ) / sizeof( SHELLCOMMANDENTRY );

    // 가장 긴 커맨드를 계산
    for( i = 0 ; i < iCount ; i++ )
    {
        iLength = kStrLen( gs_vstCommandTable[ i ].pcCommand );
        if( iLength > iMaxCommandLength )
        {
            iMaxCommandLength = iLength;
        }
    }

    // 도움말 출력
    for( i = 0 ; i < iCount ; i++ )
    {
        kPrintf( "%s", gs_vstCommandTable[ i ].pcCommand );
        kGetCursor( &iCursorX, &iCursorY );
        kSetCursor( iMaxCommandLength, iCursorY );
        kPrintf( "  - %s\n", gs_vstCommandTable[ i ].pcHelp );

		//목록이 많을 경우 나눠서 보여줌
		if((i!=0)&&((i%20)==0)) {
			kPrintf("press any key to continue....('q' ls exit) : ");
			if(kGetCh()=='q') {
				kPrintf("\n");
				break;
			}
			kPrintf("\n");
		}
    }
}


void kCls( const char* pcParameterBuffer )
{
    kClearScreen();
    //占쏙옙占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙占� 占쎈도占쏙옙 占쏙옙占�
    kSetCursor( 0, 1 );
}


void kShowTotalRAMSize( const char* pcParameterBuffer )
{
    kPrintf( "Total RAM Size = %d MB\n", kGetTotalRAMSize() );
}

void kStringToDecimalHexTest(const char* pcParameterBuffer) {
	char vcParameter[100];
	int iLength;
	PARAMETERLIST stList;
	int iCount = 0;
	long lValue;

	kInitializeParameter(&stList,pcParameterBuffer);

	while(1) {
		iLength = kGetNextParameter(&stList, vcParameter);
		if(iLength==0)
			break;

		kPrintf("Param %d = '%s', Length=%d, ",
				iCount+1,vcParameter,iLength);
		if(kMemCmp(vcParameter, "0x",2)) {
			lValue = kAToI(vcParameter + 2,16);
			kPrintf("HEX Value = %q\n",lValue);
		}
		else {
			lValue = kAToI(vcParameter,10);
			kPrintf("Decimal Value = %d\n",lValue);
		}
		iCount++;
	}
}

void kShutdown(const char* pcParameterBuffer) {
	kPrintf("System Shutdown Start......");
	kPrintf("Press any key to reboot....");
	kGetCh();
	kReboot();
}

void kSetTimer(const char* pcParameterBuffer) {
	char vcParameter[100];
	PARAMETERLIST stList;
	long lValue;
	BOOL bPeriodic;

	kInitializeParameter(&stList, pcParameterBuffer);
	if(kGetNextParameter(&stList, vcParameter)==0) {
		kPrintf("ex)settimer 10(ms) 1(periodic)\n");
		return;
	}
	lValue = kAToI(vcParameter, 10);

	if(kGetNextParameter(&stList, vcParameter)==0) {
		kPrintf("ex)settimer 10(ms) 1(periodic)\n");
	}
	bPeriodic = kAToI(vcParameter, 10);
	kInitializePIT(MSTOCOUNT(lValue), bPeriodic);
	kPrintf("Time = %d ms, Periodic = %d Change Complete\n",lValue, bPeriodic);
}

void kWaitUsingPIT(const char* pcParameterBuffer) {
	char vcParameter[100];
	int iLength;
	PARAMETERLIST stList;
	long lMillisecond;
	int i;

	kInitializeParameter(&stList, pcParameterBuffer);
	if(kGetNextParameter(&stList, vcParameter)==0) {
		kPrintf("ex)wait 100(ms)\n");
		return;
	}
	lMillisecond = kAToI(vcParameter,10);
	kPrintf("%d ms Sleep Start...\n",lMillisecond);
	kDisableInterrupt();
	//占쌍댐옙 50ms占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙占쏙옙占싼듸옙, 占쏙옙占쏙옙占쏙옙占쏙옙占쏙옙 30ms占쏙옙占쏙옙占쏙옙 占쏙옙占쏘서 wait占쌉쇽옙占쏙옙 占쏙옙占쏙옙
	for(i=0; i<lMillisecond/30; i++) {
		kWaitUsingDirectPIT(MSTOCOUNT(30));
	}
	kWaitUsingDirectPIT(MSTOCOUNT(lMillisecond%30));

	kEnableInterrupt();
	kPrintf("Sleep Complete\n");
	//타占싱몌옙 占쏙옙占쏙옙
	kInitializePIT(MSTOCOUNT(1),TRUE);
}

void kReadTimeStampCounter(const char* pcParameterBuffer) {
	QWORD qwTSC;
	qwTSC = kReadTSC();

	kPrintf("Time Stamp Counter = %q\n",qwTSC);
}

void kMeasureProcessorSpeed(const char* pcParameterBuffer) {
	int i;
	QWORD qwLastTSC, qwTotalTSC = 0;
	kPrintf("Now Measuring.");
	kDisableInterrupt();
	for(i=0; i<200; i++) {
		qwLastTSC = kReadTSC();
		kWaitUsingDirectPIT(MSTOCOUNT(50));
		qwTotalTSC +=kReadTSC() - qwLastTSC;
		kPrintf(".");
	}
	//타占싱몌옙 占쏙옙占쏙옙
	kInitializePIT(MSTOCOUNT(1),TRUE);
	kEnableInterrupt();
	kPrintf("\nCPU Speed = %d MHz\n",qwTotalTSC/10/1000/1000);
}

void kShowDateAndTime(const char* pcParameterBuffer) {
	BYTE bSecond, bMinute, bHour;
	BYTE bDayOfWeek, bDayOfMonth, bMonth;
	WORD wYear;

	kReadRTCTime(&bHour, &bMinute, &bSecond);
	kReadRTCDate(&wYear, &bMonth, &bDayOfMonth, &bDayOfWeek);

	kPrintf("Date : %d/%d/%d/ %s",wYear,bMonth,bDayOfMonth,
			kConvertDayOfWeekToString(bDayOfWeek));
	kPrintf("time : %d:%d:%d\n", bHour, bMinute, bSecond);
}

void kTestTask1() {
	BYTE bData;
	int i=0, iX=0, iY=0, iMargin,j;
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	TCB* pstRunningTask;

	pstRunningTask = kGetRunningTask();
	iMargin = (pstRunningTask->stLink.qwID & 0xffffffff) % 10;

	for(j=0; j<20000; j++) {
		switch(i) {
		case 0:
			iX++;
			if(iX>=(CONSOLE_WIDTH - iMargin))
				i=1;
			break;
		case 1:
			iY++;
			if(iY>=(CONSOLE_HEIGHT - iMargin))
				i=2;
			break;
		case 2:
			iX--;
			if(iX<iMargin)
				i=3;
			break;
		case 3:
			iY--;
			if(iY<iMargin)
				i=0;
			break;
		}
		pstScreen[iY*CONSOLE_WIDTH + iX].bCharactor = bData;
		pstScreen[iY*CONSOLE_WIDTH + iX].bAttribute = bData & 0x0f;
		bData++;
		//kSchedule();
	}
	kExitTask();
}

void kTestTask2() {
	int i=0, iOffset;
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	TCB* pstRunningTask;
	char vcData[4] = {'-','\\','|','/'};
	pstRunningTask = kGetRunningTask();
	iOffset = (pstRunningTask->stLink.qwID & 0xffffffff) * 2;
	iOffset = CONSOLE_WIDTH * CONSOLE_HEIGHT - (iOffset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));
	while(1) {
		pstScreen[iOffset].bCharactor = vcData[i%4];
		pstScreen[iOffset].bAttribute = (iOffset % 15) + 1;
		i++;
		kSchedule();
	}
}

void kCreateTestTask(const char* pcParameterBuffer) {
	PARAMETERLIST stList;
	char vcType[30];
	char vcCount[30];
	int i;

	kInitializeParameter(&stList, pcParameterBuffer);
	kGetNextParameter(&stList, vcType);
	kGetNextParameter(&stList, vcCount);

	switch(kAToI(vcType,10)) {
	case 1:
		for(i=0; i<kAToI(vcCount, 10); i++) {
			if(kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD,0,0,(QWORD)kTestTask1)==NULL)
				break;
		}
		kPrintf("Task1 %d Created\n",i);
		break;


	case 2:
		for(i=0; i<kAToI(vcCount,10);i++) {
			if(kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD,0,0,(QWORD)kTestTask2)==NULL)
				break;
		}
		kPrintf("Task2 %d Created\n",i);
		break;
	case 3:
		for( i = 0 ; i < kAToI(vcCount,10) ; i++ )
		    {
		        if( kCreateTask( TASK_FLAGS_PROCESS | TASK_FLAGS_HIGHEST, 0, 0,
		            ( QWORD ) kDropCharactorThread) == NULL )
		        {
		            break;
		        }

		        kSleep( kRandom() % 5 + 5 );
		    }
		break;
	}


	//占쏙옙占쏙옙 타占싱몌옙 占쏙옙占싶뤄옙트占쏙옙 占쏙옙활占쏙옙화占쏙옙 占쏙옙占�, kTestTask占쏙옙 占쏙옙占쏙옙占쏙옙占쏙옙占쏙옙 占쏙옙쨉占쏙옙占� 占쏙옙占쏙옙 占쏙옙占싱댐옙
	//占쏙옙占쏙옙占쏙옙 kTestTask 占쏙옙占싸울옙占쏙옙占쏙옙 kSchedule()占쌉쇽옙占쏙옙 占쌍어서 占승쏙옙크占쏙옙 占쏙옙占쏙옙칭占쏙옙 占쏙옙占쏙옙占쏙옙,
	//占쏙옙占쏙옙 占승쏙옙크占쏙옙 占쏙옙占쏙옙占쏙옙 占쏙옙 占승쏙옙크占쏙옙 占쏙옙占쏙옙칭 占쏙옙占쌍댐옙 占쌘드가 占쏙옙占쏙옙占실뤄옙 kTestTask占쏙옙 占쏙옙占쏙옙漫占� 占쏙옙占쏙옙 占쏙옙占쏙옙트占쏙옙占쏙옙 占쏙옙占쏙옙占싹곤옙
	//占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙 占싫댐옙. 占쏙옙占쏙옙 占쏙옙占싶뤄옙트 활占쏙옙화 占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙占싹는곤옙占쏙옙 占쏙옙占쏙옙 占싶다몌옙 占싣뤄옙占쏙옙 占쏙옙占쏙옙 占싹몌옙 占싫댐옙
	//while(1) kSchedule();
	//占싱뤄옙占쏙옙占싹몌옙 占쏙옙占쏙옙占쏙옙占쏙옙占쏙옙 占승쏙옙크 占쏙옙占쏙옙칭占쏙옙 占쏙옙占쌍므뤄옙 占쏙옙占싹는댐옙占� 占쏙옙占쏙옙占싼댐옙.
}

static void kChangeTaskPriority(const char* pcParameterBuffer) {
	PARAMETERLIST stList;
	char vcID[30];
	char vcPriority[30];
	QWORD qwID;
	BYTE bPriority;

	kInitializeParameter(&stList, pcParameterBuffer);
	kGetNextParameter(&stList, vcID);
	kGetNextParameter(&stList, vcPriority);
	if(kMemCmp(vcID,"0x",2)==0) {
		qwID = kAToI(vcID+2,16);
	}
	else {
		qwID = kAToI(vcID, 10);
	}
	bPriority = kAToI(vcPriority,10);
	kPrintf("Change Task Priority ID [0x%q] Priority[%d] ",qwID, bPriority);
	if(kChangePriority(qwID, bPriority)==TRUE)
		kPrintf("Success\n");
	else
		kPrintf("Fail\n");
}

static void kShowTaskList(const char* pcParameterBuffer) {
	int i;
	TCB* pstTCB;
	int iCount = 0;
	kPrintf("===============Task Total Count [%d] ===============\n",kGetTaskCount());
	for(i=0; i<TASK_MAXCOUNT; i++) {
		pstTCB = kGetTCBInTCBPool(i);
		if((pstTCB->stLink.qwID>>32)!=0) {
			if((iCount!=0)&&((iCount%10)==0)) {
				//占쏙옙占쏙옙 task占쏙옙 占쏙옙占쏙옙 占쏙옙占� 10占쏙옙 占쏙옙占쏙옙占쏙옙 占심곤옙占쏙옙 占쏙옙占쏙옙磯占�.
				kPrintf("Press any key to continue......('q' is exit) : ");
				if(kGetCh()=='q') {
					kPrintf("\n");
					break;
				}
				kPrintf("\n");
			}
			kPrintf( "[%d] Task ID[0x%Q], Priority[%d], Flags[0x%Q], Thread[%d]\n", 1 + iCount++,
			                     pstTCB->stLink.qwID, GETPRIORITY( pstTCB->qwFlags ),
			                     pstTCB->qwFlags, kGetListCount( &( pstTCB->stChildThreadList ) ) );
			kPrintf( "Parent PID[0x%Q], Memory Address[0x%Q], Size[0x%Q]\n",
			                    pstTCB->qwParentProcessID, pstTCB->pvMemoryAddress, pstTCB->qwMemorySize );
		}
	}
}

static void kKillTask(const char* pcParameterBuffer) {
	PARAMETERLIST stList;
	char vcID[30];
	QWORD qwID;
	TCB* pstTCB;
	int i;

	kInitializeParameter(&stList, pcParameterBuffer);
	kGetNextParameter(&stList, vcID);

	if(kMemCmp(vcID,"0x",2)==0)
		qwID = kAToI(vcID+2,16);
	else
		qwID = kAToI(vcID,10);

	if(qwID!=0xffffffff) {
		pstTCB = kGetTCBInTCBPool(GETTCBOFFSET(qwID));
		qwID = pstTCB->stLink.qwID;

		if(((qwID>>32)!=0)&&((pstTCB->qwFlags & TASK_FLAGS_SYSTEM)==0x00)) {
			kPrintf("kill Task ID [0x%q] " ,qwID);
			if(kEndTask(qwID)==TRUE)
				kPrintf("Success\n");
			else
				kPrintf("Fail\n");
		}
		else {
			kPrintf("Task does not exist or task is system task\n");
		}
	}

	else {
		for(i=0; i<TASK_MAXCOUNT; i++) {
			pstTCB = kGetTCBInTCBPool(i);
			qwID = pstTCB->stLink.qwID;
			if(((qwID>>32)!=0)&&((pstTCB->qwFlags & TASK_FLAGS_SYSTEM)==0x00)) {
				kPrintf("kill Task ID [0x%q] ",qwID);
				if(kEndTask(qwID)==TRUE)
					kPrintf("Success\n");
				else
					kPrintf("Fail\n");
			}
		}
	}

}

static void kCPULoad(const char* pcParameterbuffer) {
	kPrintf("Processor Load : %d%%\n",kGetProcessorLoad());
}

static void kPrintNumberTask() {
	int i;
	int j;
	QWORD qwTickCount;
	qwTickCount = kGetTickCount();
	//50ms占쏙옙占쏙옙 占쏙옙占쏙옙臼占� 占쌤솔쇽옙占쏙옙 占쏙옙占쏙옙求占� 占쌨쏙옙占쏙옙占쏙옙 占쏙옙치占쏙옙 占십곤옙 占쏙옙
	while((kGetTickCount() - qwTickCount) < 50) {
		kSchedule();

	}
		for(i=0; i<5; i++) {
		kLock(&(gs_stMutex));
		kPrintf("Task ID [0x%Q] Value[%d]\n",kGetRunningTask()->stLink.qwID,gs_qwAdder);
		gs_qwAdder +=1;
		kUnlock(&(gs_stMutex));
		//占쏙옙占싸쇽옙占쏙옙 占쌀몌옙 占쏙옙占쏙옙 占쌘듸옙
		for(j=0; j<30000; j++);
	}

	qwTickCount = kGetTickCount();
	while((kGetTickCount() - qwTickCount)<1000)
		kSchedule();

	kExitTask();
}

static void kTestMutex(const char* pcParameterBuffer) {
	int i;
	gs_qwAdder = 1;
	kInitializeMutex(&gs_stMutex);
	for(i=0; i<3; i++) {
		kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD,0,0, (QWORD)kPrintNumberTask);
	}
	kPrintf("Wait Util %d Task End....\n",i);
	kGetCh();
}

static void kCreateThreadTask() {
	int i;
	for(i=0; i<3;i++) {
		kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD,0,0,(QWORD)kTestTask2);
	}
	while(1)
		kSleep(1);
}

static void kTestThread(const char* pcParameterBuffer) {
	TCB* pstProcess;
	pstProcess = kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_PROCESS,
			(void*)0xeeeeeeee, 0x1000, (QWORD)kCreateThreadTask);
	if(pstProcess!=NULL)
		kPrintf("Process [0x%q] Create Success\n",pstProcess->stLink.qwID);
	else
		kPrintf("Process Create Fail\n");

}

static void kDropCharactorThread( void )
{
    int iX, iY;
    int i;
    char vcText[ 2 ] = { 0, };

    iX = kRandom() % CONSOLE_WIDTH;

    while( 1 )
    {
        // 占쏙옙占� 占쏙옙占쏙옙占�
        kSleep( kRandom() % 20 );

        if( ( kRandom() % 20 ) < 16 )
        {
            vcText[ 0 ] = ' ';
            for( i = 0 ; i < CONSOLE_HEIGHT - 1 ; i++ )
            {
                kPrintStringXY( iX, i , vcText );
                kSleep( 50 );
            }
        }
        else
        {
            for( i = 0 ; i < CONSOLE_HEIGHT - 1 ; i++ )
            {
                vcText[ 0 ] = ( i + kRandom() ) % 128;
                kPrintStringXY( iX, i, vcText );
                kSleep( 50 );
            }
        }

    }
}

/**
 *  占쏙옙占쏙옙占썲를 占쏙옙占쏙옙占싹울옙 占쏙옙트占쏙옙占쏙옙 화占쏙옙처占쏙옙 占쏙옙占쏙옙占쌍댐옙 占쏙옙占싸쇽옙占쏙옙
 */
static void kMatrixProcess( void )
{
    int i;

    for( i = 0 ; i < 300 ; i++ )
    {
        if( kCreateTask( TASK_FLAGS_PROCESS | TASK_FLAGS_LOW, 0, 0,
            ( QWORD ) kDropCharactorThread) == NULL )
        {
            break;
        }

        kSleep( kRandom() % 5 + 5 );
    }

    kPrintf( "%d Thread is created!!!!!!!!!!!!!!!!!!!!!!!1\n", i );

    // 키占쏙옙 占쌉력되몌옙 占쏙옙占싸쇽옙占쏙옙 占쏙옙占쏙옙
    kGetCh();
}

/**
 *  占쏙옙트占쏙옙占쏙옙 화占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙
 */
static void kShowMatrix( const char* pcParameterBuffer )
{
    TCB* pstProcess;

    pstProcess = kCreateTask( TASK_FLAGS_PROCESS | TASK_FLAGS_HIGHEST, ( void* ) 0xE00000, 0xE00000,
                              ( QWORD ) kMatrixProcess);
    if( pstProcess != NULL )
    {
        kPrintf( "Matrix Process [0x%Q] Create Success\n" );

        // 占승쏙옙크占쏙옙 占쏙옙占쏙옙 占쏙옙 占쏙옙占쏙옙占쏙옙 占쏙옙占�
        while( ( pstProcess->stLink.qwID >> 32 ) != 0 )
        {
            kSleep( 100 );
        }
    }
    else
    {
        kPrintf( "Matrix Process Create Fail\n" );
    }
}

static void kFPUTestTask() {
	double dValue1;
	double dValue2;
	TCB* pstRunningTask;
	QWORD qwCount = 0;
	QWORD qwRandomValue;
	int i;
	int iOffset;
	char vcData[4] = {'-','\\','|','/'};
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;

	pstRunningTask = kGetRunningTask();

	iOffset = (pstRunningTask->stLink.qwID & 0xffffffff) * 2;
	iOffset = CONSOLE_WIDTH * CONSOLE_HEIGHT - (iOffset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

	while(1) {
		dValue1 = 1;
		dValue2 = 1;

		for(i=0; i<10; i++) {
			qwRandomValue = kRandom();
			dValue1*=(double)qwRandomValue;
			dValue2*=(double)qwRandomValue;

			kSleep(3);

			qwRandomValue = kRandom();
			dValue1 /= (double)qwRandomValue;
			dValue2 /= (double)qwRandomValue;

		}

		if(dValue1 != dValue2) {
			kPrintf("Value is not same~!!! [%f] != [%f]\n",dValue1,dValue2);
			break;
		}
		qwCount++;
		pstScreen[iOffset].bCharactor = vcData[qwCount%4];

		pstScreen[iOffset].bAttribute = (iOffset%15) + 1;
		//kPrintf("ID : 0x%q\n",pstRunningTask->stLink.qwID);
	}
}

static void kTestPIE(const char* pcParameterBuffer) {
	double dResult;
	int i;
	kPrintf("PIE Caculation Test\n");
	kPrintf("Result: 355/113 = ");
	dResult = (double)355/113;
	kPrintf("%d.%d%d\n",(QWORD)dResult,((QWORD)(dResult*10)%10),((QWORD)(dResult*100)%10));

	for(i=0; i<10; i++)
		kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD,0,0,(QWORD)kFPUTestTask);
}

static void kShowDynamicMemoryInformation(const char* pcParameterBuffer) {
	QWORD qwStartAddress, qwTotalSize, qwMetaSize, qwUsedSize;
	kGetDynamicMemoryInformation(&qwStartAddress, &qwTotalSize, &qwMetaSize, &qwUsedSize);
	kPrintf("====================dynamic memory information==================\n");
	kPrintf("Start Address: [0x%q]\n",qwStartAddress);
	kPrintf("Total Size   : [0x%q]byte, [%d]MB\n",qwTotalSize,qwTotalSize/1024/1024);
	kPrintf("Meta Size    : [0x%q]byte, [%d]KB\n",qwMetaSize, qwMetaSize/1024);
	kPrintf("Used Size    : [0x%q]byte, [%d]KB\n",qwUsedSize, qwUsedSize/1024);
}

static void kTestSequentialAllocation(const char* pcParameterBuffer) {
	DYNAMICMEMORY* pstMemory;
	long i,j,k;
	QWORD* pqwBuffer;

	kPrintf("==========Dynamic Memory Test==========\n");
	pstMemory = kGetDynamicMemoryManager();

	for(i=0; i<pstMemory->iMaxLevelCount; i++) {
		kPrintf("Block List [%d] Test Start\n",i);
		kPrintf("Allocation And Compare: ");

		//모든 블록을 할당받아서 값을 채운후 계산
		for(j=0; j<(pstMemory->iBlockCountOfSmallestBlock>>i); j++) {
			pqwBuffer = kAllocateMemory(DYNAMICMEMORY_MIN_SIZE<<i);
			if(pqwBuffer==NULL) {
				kPrintf("\nAllocation Fail\n");
				return;
			}

			for(k=0; k<(DYNAMICMEMORY_MIN_SIZE<<i)/8; k++) {
				pqwBuffer[k] = k;
			}
			kPrintf(".");
		}
		kPrintf("\nFree: ");

		for(j=0; j<(pstMemory->iBlockCountOfSmallestBlock>>i); j++) {
			if(kFreeMemory((void*)(pstMemory->qwStartAddress + (DYNAMICMEMORY_MIN_SIZE<<i)*j))==FALSE) {
				kPrintf("\nFree Fail\n");
				return;
			}
			kPrintf(".");
		}
		kPrintf("\n");
	}
	kPrintf("Test Complete!\n");
}


static void kRandomAllocationTask() {
	TCB* pstTask;
	QWORD qwMemorySize;
	char vcBuffer[200];
	BYTE* pbAllocationBuffer;
	int i,j;
	int iY;

	pstTask = kGetRunningTask();
	iY = (pstTask->stLink.qwID) % 15 + 9;

	for(j=0;j<10; j++) {
		do {
			qwMemorySize = ((kRandom()%(32*1024))+1)*1024;
			pbAllocationBuffer = kAllocateMemory(qwMemorySize);

			if(pbAllocationBuffer == 0) {
				kSleep(1);
			}
		} while(pbAllocationBuffer==0);

		kSPrintf(vcBuffer, "|Address: [0x%Q] Size: [0x%Q] Allocation Success",pbAllocationBuffer,qwMemorySize);
		kPrintStringXY(20,iY,vcBuffer);
		kSleep(200);

		kSPrintf(vcBuffer, "|Address: [0x%q] Size: [0x%q] Data Write...         ",pbAllocationBuffer, qwMemorySize);
		kPrintStringXY(20,iY,vcBuffer);
		for(i=0; i<qwMemorySize/2; i++) {
			pbAllocationBuffer[i] = kRandom() & 0xff;
			pbAllocationBuffer[i + (qwMemorySize/2)] = pbAllocationBuffer[i];
		}
		kSleep(200);

		kSPrintf(vcBuffer, "|Address: [0x%q] Size: [0x%q] Data Verify...     ",pbAllocationBuffer, qwMemorySize);
		kPrintStringXY(20, iY, vcBuffer);

		for(i=0; i<qwMemorySize/2; i++) {
			if(pbAllocationBuffer[i] != pbAllocationBuffer[i+(qwMemorySize/2)]) {
				kPrintf("Task ID[0x%q] Verify Fail\n", pstTask->stLink.qwID);
				kExitTask();
			}
		}
		kFreeMemory(pbAllocationBuffer);
		kSleep(200);
	}
	kExitTask();
}

static void kTestRandomAllocation(const char* pcParameterBuffer) {
	int i;
	for(i=0; i<1000; i++) {
		kCreateTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD,0,0,(QWORD)kRandomAllocationTask);
	}
}

static void kShowHDDInformation( const char* pcParameterBuffer )
{
    HDDINFORMATION stHDD;
    char vcBuffer[ 100 ];

    // 하드 디스크의 정보를 읽음
    if( kReadHDDInformation(TRUE,TRUE, &stHDD) == FALSE )
    {
        kPrintf( "HDD Information Read Fail\n" );
        return ;
    }

    kPrintf( "============ Primary Master HDD Information ============\n" );

    // 모델 번호 출력
    kMemCpy( vcBuffer, stHDD.vwModelNumber, sizeof( stHDD.vwModelNumber ) );
    vcBuffer[ sizeof( stHDD.vwModelNumber ) - 1 ] = '\0';
    kPrintf( "Model Number:\t %s\n", vcBuffer );

    // 시리얼 번호 출력
    kMemCpy( vcBuffer, stHDD.vwSerialNumber, sizeof( stHDD.vwSerialNumber ) );
    vcBuffer[ sizeof( stHDD.vwSerialNumber ) - 1 ] = '\0';
    kPrintf( "Serial Number:\t %s\n", vcBuffer );

    // 헤드, 실린더, 실린더 당 섹터 수를 출력
    kPrintf( "Head Count:\t %d\n", stHDD.wNumberOfHead );
    kPrintf( "Cylinder Count:\t %d\n", stHDD.wNumberOfCylinder );
    kPrintf( "Sector Count:\t %d\n", stHDD.wNumberOfSectorPerCylinder );

    // 총 섹터 수 출력
    kPrintf( "Total Sector:\t %d Sector, %dMB\n", stHDD.dwTotalSectors,
            stHDD.dwTotalSectors / 2 / 1024 );
}

/**
 *  하드 디스크에 파라미터로 넘어온 LBA 어드레스에서 섹터 수 만큼 읽음
 */
static void kReadSector( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcLBA[ 50 ], vcSectorCount[ 50 ];
    DWORD dwLBA;
    int iSectorCount;
    char* pcBuffer;
    int i, j;
    BYTE bData;
    BOOL bExit = FALSE;

    // 파라미터 리스트를 초기화하여 LBA 어드레스와 섹터 수 추출
    kInitializeParameter( &stList, pcParameterBuffer );
    if( ( kGetNextParameter( &stList, vcLBA ) == 0 ) ||
        ( kGetNextParameter( &stList, vcSectorCount ) == 0 ) )
    {
        kPrintf( "ex) readsector 0(LBA) 10(count)\n" );
        return ;
    }
    dwLBA = kAToI( vcLBA, 10 );
    iSectorCount = kAToI( vcSectorCount, 10 );

    // 섹터 수만큼 메모리를 할당 받아 읽기 수행
    pcBuffer = kAllocateMemory( iSectorCount * 512 );
    if( kReadHDDSector( TRUE, TRUE, dwLBA, iSectorCount, pcBuffer ) == iSectorCount )
    {
        kPrintf( "LBA [%d], [%d] Sector Read Success~!!", dwLBA, iSectorCount );
        // 데이터 버퍼의 내용을 출력
        for( j = 0 ; j < iSectorCount ; j++ )
        {
            for( i = 0 ; i < 512 ; i++ )
            {
                if( !( ( j == 0 ) && ( i == 0 ) ) && ( ( i % 256 ) == 0 ) )
                {
                    kPrintf( "\nPress any key to continue... ('q' is exit) : " );
                    if( kGetCh() == 'q' )
                    {
                        bExit = TRUE;
                        break;
                    }
                }

                if( ( i % 16 ) == 0 )
                {
                    kPrintf( "\n[LBA:%d, Offset:%d]\t| ", dwLBA + j, i );
                }

                // 모두 두 자리로 표시하려고 16보다 작은 경우 0을 추가해줌
                bData = pcBuffer[ j * 512 + i ] & 0xFF;
                if( bData < 16 )
                {
                    kPrintf( "0" );
                }
                kPrintf( "%X ", bData );
            }

            if( bExit == TRUE )
            {
                break;
            }
        }
        kPrintf( "\n" );
    }
    else
    {
        kPrintf( "Read Fail\n" );
    }

    kFreeMemory( pcBuffer );
}

/**
 *  하드 디스크에 파라미터로 넘어온 LBA 어드레스에서 섹터 수 만큼 씀
 */
static void kWriteSector( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcLBA[ 50 ], vcSectorCount[ 50 ];
    DWORD dwLBA;
    int iSectorCount;
    char* pcBuffer;
    int i, j;
    BOOL bExit = FALSE;
    BYTE bData;
    static DWORD s_dwWriteCount = 0;

    // 파라미터 리스트를 초기화하여 LBA 어드레스와 섹터 수 추출
    kInitializeParameter( &stList, pcParameterBuffer );
    if( ( kGetNextParameter( &stList, vcLBA ) == 0 ) ||
        ( kGetNextParameter( &stList, vcSectorCount ) == 0 ) )
    {
        kPrintf( "ex) writesector 0(LBA) 10(count)\n" );
        return ;
    }
    dwLBA = kAToI( vcLBA, 10 );
    iSectorCount = kAToI( vcSectorCount, 10 );

    s_dwWriteCount++;

    // 버퍼를 할당 받아 데이터를 채움.
    // 패턴은 4 바이트의 LBA 어드레스와 4 바이트의 쓰기가 수행된 횟수로 생성
    pcBuffer = kAllocateMemory( iSectorCount * 512 );
    for( j = 0 ; j < iSectorCount ; j++ )
    {
        for( i = 0 ; i < 512 ; i += 8 )
        {
            *( DWORD* ) &( pcBuffer[ j * 512 + i ] ) = dwLBA + j;
            *( DWORD* ) &( pcBuffer[ j * 512 + i + 4 ] ) = s_dwWriteCount;
        }
    }

    // 쓰기 수행
    if( kWriteHDDSector( TRUE, TRUE, dwLBA, iSectorCount, pcBuffer ) != iSectorCount )
    {
        kPrintf( "Write Fail\n" );
        return ;
    }
    kPrintf( "LBA [%d], [%d] Sector Write Success~!!", dwLBA, iSectorCount );

    // 데이터 버퍼의 내용을 출력
    for( j = 0 ; j < iSectorCount ; j++ )
    {
        for( i = 0 ; i < 512 ; i++ )
        {
            if( !( ( j == 0 ) && ( i == 0 ) ) && ( ( i % 256 ) == 0 ) )
            {
                kPrintf( "\nPress any key to continue... ('q' is exit) : " );
                if( kGetCh() == 'q' )
                {
                    bExit = TRUE;
                    break;
                }
            }

            if( ( i % 16 ) == 0 )
            {
                kPrintf( "\n[LBA:%d, Offset:%d]\t| ", dwLBA + j, i );
            }

            // 모두 두 자리로 표시하려고 16보다 작은 경우 0을 추가해줌
            bData = pcBuffer[ j * 512 + i ] & 0xFF;
            if( bData < 16 )
            {
                kPrintf( "0" );
            }
            kPrintf( "%X ", bData );
        }

        if( bExit == TRUE )
        {
            break;
        }
    }
    kPrintf( "\n" );
    kFreeMemory( pcBuffer );
}

static void kMountHDD(const char* pcParameterBuffer) {
	if(kMount()==FALSE) {
		kPrintf("HDD Mount Fail\n");
		return ;
	}
	kPrintf("HDD Mount Success\n");

}

static void kFormatHDD(const char* pcParameterBuffer) {
	if(kFormat()==FALSE) {
		kPrintf("HDD Format Fail\n");
		return;
	}
	kPrintf("HDD Format Success\n");
}

static void kShowFileSystemInformation(const char* pcParameterBuffer) {
	FILESYSTEMMANAGER stManager;
	kGetFileSystemInformation(&stManager);
	kPrintf("===============File System Information=============\n");
	kPrintf("Mounted:\t\t\t\t\t %d\n",stManager.bMounted);
	kPrintf("Reserved Sector Count:\t\t\t %d Sector\n", stManager.dwReservedSectorCount);
	kPrintf("Cluster Link Table StartAddress:\t %d Sector\n",stManager.dwClusterLinkAreaStartAddress);
	kPrintf("Cluster LinkTable Size:\t\t %d Sector\n", stManager.dwClusterLinkAreaSize);
	kPrintf("Data Area Start Address:\t\t %d Sector\n",stManager.dwDataAreaStartAddress);
	kPrintf("Total Cluster Count:\t\t\t %d Cluster\n",stManager.dwTotalClusterCount);
}

static void kCreateFileInRootDirectory(const char* pcParameterBuffer) {
	PARAMETERLIST stList;
	char vcFileName[50];
	int iLength;
	DWORD dwCluster;
	DIRECTORYENTRY stEntry;
	int i;
	FILE* pstFile;

	kInitializeParameter(&stList, pcParameterBuffer);
	iLength = kGetNextParameter(&stList,vcFileName);
	vcFileName[iLength] = '\0';
	if((iLength > (sizeof(stEntry.vcFileName) - 1)) || (iLength==0)) {
		kPrintf("Too Ling or Too Short File Name\n");
		return ;
	}

    pstFile = fopen( vcFileName, "w" );
    if( pstFile == NULL )
    {
        kPrintf( "File Create Fail\n" );
        return;
    }
    fclose( pstFile );
    kPrintf( "File Create Success\n" );
}

static void kDeleteFileInRootDirectory(const char* pcParameterBuffer) {
	PARAMETERLIST stList;
	char vcFileName[ 50 ];
	int iLength;

	// 파라미터 리스트를 초기화하여 파일 이름을 추출
	kInitializeParameter( &stList, pcParameterBuffer );
	iLength = kGetNextParameter( &stList, vcFileName );
	vcFileName[ iLength ] = '\0';
	if( ( iLength > ( FILESYSTEM_MAXFILENAMELENGTH - 1 ) ) || ( iLength == 0 ) )
	{
		kPrintf( "Too Long or Too Short File Name\n" );
		return ;
	}

	if( remove( vcFileName ) != 0 )
	{
		kPrintf( "File Not Found or File Opened\n" );
		return ;
	}

	kPrintf( "File Delete Success\n" );
}

static void kShowRootDirectory(const char* pcParameterBuffer) {
	DIR* pstDirectory;
	int i, iCount, iTotalCount;
	struct dirent* pstEntry;
	char vcBuffer[ 400 ];
	char vcTempValue[ 50 ];
	DWORD dwTotalByte;
	DWORD dwUsedClusterCount;
	FILESYSTEMMANAGER stManager;

	// 파일 시스템 정보를 얻음
	kGetFileSystemInformation( &stManager );

	// 루트 디렉터리를 엶
	pstDirectory = opendir( "/" );
	if( pstDirectory == NULL )
	{
		kPrintf( "Root Directory Open Fail\n" );
		return ;
	}

	// 먼저 루프를 돌면서 디렉터리에 있는 파일의 개수와 전체 파일이 사용한 크기를 계산
	iTotalCount = 0;
	dwTotalByte = 0;
	dwUsedClusterCount = 0;
	while( 1 )
	{
		// 디렉터리에서 엔트리 하나를 읽음
		pstEntry = readdir( pstDirectory );
		// 더이상 파일이 없으면 나감
		if( pstEntry == NULL )
		{
			break;
		}
		iTotalCount++;
		dwTotalByte += pstEntry->dwFileSize;

		// 실제로 사용된 클러스터의 개수를 계산
		if( pstEntry->dwFileSize == 0 )
		{
			// 크기가 0이라도 클러스터 1개는 할당되어 있음
			dwUsedClusterCount++;
		}
		else
		{
			// 클러스터 개수를 올림하여 더함
			dwUsedClusterCount += ( pstEntry->dwFileSize +
				( FILESYSTEM_CLUSTERSIZE - 1 ) ) / FILESYSTEM_CLUSTERSIZE;
		}
	}

	// 실제 파일의 내용을 표시하는 루프
	rewinddir( pstDirectory );
	iCount = 0;
	while( 1 )
	{
		// 디렉터리에서 엔트리 하나를 읽음
		pstEntry = readdir( pstDirectory );
		// 더이상 파일이 없으면 나감
		if( pstEntry == NULL )
		{
			break;
		}

		// 전부 공백으로 초기화 한 후 각 위치에 값을 대입
		kMemSet( vcBuffer, ' ', sizeof( vcBuffer ) - 1 );
		vcBuffer[ sizeof( vcBuffer ) - 1 ] = '\0';

		// 파일 이름 삽입
		kMemCpy( vcBuffer, pstEntry->d_name,
				 kStrLen( pstEntry->d_name ) );

		// 파일 길이 삽입
		kSPrintf( vcTempValue, "%d Byte", pstEntry->dwFileSize );
		kMemCpy( vcBuffer + 30, vcTempValue, kStrLen( vcTempValue ) );

		// 파일의 시작 클러스터 삽입
		kSPrintf( vcTempValue, "0x%X Cluster", pstEntry->dwStartClusterIndex );
		kMemCpy( vcBuffer + 55, vcTempValue, kStrLen( vcTempValue ) + 1 );
		kPrintf( "    %s\n", vcBuffer );

		if( ( iCount != 0 ) && ( ( iCount % 20 ) == 0 ) )
		{
			kPrintf( "Press any key to continue... ('q' is exit) : " );
			if( kGetCh() == 'q' )
			{
				kPrintf( "\n" );
				break;
			}
		}
		iCount++;
	}

	// 총 파일의 개수와 파일의 총 크기를 출력
	kPrintf( "\t\tTotal File Count: %d\n", iTotalCount );
	kPrintf( "\t\tTotal File Size: %d KByte (%d Cluster)\n", dwTotalByte,
			 dwUsedClusterCount );

	// 남은 클러스터 수를 이용해서 여유 공간을 출력
	kPrintf( "\t\tFree Space: %d KByte (%d Cluster)\n",
			 ( stManager.dwTotalClusterCount - dwUsedClusterCount ) *
			 FILESYSTEM_CLUSTERSIZE / 1024, stManager.dwTotalClusterCount -
			 dwUsedClusterCount );

	// 디렉터리를 닫음
	closedir( pstDirectory );
}

static void kWriteDataToFile( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcFileName[ 50 ];
    int iLength;
    FILE* fp;
    int iEnterCount;
    BYTE bKey;

    // 파라미터 리스트를 초기화하여 파일 이름을 추출
    kInitializeParameter( &stList, pcParameterBuffer );
    iLength = kGetNextParameter( &stList, vcFileName );
    vcFileName[ iLength ] = '\0';
    if( ( iLength > ( FILESYSTEM_MAXFILENAMELENGTH - 1 ) ) || ( iLength == 0 ) )
    {
        kPrintf( "Too Long or Too Short File Name\n" );
        return ;
    }

    // 파일 생성
    fp = fopen( vcFileName, "w" );
    if( fp == NULL )
    {
        kPrintf( "%s File Open Fail\n", vcFileName );
        return ;
    }

    // 엔터 키가 연속으로 3번 눌러질 때까지 내용을 파일에 씀
    iEnterCount = 0;
    while( 1 )
    {
        bKey = kGetCh();
        // 엔터 키이면 연속 3번 눌러졌는가 확인하여 루프를 빠져 나감
        if( bKey == KEY_ENTER )
        {
            iEnterCount++;
            if( iEnterCount >= 3 )
            {
                break;
            }
        }
        // 엔터 키가 아니라면 엔터 키 입력 횟수를 초기화
        else
        {
            iEnterCount = 0;
        }

        kPrintf( "%c", bKey );
        if( fwrite( &bKey, 1, 1, fp ) != 1 )
        {
            kPrintf( "File Wirte Fail\n" );
            break;
        }
    }

    kPrintf( "File Create Success\n" );
    fclose( fp );
}

/**
 *  파일을 열어서 데이터를 읽음
 */
static void kReadDataFromFile( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcFileName[ 50 ];
    int iLength;
    FILE* fp;
    int iEnterCount;
    BYTE bKey;

    // 파라미터 리스트를 초기화하여 파일 이름을 추출
    kInitializeParameter( &stList, pcParameterBuffer );
    iLength = kGetNextParameter( &stList, vcFileName );
    vcFileName[ iLength ] = '\0';
    if( ( iLength > ( FILESYSTEM_MAXFILENAMELENGTH - 1 ) ) || ( iLength == 0 ) )
    {
        kPrintf( "Too Long or Too Short File Name\n" );
        return ;
    }

    // 파일 생성
    fp = fopen( vcFileName, "r" );
    if( fp == NULL )
    {
        kPrintf( "%s File Open Fail\n", vcFileName );
        return ;
    }

    // 파일의 끝까지 출력하는 것을 반복
    iEnterCount = 0;
    while( 1 )
    {
        if( fread( &bKey, 1, 1, fp ) != 1 )
        {
            break;
        }
        kPrintf( "%c", bKey );

        // 만약 엔터 키이면 엔터 키 횟수를 증가시키고 20라인까지 출력했다면
        // 더 출력할지 여부를 물어봄
        if( bKey == KEY_ENTER )
        {
            iEnterCount++;

            if( ( iEnterCount != 0 ) && ( ( iEnterCount % 20 ) == 0 ) )
            {
                kPrintf( "Press any key to continue... ('q' is exit) : " );
                if( kGetCh() == 'q' )
                {
                    kPrintf( "\n" );
                    break;
                }
                kPrintf( "\n" );
                iEnterCount = 0;
            }
        }
    }
    fclose( fp );
}

static void kTestFileIO(const char* pcParameterBuffer) {
	FILE* pstFile;
	BYTE* pbBuffer;
	int i;
	int j;
	DWORD dwRandomOffset;
	DWORD dwByteCount;
	BYTE vbTempBuffer[1024];
	DWORD dwMaxFileSize;

	kPrintf("==============File I/O Function Test============\n");

	//4MB 버퍼 할당
	dwMaxFileSize = 4 * 1024 * 1024;
	pbBuffer = kAllocateMemory(dwMaxFileSize);
	if(pbBuffer==NULL) {
		kPrintf("Memory Allocation Fail\n");
		return;
	}
	remove("testfileio.bin");

	//////////////////////////////
	//파일 열기 테스트
	//////////////////////////////
	kPrintf("1. File Open Fail Test...");
	pstFile = fopen(*"testfileio.bin" , "r");
	if(pstFile==NULL) {
		kPrintf("[Pass]\n");
	}
	else {
		kPrintf("[Fail]\n");
		fclose(pstFile);
	}

	////////////////////////////////
	//파일 생성 테스트
	////////////////////////////////
	kPrintf("2. File Create Test...");
	pstFile = fopen("testfileio.bin","w");
	if(pstFile != NULL) {
		kPrintf("[PASS]\n");
		kPrintf(" 	File Handle [0x%q]\n",pstFile);
	}
	else
		kPrintf("[Fail]\n");

	/////////////////
	//순차적인 영역 쓰기 테스트
	/////////////////
	kPrintf("3. Sequential Write Test(Cluster Size)...");
	for(i=0; i<100; i++) {
		kMemSet(pbBuffer,i,FILESYSTEM_CLUSTERSIZE);
		if(fwrite(pbBuffer,1,FILESYSTEM_CLUSTERSIZE, pstFile)!=FILESYSTEM_CLUSTERSIZE) {
			kPrintf("[Fail]\n");
			kPrintf("   %d Cluster Error\n", i);
			break;
		}
	}

	if(i>=100)
		kPrintf("Pass\n");


}

void TestCode() {
	int a = 1;
	int b = 2;
	kPrintf("aaaa 0x%x 0x%x %x %x %x %x %x %x\n",a,b);
}
