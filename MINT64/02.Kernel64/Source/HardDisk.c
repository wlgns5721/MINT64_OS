#include "HardDisk.h"
#include "AssemblyUtility.h"
#include "Utility.h"

static HDDMANAGER gs_stHDDManager;

BOOL kInitializeHDD() {
	//���ؽ� �ʱ�ȭ
	kInitializeMutex(&(gs_stHDDManager.stMutex));

	//���ͷ�Ʈ �÷��� �ʱ�ȭ
	gs_stHDDManager.bPrimaryInterruptOccur = FALSE;
	gs_stHDDManager.bPrimaryInterruptOccur = FALSE;

	//ù��°�� �ι�° PATA��Ʈ�� ������ ��� �������Ϳ� 0�� ��������ν�
	//�ϵ� ��ũ ��Ʈ�ѷ��� ���ͷ�Ʈ�� Ȱ��ȭ��Ŵ
	kOutPortByte(HDD_PORT_PRIMARYBASE + HDD_PORT_INDEX_DIGITALOUTPUT,0);
	kOutPortByte(HDD_PORT_SECONDARYBASE + HDD_PORT_INDEX_DIGITALOUTPUT,0);

	if(kReadHDDInformation(TRUE,TRUE, &(gs_stHDDManager.stHDDInformation))==FALSE) {
		gs_stHDDManager.bHDDDetected = FALSE;
		gs_stHDDManager.bCanWrite = FALSE;
		return FALSE;
	}
	gs_stHDDManager.bHDDDetected = TRUE;

	//�ϵ��ũ�� �˻��ؼ�, QEMU������ �� �� �ֵ��� ��
	if(kMemCmp(gs_stHDDManager.stHDDInformation.vwModelNumber, "QEMU",4)==0)
		gs_stHDDManager.bCanWrite = TRUE;
	else
		gs_stHDDManager.bCanWrite = FALSE;
	return TRUE;
}

static BYTE kReadHDDStatus(BOOL bPrimary) {

	// ���� �������� ����
	if(bPrimary == TRUE) {
		return kInPortByte(HDD_PORT_PRIMARYBASE + HDD_PORT_INDEX_STATUS);
	}
	return kInPortByte(HDD_PORT_SECONDARYBASE);
}

