#include "HardDisk.h"
#include "AssemblyUtility.h"
#include "Utility.h"

static HDDMANAGER gs_stHDDManager;

BOOL kInitializeHDD() {
	//뮤텍스 초기화
	kInitializeMutex(&(gs_stHDDManager.stMutex));

	//인터럽트 플래그 초기화
	gs_stHDDManager.bPrimaryInterruptOccur = FALSE;
	gs_stHDDManager.bPrimaryInterruptOccur = FALSE;

	//첫번째와 두번째 PATA포트의 디지털 출력 레지스터에 0을 출력함으로써
	//하드 디스크 컨트롤러의 인터럽트를 활성화시킴
	kOutPortByte(HDD_PORT_PRIMARYBASE + HDD_PORT_INDEX_DIGITALOUTPUT,0);
	kOutPortByte(HDD_PORT_SECONDARYBASE + HDD_PORT_INDEX_DIGITALOUTPUT,0);

	if(kReadHDDInformation(TRUE,TRUE, &(gs_stHDDManager.stHDDInformation))==FALSE) {
		gs_stHDDManager.bHDDDetected = FALSE;
		gs_stHDDManager.bCanWrite = FALSE;
		return FALSE;
	}
	gs_stHDDManager.bHDDDetected = TRUE;

	//하드디스크를 검색해서, QEMU에서만 쓸 수 있도록 함
	if(kMemCmp(gs_stHDDManager.stHDDInformation.vwModelNumber, "QEMU",4)==0)
		gs_stHDDManager.bCanWrite = TRUE;
	else
		gs_stHDDManager.bCanWrite = FALSE;
	return TRUE;
}

static BYTE kReadHDDStatus(BOOL bPrimary) {

	// 상태 레지스터 리턴
	if(bPrimary == TRUE) {
		return kInPortByte(HDD_PORT_PRIMARYBASE + HDD_PORT_INDEX_STATUS);
	}
	return kInPortByte(HDD_PORT_SECONDARYBASE);
}

static BOOL kWaitForHDDNoBusy(BOOL bPrimary) {
	//하드디스크가 busy상태인지를 리턴
	QWORD qwStartTickCount;
	BYTE bStatus;

	qwStartTickCount = kGetTickCount();

	//waittime만큼만 대기
	while((kGetTickCount() - qwStartTickCount) <= HDD_WAITTIME) {
		bStatus = kReadHDDStatus(bPrimary);
		//Busy비트가 설정되어 있지 않으면 no Busy 상태이므로 대기하지 않음
		if((bStatus & HDD_STATUS_BUSY) != HDD_STATUS_BUSY) {
			return TRUE;
		}

		kSleep(100);

	}
	return FALSE;
}

static BOOL kWaitForHDDReady(BOOL bPrimary) {
	QWORD qwStartTickCount;
	BYTE bStatus;

	qwStartTickCount = kGetTickCount();

	//하드디스크가 ready가 될 때까지 대기. 일정 시간만큼만 대기한다
	while((kGetTickCount()-qwStartTickCount)<=HDD_WAITTIME) {
		bStatus = kReadHDDStatus(bPrimary);

		//ready비트를 확인함
		if((bStatus & HDD_STATUS_READY)==HDD_STATUS_READY)
			return TRUE;
		kSleep(1);

	}
	return FALSE;
}

void kSetHDDInterruptFlag(BOOL bPrimary, BOOL bFlag) {
	if(bPrimary == TRUE)
		gs_stHDDManager.bPrimaryInterruptOccur = bFlag;
	else
		gs_stHDDManager.bSecondaryInterruptOccur = bFlag;
}

static BOOL kWaitForHDDInterrupt(BOOL bPrimary) {
	QWORD qwTickCount;

	qwTickCount = kGetTickCount();

	//wait time만큼 하드디스크의 인터럽가 발생할 때까지 대기
	while(kGetTickCount() - qwTickCount <= HDD_WAITTIME) {
		if((bPrimary==TRUE)&&(gs_stHDDManager.bPrimaryInterruptOccur = TRUE))
			return TRUE;
		else if ((bPrimary==FALSE)&& (gs_stHDDManager.bSecondaryInterruptOccur==TRUE))
			return TRUE;
	}
	return FALSE;
}