static BOOL kWaitForHDDNoBusy(BOOL bPrimary) {
	//�ϵ��ũ�� busy���������� ����
	QWORD qwStartTickCount;
	BYTE bStatus;

	qwStartTickCount = kGetTickCount();

	//waittime��ŭ�� ���
	while((kGetTickCount() - qwStartTickCount) <= HDD_WAITTIME) {
		bStatus = kReadHDDStatus(bPrimary);
		//Busy��Ʈ�� �����Ǿ� ���� ������ no Busy �����̹Ƿ� ������� ����
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

	//�ϵ��ũ�� ready�� �� ������ ���. ���� �ð���ŭ�� ����Ѵ�
	while((kGetTickCount()-qwStartTickCount)<=HDD_WAITTIME) {
		bStatus = kReadHDDStatus(bPrimary);

		//ready��Ʈ�� Ȯ����
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

	//wait time��ŭ �ϵ��ũ�� ���ͷ��� �߻��� ������ ���
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

	//PATA��Ʈ�� ���� I/O ��Ʈ�� �⺻ ��巹���� ����
	if(bPrimary==TRUE) {
		wPortBase = HDD_PORT_PRIMARYBASE;
	}
	else {
		wPortBase = HDD_PORT_SECONDARYBASE;
	}

	//���ؽ��� �̿��� ����ȭ
	kLock(&(gs_stHDDManager.stMutex));

	//�ϵ��ũ�� busy�������� üũ
	if(kWaitForHDDNoBusy(bPrimary)==FALSE) {
		kUnlock(&(gs_stHDDManager.stMutex));
		kPrintf("busy~~~~~~~~~~~~");
		return FALSE;
	}

	//HDD information�� �б� �� LBA��巹���� ����̺� �� ��忡 ���õ� �������͸� �����Ѵ�.
	if(bMaster==TRUE) {
		//�����Ͷ�� slave ��Ʈ�� �������� �ʰ� LBA�÷��׸� ����
		bDriveFlag = HDD_DRIVEANDHEAD_LBA;
	}
	else {
		bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
	}

	//������ ���� ����̺�/��� �������Ϳ� ����
	kOutPortByte(wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag);

	if(kWaitForHDDReady(bPrimary)==FALSE) {
		kUnlock(&(gs_stHDDManager.stMutex));
		return FALSE;
	}
	//���ͷ�Ʈ �÷��׸� �ʱ�ȭ
	kSetHDDInterruptFlag(bPrimary, FALSE);

	//����̺� �ν� Ŀ�ǵ带 Ŀ�ǵ� �������Ϳ� ����
	kOutPortByte(wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_IDENTIFY);

	//ó���� �Ϸ�� ������ ���ͷ�Ʈ �߻��� ��ٸ�
	bWaitResult = kWaitForHDDInterrupt(bPrimary);
	bStatus = kReadHDDStatus(bPrimary);
	if((bWaitResult==FALSE) || ((bStatus & HDD_STATUS_ERROR)==HDD_STATUS_ERROR)) {
		kUnlock(&(gs_stHDDManager.stMutex));
		return FALSE;
	}

	//������ �����͸� ó��
	for(i=0; i<256; i++) {
		((WORD*)pstHDDInformation)[i] = kInPortWord(wPortBase + HDD_PORT_INDEX_DATA);
	}

	//kInPortWord�� ���� WORD�� BYTE������ �������� �����Ƿ� �ϳ��� ������� �����ش�.
	kSwapByteInWord(pstHDDInformation->vwModelNumber, sizeof(pstHDDInformation->vwModelNumber)/2);
	kSwapByteInWord(pstHDDInformation->vwSerialNumber, sizeof(pstHDDInformation->vwSerialNumber)/2);

	kUnlock(&(gs_stHDDManager.stMutex));
	return TRUE;
}

static void kSwapByteInWord(WORD* pwData, int iWordCount) {
	int i;
	WORD wTemp;

	//swap�� �Ѵ�.
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

    // ���� �˻�. �ִ� 256 ���͸� ó���� �� ����
    if( ( gs_stHDDManager.bHDDDetected == FALSE ) ||
        ( iSectorCount <= 0 ) || ( 256 < iSectorCount ) ||
        ( ( dwLBA + iSectorCount ) >= gs_stHDDManager.stHDDInformation.dwTotalSectors ) )
    {
        return 0;
    }

    // PATA ��Ʈ�� ���� I/O ��Ʈ�� �⺻ ��巹���� ����
    if( bPrimary == TRUE )
    {
        // ù ��° PATA ��Ʈ�̸� ��Ʈ 0x1F0�� ����
        wPortBase = HDD_PORT_PRIMARYBASE;
    }
    else
    {
        // �� ��° PATA ��Ʈ�̸� ��Ʈ 0x170�� ����
        wPortBase = HDD_PORT_SECONDARYBASE;
    }

    // ����ȭ ó��
    kLock( &( gs_stHDDManager.stMutex ) );

    // ���� ���� ���� Ŀ�ǵ尡 �ִٸ� ���� �ð� ���� ���� ������ ���
    if( kWaitForHDDNoBusy( bPrimary ) == FALSE )
    {
        // ����ȭ ó��
        kUnlock( &( gs_stHDDManager.stMutex ) );
        return FALSE;
    }

    //==========================================================================
    //  ������ �������� ����
    //      LBA ����� ���, ���� ��ȣ -> �Ǹ��� ��ȣ -> ��� ��ȣ�� ������
    //      LBA ��巹���� ����
    //==========================================================================
    // ���� �� ��������(��Ʈ 0x1F2 �Ǵ� 0x172)�� ���� ���� ���� ����
    kOutPortByte( wPortBase + HDD_PORT_INDEX_SECTORCOUNT, iSectorCount );
    // ���� ��ȣ ��������(��Ʈ 0x1F3 �Ǵ� 0x173)�� ���� ���� ��ġ(LBA 0~7��Ʈ)�� ����
    kOutPortByte( wPortBase + HDD_PORT_INDEX_SECTORNUMBER, dwLBA );
    // �Ǹ��� LSB ��������(��Ʈ 0x1F4 �Ǵ� 0x174)�� ���� ���� ��ġ(LBA 8~15��Ʈ)�� ����
    kOutPortByte( wPortBase + HDD_PORT_INDEX_CYLINDERLSB, dwLBA >> 8 );
    // �Ǹ��� MSB ��������(��Ʈ 0x1F5 �Ǵ� 0x175)�� ���� ���� ��ġ(LBA 16~23��Ʈ)�� ����
    kOutPortByte( wPortBase + HDD_PORT_INDEX_CYLINDERMSB, dwLBA >> 16 );
    // ����̺�� ��� ������ ����
    if( bMaster == TRUE )
    {
        bDriveFlag = HDD_DRIVEANDHEAD_LBA;
    }
    else
    {
        bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
    }
    // ����̺�/��� ��������(��Ʈ 0x1F6 �Ǵ� 0x176)�� ���� ������ ��ġ(LBA 24~27��Ʈ)��
    // ������ ���� ���� ����
    kOutPortByte( wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag | ( (dwLBA
            >> 24 ) & 0x0F ) );

    //==========================================================================
    //  Ŀ�ǵ� ����
    //==========================================================================
    // Ŀ�ǵ带 �޾Ƶ��� �غ� �� ������ ���� �ð� ���� ���
    if( kWaitForHDDReady( bPrimary ) == FALSE )
    {
        // ����ȭ ó��
        kUnlock( &( gs_stHDDManager.stMutex ) );
        return FALSE;
    }

    // ���ͷ�Ʈ �÷��׸� �ʱ�ȭ
    kSetHDDInterruptFlag( bPrimary, FALSE );

    // Ŀ�ǵ� ��������(��Ʈ 0x1F7 �Ǵ� 0x177)�� �б�(0x20)�� ����
    kOutPortByte( wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_READ );

    //==========================================================================
    //  ���ͷ�Ʈ ��� ��, ������ ����
    //==========================================================================
    // ���� ����ŭ ������ ���鼭 ������ ����
    for( i = 0 ; i < iSectorCount ; i++ )
    {
        // ������ �߻��ϸ� ����
        bStatus = kReadHDDStatus( bPrimary );
        if( ( bStatus & HDD_STATUS_ERROR ) == HDD_STATUS_ERROR )
        {
            kPrintf( "Error Occur\n" );
            // ����ȭ ó��
            kUnlock( &( gs_stHDDManager.stMutex ) );
            return i;
        }

        // DATAREQUEST ��Ʈ�� �������� �ʾ����� �����Ͱ� ���ŵǱ� ��ٸ�
        if( ( bStatus & HDD_STATUS_DATAREQUEST ) != HDD_STATUS_DATAREQUEST )
        {
            // ó���� �Ϸ�� ������ ���� �ð� ���� ���ͷ�Ʈ�� ��ٸ�
            bWaitResult = kWaitForHDDInterrupt( bPrimary );
            kSetHDDInterruptFlag( bPrimary, FALSE );
            // ���ͷ�Ʈ�� �߻����� ������ ������ �߻��� ���̹Ƿ� ����
            if( bWaitResult == FALSE )
            {
                kPrintf( "Interrupt Not Occur\n" );
                // ����ȭ ó��
                kUnlock( &( gs_stHDDManager.stMutex ) );
                return FALSE;
            }
        }

        // �� ���͸� ����
        for( j = 0 ; j < 512 / 2 ; j++ )
        {
            ( ( WORD* ) pcBuffer )[ lReadCount++ ]
                    = kInPortWord( wPortBase + HDD_PORT_INDEX_DATA );
        }
    }

    // ����ȭ ó��
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

    // ���� �˻�. �ִ� 256 ���͸� ó���� �� ����
    if( ( gs_stHDDManager.bCanWrite == FALSE ) ||
        ( iSectorCount <= 0 ) || ( 256 < iSectorCount ) ||
        ( ( dwLBA + iSectorCount ) >= gs_stHDDManager.stHDDInformation.dwTotalSectors ) )

    {
        return 0;
    }

    // PATA ��Ʈ�� ���� I/O ��Ʈ�� �⺻ ��巹���� ����
    if( bPrimary == TRUE )
    {
        // ù ��° PATA ��Ʈ�̸� ��Ʈ 0x1F0�� ����
        wPortBase = HDD_PORT_PRIMARYBASE;
    }
    else
    {
        // �� ��° PATA ��Ʈ�̸� ��Ʈ 0x170�� ����
        wPortBase = HDD_PORT_SECONDARYBASE;
    }

    // ���� ���� ���� Ŀ�ǵ尡 �ִٸ� ���� �ð� ���� ���� ������ ���
    if( kWaitForHDDNoBusy( bPrimary ) == FALSE )
    {
        return FALSE;
    }

    // ����ȭ ó��
    kLock( &(gs_stHDDManager.stMutex ) );

    //==========================================================================
    //  ������ �������� ����
    //      LBA ����� ���, ���� ��ȣ -> �Ǹ��� ��ȣ -> ��� ��ȣ�� ������
    //      LBA ��巹���� ����
    //==========================================================================
    // ���� �� ��������(��Ʈ 0x1F2 �Ǵ� 0x172)�� �� ���� ���� ����
    kOutPortByte( wPortBase + HDD_PORT_INDEX_SECTORCOUNT, iSectorCount );
    // ���� ��ȣ ��������(��Ʈ 0x1F3 �Ǵ� 0x173)�� �� ���� ��ġ(LBA 0~7��Ʈ)�� ����
    kOutPortByte( wPortBase + HDD_PORT_INDEX_SECTORNUMBER, dwLBA );
    // �Ǹ��� LSB ��������(��Ʈ 0x1F4 �Ǵ� 0x174)�� �� ���� ��ġ(LBA 8~15��Ʈ)�� ����
    kOutPortByte( wPortBase + HDD_PORT_INDEX_CYLINDERLSB, dwLBA >> 8 );
    // �Ǹ��� MSB ��������(��Ʈ 0x1F5 �Ǵ� 0x175)�� �� ���� ��ġ(LBA 16~23��Ʈ)�� ����
    kOutPortByte( wPortBase + HDD_PORT_INDEX_CYLINDERMSB, dwLBA >> 16 );
    // ����̺�� ��� ������ ����
    if( bMaster == TRUE )
    {
        bDriveFlag = HDD_DRIVEANDHEAD_LBA;
    }
    else
    {
        bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
    }
    // ����̺�/��� ��������(��Ʈ 0x1F6 �Ǵ� 0x176)�� �� ������ ��ġ(LBA 24~27��Ʈ)��
    // ������ ���� ���� ����
    kOutPortByte(wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag | ( (dwLBA
            >> 24 ) & 0x0F ) );

    //==========================================================================
    //  Ŀ�ǵ� ���� ��, ������ �۽��� ������ ������ ���
    //==========================================================================
    // Ŀ�ǵ带 �޾Ƶ��� �غ� �� ������ ���� �ð� ���� ���
    if( kWaitForHDDReady( bPrimary ) == FALSE )
    {
        // ����ȭ ó��
        kUnlock( &( gs_stHDDManager.stMutex ) );
        return FALSE;
    }

    // Ŀ�ǵ� ����
    kOutPortByte( wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_WRITE );

    // ������ �۽��� ������ ������ ���
    while( 1 )
    {
        bStatus = kReadHDDStatus( bPrimary );
        // ������ �߻��ϸ� ����
        if( ( bStatus & HDD_STATUS_ERROR ) == HDD_STATUS_ERROR )
        {
            // ����ȭ ó��
            kUnlock( &(gs_stHDDManager.stMutex ) );
            return 0;
        }

        // Data Request��Ʈ�� �����Ǿ��ٸ� ������ �۽� ����
        if( ( bStatus & HDD_STATUS_DATAREQUEST ) == HDD_STATUS_DATAREQUEST )
        {
            break;
        }

        kSleep( 1 );
    }

    //==========================================================================
    //  ������ �۽� ��, ���ͷ�Ʈ ���
    //==========================================================================
    // ���� ����ŭ ������ ���鼭 ������ �۽�
    for( i = 0 ; i < iSectorCount ; i++)
    {
        // ���ͷ�Ʈ �÷��׸� �ʱ�ȭ�ϰ� �� ���͸� ��
        kSetHDDInterruptFlag( bPrimary, FALSE );
        for( j = 0 ; j < 512 / 2 ; j++ )
        {
            kOutPortWord( wPortBase + HDD_PORT_INDEX_DATA,
                         ( ( WORD* ) pcBuffer )[ lReadCount++ ]);
        }

        // ������ �߻��ϸ� ����
        bStatus = kReadHDDStatus( bPrimary );
        if( ( bStatus & HDD_STATUS_ERROR ) == HDD_STATUS_ERROR )
        {
            // ����ȭ ó��
            kUnlock( &(gs_stHDDManager.stMutex ) );
            return i;
        }

        // DATAREQUEST ��Ʈ�� �������� �ʾ����� �����Ͱ� ó���� �Ϸ�Ǳ� ��ٸ�
        if( ( bStatus & HDD_STATUS_DATAREQUEST ) != HDD_STATUS_DATAREQUEST )
        {
            // ó���� �Ϸ�� ������ ���� �ð� ���� ���ͷ�Ʈ�� ��ٸ�
            bWaitResult = kWaitForHDDInterrupt( bPrimary );
            kSetHDDInterruptFlag( bPrimary, FALSE );
            // ���ͷ�Ʈ�� �߻����� ������ ������ �߻��� ���̹Ƿ� ����
            if( bWaitResult == FALSE )
            {
                // ����ȭ ó��
                kUnlock( &( gs_stHDDManager.stMutex ) );
                return FALSE;
            }
        }
    }
    // ����ȭ ó��
    kUnlock( &(gs_stHDDManager.stMutex ) );
    return i;
}