BOOL kReadHDDInformation(BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation) {
	WORD wPortBase;
	QWORD qwLastTickCount;
	BYTE bStatus;
	BYTE bDriveFlag;
	int i;
	WORD wTemp;
	BOOL bWaitResult;

	//PATA포트에 따라 I/O 포트의 기본 어드레스를 설정
	if(bPrimary==TRUE) {
		wPortBase = HDD_PORT_PRIMARYBASE;
	}
	else {
		wPortBase = HDD_PORT_SECONDARYBASE;
	}

	//뮤텍스를 이용한 동기화
	kLock(&(gs_stHDDManager.stMutex));

	//하드디스크가 busy상태인지 체크
	if(kWaitForHDDNoBusy(bPrimary)==FALSE) {
		kUnlock(&(gs_stHDDManager.stMutex));
		kPrintf("busy~~~~~~~~~~~~");
		return FALSE;
	}

	//HDD information을 읽기 전 LBA어드레스와 드라이브 및 헤드에 관련된 레지스터를 설정한다.
	if(bMaster==TRUE) {
		//마스터라면 slave 비트는 설정하지 않고 LBA플래그만 설정
		bDriveFlag = HDD_DRIVEANDHEAD_LBA;
	}
	else {
		bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
	}

	//설정된 값을 드라이브/헤드 레지스터에 전송
	kOutPortByte(wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag);

	if(kWaitForHDDReady(bPrimary)==FALSE) {
		kUnlock(&(gs_stHDDManager.stMutex));
		return FALSE;
	}
	//인터럽트 플래그를 초기화
	kSetHDDInterruptFlag(bPrimary, FALSE);

	//드라이브 인식 커맨드를 커맨드 레지스터에 전송
	kOutPortByte(wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_IDENTIFY);

	//처리가 완료될 때까지 인터럽트 발생을 기다림
	bWaitResult = kWaitForHDDInterrupt(bPrimary);
	bStatus = kReadHDDStatus(bPrimary);
	if((bWaitResult==FALSE) || ((bStatus & HDD_STATUS_ERROR)==HDD_STATUS_ERROR)) {
		kUnlock(&(gs_stHDDManager.stMutex));
		return FALSE;
	}

	//수신한 데이터를 처리
	for(i=0; i<256; i++) {
		((WORD*)pstHDDInformation)[i] = kInPortWord(wPortBase + HDD_PORT_INDEX_DATA);
	}

	//kInPortWord로 받은 WORD는 BYTE순서가 뒤집혀져 있으므로 하나씩 원래대로 돌려준다.
	kSwapByteInWord(pstHDDInformation->vwModelNumber, sizeof(pstHDDInformation->vwModelNumber)/2);
	kSwapByteInWord(pstHDDInformation->vwSerialNumber, sizeof(pstHDDInformation->vwSerialNumber)/2);

	kUnlock(&(gs_stHDDManager.stMutex));
	return TRUE;
}

static void kSwapByteInWord(WORD* pwData, int iWordCount) {
	int i;
	WORD wTemp;

	//swap을 한다.
	for(i=0; i<iWordCount; i++) {
		wTemp = pwData[i];
		pwData[i] = (wTemp>>8) | (wTemp << 8);
	}
}

int kReadHDDSector( BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount,
        char* pcBuffer )
{
    WORD wPortBase;
    int i, j;
    BYTE bDriveFlag;
    BYTE bStatus;
    long lReadCount = 0;
    BOOL bWaitResult;

    // 범위 검사. 최대 256 섹터를 처리할 수 있음
    if( ( gs_stHDDManager.bHDDDetected == FALSE ) ||
        ( iSectorCount <= 0 ) || ( 256 < iSectorCount ) ||
        ( ( dwLBA + iSectorCount ) >= gs_stHDDManager.stHDDInformation.dwTotalSectors ) )
    {
        return 0;
    }

    // PATA 포트에 따라서 I/O 포트의 기본 어드레스를 설정
    if( bPrimary == TRUE )
    {
        // 첫 번째 PATA 포트이면 포트 0x1F0을 저장
        wPortBase = HDD_PORT_PRIMARYBASE;
    }
    else
    {
        // 두 번째 PATA 포트이면 포트 0x170을 저장
        wPortBase = HDD_PORT_SECONDARYBASE;
    }

    // 동기화 처리
    kLock( &( gs_stHDDManager.stMutex ) );

    // 아직 수행 중인 커맨드가 있다면 일정 시간 동안 끝날 때까지 대기
    if( kWaitForHDDNoBusy( bPrimary ) == FALSE )
    {
        // 동기화 처리
        kUnlock( &( gs_stHDDManager.stMutex ) );
        return FALSE;
    }

    //==========================================================================
    //  데이터 레지스터 설정
    //      LBA 모드의 경우, 섹터 번호 -> 실린더 번호 -> 헤드 번호의 순으로
    //      LBA 어드레스를 대입
    //==========================================================================
    // 섹터 수 레지스터(포트 0x1F2 또는 0x172)에 읽을 섹터 수를 전송
    kOutPortByte( wPortBase + HDD_PORT_INDEX_SECTORCOUNT, iSectorCount );
    // 섹터 번호 레지스터(포트 0x1F3 또는 0x173)에 읽을 섹터 위치(LBA 0~7비트)를 전송
    kOutPortByte( wPortBase + HDD_PORT_INDEX_SECTORNUMBER, dwLBA );
    // 실린더 LSB 레지스터(포트 0x1F4 또는 0x174)에 읽을 섹터 위치(LBA 8~15비트)를 전송
    kOutPortByte( wPortBase + HDD_PORT_INDEX_CYLINDERLSB, dwLBA >> 8 );
    // 실린더 MSB 레지스터(포트 0x1F5 또는 0x175)에 읽을 섹터 위치(LBA 16~23비트)를 전송
    kOutPortByte( wPortBase + HDD_PORT_INDEX_CYLINDERMSB, dwLBA >> 16 );
    // 드라이브와 헤드 데이터 설정
    if( bMaster == TRUE )
    {
        bDriveFlag = HDD_DRIVEANDHEAD_LBA;
    }
    else
    {
        bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
    }
    // 드라이브/헤드 레지스터(포트 0x1F6 또는 0x176)에 읽을 섹터의 위치(LBA 24~27비트)와
    // 설정된 값을 같이 전송
    kOutPortByte( wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag | ( (dwLBA
            >> 24 ) & 0x0F ) );

    //==========================================================================
    //  커맨드 전송
    //==========================================================================
    // 커맨드를 받아들일 준비가 될 때까지 일정 시간 동안 대기
    if( kWaitForHDDReady( bPrimary ) == FALSE )
    {
        // 동기화 처리
        kUnlock( &( gs_stHDDManager.stMutex ) );
        return FALSE;
    }

    // 인터럽트 플래그를 초기화
    kSetHDDInterruptFlag( bPrimary, FALSE );

    // 커맨드 레지스터(포트 0x1F7 또는 0x177)에 읽기(0x20)을 전송
    kOutPortByte( wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_READ );

    //==========================================================================
    //  인터럽트 대기 후, 데이터 수신
    //==========================================================================
    // 섹터 수만큼 루프를 돌면서 데이터 수신
    for( i = 0 ; i < iSectorCount ; i++ )
    {
        // 에러가 발생하면 종료
        bStatus = kReadHDDStatus( bPrimary );
        if( ( bStatus & HDD_STATUS_ERROR ) == HDD_STATUS_ERROR )
        {
            kPrintf( "Error Occur\n" );
            // 동기화 처리
            kUnlock( &( gs_stHDDManager.stMutex ) );
            return i;
        }

        // DATAREQUEST 비트가 설정되지 않았으면 데이터가 수신되길 기다림
        if( ( bStatus & HDD_STATUS_DATAREQUEST ) != HDD_STATUS_DATAREQUEST )
        {
            // 처리가 완료될 때까지 일정 시간 동안 인터럽트를 기다림
            bWaitResult = kWaitForHDDInterrupt( bPrimary );
            kSetHDDInterruptFlag( bPrimary, FALSE );
            // 인터럽트가 발생하지 않으면 문제가 발생한 것이므로 종료
            if( bWaitResult == FALSE )
            {
                kPrintf( "Interrupt Not Occur\n" );
                // 동기화 처리
                kUnlock( &( gs_stHDDManager.stMutex ) );
                return FALSE;
            }
        }

        // 한 섹터를 읽음
        for( j = 0 ; j < 512 / 2 ; j++ )
        {
            ( ( WORD* ) pcBuffer )[ lReadCount++ ]
                    = kInPortWord( wPortBase + HDD_PORT_INDEX_DATA );
        }
    }

    // 동기화 처리
    kUnlock( &( gs_stHDDManager.stMutex ) );
    return i;
}




int kWriteHDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount,
        char* pcBuffer)
{
    WORD wPortBase;
    WORD wTemp;
    int i, j;
    BYTE bDriveFlag;
    BYTE bStatus;
    long lReadCount = 0;
    BOOL bWaitResult;

    // 범위 검사. 최대 256 섹터를 처리할 수 있음
    if( ( gs_stHDDManager.bCanWrite == FALSE ) ||
        ( iSectorCount <= 0 ) || ( 256 < iSectorCount ) ||
        ( ( dwLBA + iSectorCount ) >= gs_stHDDManager.stHDDInformation.dwTotalSectors ) )

    {
        return 0;
    }

    // PATA 포트에 따라서 I/O 포트의 기본 어드레스를 설정
    if( bPrimary == TRUE )
    {
        // 첫 번째 PATA 포트이면 포트 0x1F0을 저장
        wPortBase = HDD_PORT_PRIMARYBASE;
    }
    else
    {
        // 두 번째 PATA 포트이면 포트 0x170을 저장
        wPortBase = HDD_PORT_SECONDARYBASE;
    }

    // 아직 수행 중인 커맨드가 있다면 일정 시간 동안 끝날 때까지 대기
    if( kWaitForHDDNoBusy( bPrimary ) == FALSE )
    {
        return FALSE;
    }

    // 동기화 처리
    kLock( &(gs_stHDDManager.stMutex ) );

    //==========================================================================
    //  데이터 레지스터 설정
    //      LBA 모드의 경우, 섹터 번호 -> 실린더 번호 -> 헤드 번호의 순으로
    //      LBA 어드레스를 대입
    //==========================================================================
    // 섹터 수 레지스터(포트 0x1F2 또는 0x172)에 쓸 섹터 수를 전송
    kOutPortByte( wPortBase + HDD_PORT_INDEX_SECTORCOUNT, iSectorCount );
    // 섹터 번호 레지스터(포트 0x1F3 또는 0x173)에 쓸 섹터 위치(LBA 0~7비트)를 전송
    kOutPortByte( wPortBase + HDD_PORT_INDEX_SECTORNUMBER, dwLBA );
    // 실린더 LSB 레지스터(포트 0x1F4 또는 0x174)에 쓸 섹터 위치(LBA 8~15비트)를 전송
    kOutPortByte( wPortBase + HDD_PORT_INDEX_CYLINDERLSB, dwLBA >> 8 );
    // 실린더 MSB 레지스터(포트 0x1F5 또는 0x175)에 쓸 섹터 위치(LBA 16~23비트)를 전송
    kOutPortByte( wPortBase + HDD_PORT_INDEX_CYLINDERMSB, dwLBA >> 16 );
    // 드라이브와 헤드 데이터 설정
    if( bMaster == TRUE )
    {
        bDriveFlag = HDD_DRIVEANDHEAD_LBA;
    }
    else
    {
        bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
    }
    // 드라이브/헤드 레지스터(포트 0x1F6 또는 0x176)에 쓸 섹터의 위치(LBA 24~27비트)와
    // 설정된 값을 같이 전송
    kOutPortByte(wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag | ( (dwLBA
            >> 24 ) & 0x0F ) );

    //==========================================================================
    //  커맨드 전송 후, 데이터 송신이 가능할 때까지 대기
    //==========================================================================
    // 커맨드를 받아들일 준비가 될 때까지 일정 시간 동안 대기
    if( kWaitForHDDReady( bPrimary ) == FALSE )
    {
        // 동기화 처리
        kUnlock( &( gs_stHDDManager.stMutex ) );
        return FALSE;
    }

    // 커맨드 전송
    kOutPortByte( wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_WRITE );

    // 데이터 송신이 가능할 때까지 대기
    while( 1 )
    {
        bStatus = kReadHDDStatus( bPrimary );
        // 에러가 발생하면 종료
        if( ( bStatus & HDD_STATUS_ERROR ) == HDD_STATUS_ERROR )
        {
            // 동기화 처리
            kUnlock( &(gs_stHDDManager.stMutex ) );
            return 0;
        }

        // Data Request비트가 설정되었다면 데이터 송신 가능
        if( ( bStatus & HDD_STATUS_DATAREQUEST ) == HDD_STATUS_DATAREQUEST )
        {
            break;
        }

        kSleep( 1 );
    }

    //==========================================================================
    //  데이터 송신 후, 인터럽트 대기
    //==========================================================================
    // 섹터 수만큼 루프를 돌면서 데이터 송신
    for( i = 0 ; i < iSectorCount ; i++)
    {
        // 인터럽트 플래그를 초기화하고 한 섹터를 씀
        kSetHDDInterruptFlag( bPrimary, FALSE );
        for( j = 0 ; j < 512 / 2 ; j++ )
        {
            kOutPortWord( wPortBase + HDD_PORT_INDEX_DATA,
                         ( ( WORD* ) pcBuffer )[ lReadCount++ ]);
        }

        // 에러가 발생하면 종료
        bStatus = kReadHDDStatus( bPrimary );
        if( ( bStatus & HDD_STATUS_ERROR ) == HDD_STATUS_ERROR )
        {
            // 동기화 처리
            kUnlock( &(gs_stHDDManager.stMutex ) );
            return i;
        }

        // DATAREQUEST 비트가 설정되지 않았으면 데이터가 처리가 완료되길 기다림
        if( ( bStatus & HDD_STATUS_DATAREQUEST ) != HDD_STATUS_DATAREQUEST )
        {
            // 처리가 완료될 때까지 일정 시간 동안 인터럽트를 기다림
            bWaitResult = kWaitForHDDInterrupt( bPrimary );
            kSetHDDInterruptFlag( bPrimary, FALSE );
            // 인터럽트가 발생하지 않으면 문제가 발생한 것이므로 종료
            if( bWaitResult == FALSE )
            {
                // 동기화 처리
                kUnlock( &( gs_stHDDManager.stMutex ) );
                return FALSE;
            }
        }
    }
    // 동기화 처리
    kUnlock( &(gs_stHDDManager.stMutex ) );
    return i;
}
