#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include "Types.h"
#include "Synchronization.h"
#include "HardDisk.h"

// MINT ���� �ý��� �ñ׳�ó(Signature)
#define FILESYSTEM_SIGNATURE                0x7E38CF10
// Ŭ�������� ũ��(���� ��), 4Kbyte
#define FILESYSTEM_SECTORSPERCLUSTER        8
// ���� Ŭ�������� ������ ǥ��
#define FILESYSTEM_LASTCLUSTER              0xFFFFFFFF
// �� Ŭ������ ǥ��
#define FILESYSTEM_FREECLUSTER              0x00
// ��Ʈ ���͸��� �ִ� �ִ� ���͸� ��Ʈ���� ��
#define FILESYSTEM_MAXDIRECTORYENTRYCOUNT   ( ( FILESYSTEM_SECTORSPERCLUSTER * 512 ) / \
        sizeof( DIRECTORYENTRY ) )
// Ŭ�������� ũ��(����Ʈ ��)
#define FILESYSTEM_CLUSTERSIZE              ( FILESYSTEM_SECTORSPERCLUSTER * 512 )

// ���� �̸��� �ִ� ����
#define FILESYSTEM_MAXFILENAMELENGTH        24

// �ڵ��� Ÿ���� ����
#define FILESYSTEM_TYPE_FREE                0
#define FILESYSTEM_TYPE_FILE                1
#define FILESYSTEM_TYPE_DIRECTORY           2

// SEEK �ɼ� ����
#define FILESYSTEM_SEEK_SET                 0
#define FILESYSTEM_SEEK_CUR                 1
#define FILESYSTEM_SEEK_END                 2

// MINT ���� �ý��� �Լ��� ǥ�� ����� �Լ� �̸����� ������
#define fopen       kOpenFile
#define fread       kReadFile
#define fwrite      kWriteFile
#define fseek       kSeekFile
#define fclose      kCloseFile
#define remove      kRemoveFile
#define opendir     kOpenDirectory
#define readdir     kReadDirectory
#define rewinddir   kRewindDirectory
#define closedir    kCloseDirectory

// MINT ���� �ý��� ��ũ�θ� ǥ�� ������� ��ũ�θ� ������
#define SEEK_SET    FILESYSTEM_SEEK_SET
#define SEEK_CUR    FILESYSTEM_SEEK_CUR
#define SEEK_END    FILESYSTEM_SEEK_END

// MINT ���� �ý��� Ÿ�԰� �ʵ带 ǥ�� ������� Ÿ������ ������
#define size_t      DWORD
#define dirent      kDirectoryEntryStruct
#define d_name      vcFileName


// �ڵ��� �ִ� ����, �ִ� �½�ũ ���� 3��� ����
#define FILESYSTEM_HANDLE_MAXCOUNT          ( TASK_MAXCOUNT * 3 )



// �ϵ� ��ũ ��� ���õ� �Լ� ������ Ÿ�� ����
typedef BOOL (* fReadHDDInformation ) ( BOOL bPrimary, BOOL bMaster,
        HDDINFORMATION* pstHDDInformation );
typedef int (* fReadHDDSector ) ( BOOL bPrimary, BOOL bMaster, DWORD dwLBA,
        int iSectorCount, char* pcBuffer );
typedef int (* fWriteHDDSector ) ( BOOL bPrimary, BOOL bMaster, DWORD dwLBA,
        int iSectorCount, char* pcBuffer );

// 1����Ʈ�� ����
#pragma pack( push, 1 )

// ��Ƽ�� �ڷᱸ��
typedef struct kPartitionStruct
{
    // ���� ���� �÷���. 0x80�̸� ���� ������ ��Ÿ���� 0x00�� ���� �Ұ�
    BYTE bBootableFlag;
    // ��Ƽ���� ���� ��巹��. ����� ���� ������� ������ �Ʒ��� LBA ��巹���� ��� ���
    BYTE vbStartingCHSAddress[ 3 ];
    // ��Ƽ�� Ÿ��
    BYTE bPartitionType;
    // ��Ƽ���� ������ ��巹��. ����� ���� ��� �� ��
    BYTE vbEndingCHSAddress[ 3 ];
    // ��Ƽ���� ���� ��巹��. LBA ��巹���� ��Ÿ�� ��
    DWORD dwStartingLBAAddress;
    // ��Ƽ�ǿ� ���Ե� ���� ��
    DWORD dwSizeInSector;
} PARTITION;


// MBR �ڷᱸ��
typedef struct kMBRStruct
{
    // ��Ʈ �δ� �ڵ尡 ��ġ�ϴ� ����
    BYTE vbBootCode[ 430 ];

    // ���� �ý��� �ñ׳�ó, 0x7E38CF10
    DWORD dwSignature;
    // ����� ������ ���� ��
    DWORD dwReservedSectorCount;
    // Ŭ������ ��ũ ���̺� ������ ���� ��
    DWORD dwClusterLinkSectorCount;
    // Ŭ�������� ��ü ����
    DWORD dwTotalClusterCount;

    // ��Ƽ�� ���̺�
    PARTITION vstPartition[ 4 ];

    // ��Ʈ �δ� �ñ׳�ó, 0x55, 0xAA
    BYTE vbBootLoaderSignature[ 2 ];
} MBR;


// ���͸� ��Ʈ�� �ڷᱸ��
typedef struct kDirectoryEntryStruct
{
    // ���� �̸�
    char vcFileName[ FILESYSTEM_MAXFILENAMELENGTH ];
    // ������ ���� ũ��
    DWORD dwFileSize;
    // ������ �����ϴ� Ŭ������ �ε���
    DWORD dwStartClusterIndex;
} DIRECTORYENTRY;

#pragma pack( pop )



// ������ �����ϴ� ���� �ڵ� �ڷᱸ��
typedef struct kFileHandleStruct
{
    // ������ �����ϴ� ���͸� ��Ʈ���� ������
    int iDirectoryEntryOffset;
    // ���� ũ��
    DWORD dwFileSize;
    // ������ ���� Ŭ������ �ε���
    DWORD dwStartClusterIndex;
    // ���� I/O�� �������� Ŭ�������� �ε���
    DWORD dwCurrentClusterIndex;
    // ���� Ŭ�������� �ٷ� ���� Ŭ�������� �ε���
    DWORD dwPreviousClusterIndex;
    // ���� �������� ���� ��ġ
    DWORD dwCurrentOffset;
} FILEHANDLE;

// ���͸��� �����ϴ� ���͸� �ڵ� �ڷᱸ��
typedef struct kDirectoryHandleStruct
{
    // ��Ʈ ���͸��� �����ص� ����
    DIRECTORYENTRY* pstDirectoryBuffer;

    // ���͸� �������� ���� ��ġ
    int iCurrentOffset;
} DIRECTORYHANDLE;

// ���ϰ� ���͸��� ���� ������ ����ִ� �ڷᱸ��
typedef struct kFileDirectoryHandleStruct
{
    // �ڷᱸ���� Ÿ�� ����. ���� �ڵ��̳� ���͸� �ڵ�, �Ǵ� �� �ڵ� Ÿ�� ���� ����
    BYTE bType;

    // bType�� ���� ���� ���� �Ǵ� ���͸��� ���
    union
    {
        // ���� �ڵ�
        FILEHANDLE stFileHandle;
        // ���͸� �ڵ�
        DIRECTORYHANDLE stDirectoryHandle;
    };
} FILE, DIR;

// ���� �ý����� �����ϴ� ����ü
typedef struct kFileSystemManagerStruct
{
    // ���� �ý����� ���������� �νĵǾ����� ����
    BOOL bMounted;

    // �� ������ ���� ���� ���� LBA ��巹��
    DWORD dwReservedSectorCount;
    DWORD dwClusterLinkAreaStartAddress;
    DWORD dwClusterLinkAreaSize;
    DWORD dwDataAreaStartAddress;
    // ������ ������ Ŭ�������� �� ����
    DWORD dwTotalClusterCount;

    // ���������� Ŭ�����͸� �Ҵ��� Ŭ������ ��ũ ���̺��� ���� �������� ����
    DWORD dwLastAllocatedClusterLinkSectorOffset;

    // ���� �ý��� ����ȭ ��ü
    MUTEX stMutex;

    //�ڵ� Ǯ�� ��巹��
    FILE* pstHandlePool;
} FILESYSTEMMANAGER;




BOOL kInitializeFileSystem( void );
BOOL kFormat( void );
BOOL kMount( void );
BOOL kGetHDDInformation( HDDINFORMATION* pstInformation);

//  ������ �Լ�(Low Level Function)BOOL kReadClusterLinkTable( DWORD dwOffset, BYTE* pbBuffer );
BOOL kWriteClusterLinkTable( DWORD dwOffset, BYTE* pbBuffer );
BOOL kReadCluster( DWORD dwOffset, BYTE* pbBuffer );
BOOL kWriteCluster( DWORD dwOffset, BYTE* pbBuffer );
DWORD kFindFreeCluster( void );
BOOL kSetClusterLinkData( DWORD dwClusterIndex, DWORD dwData );
BOOL kGetClusterLinkData( DWORD dwClusterIndex, DWORD* pdwData );
int kFindFreeDirectoryEntry( void );
BOOL kSetDirectoryEntryData( int iIndex, DIRECTORYENTRY* pstEntry );
BOOL kGetDirectoryEntryData( int iIndex, DIRECTORYENTRY* pstEntry );
int kFindDirectoryEntry( const char* pcFileName, DIRECTORYENTRY* pstEntry );
void kGetFileSystemInformation( FILESYSTEMMANAGER* pstManager );

//����� �Լ�
FILE* kOpenFile( const char* pcFileName, const char* pcMode );
DWORD kReadFile( void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile );
DWORD kWriteFile( const void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile );
int kSeekFile( FILE* pstFile, int iOffset, int iOrigin );
int kCloseFile( FILE* pstFile );
int kRemoveFile( const char* pcFileName );
DIR* kOpenDirectory( const char* pcDirectoryName );
struct kDirectoryEntryStruct* kReadDirectory( DIR* pstDirectory );
void kRewindDirectory( DIR* pstDirectory );
int kCloseDirectory( DIR* pstDirectory );
BOOL kWriteZero( FILE* pstFile, DWORD dwCount );
BOOL kIsFileOpened( const DIRECTORYENTRY* pstEntry );

static void* kAllocateFileDirectoryHandle( void );
static void kFreeFileDirectoryHandle( FILE* pstFile );
static BOOL kCreateFile( const char* pcFileName, DIRECTORYENTRY* pstEntry,
        int* piDirectoryEntryIndex );
static BOOL kFreeClusterUntilEnd( DWORD dwClusterIndex );
static BOOL kUpdateDirectoryEntry( FILEHANDLE* pstFileHandle );


#endif /*__FILESYSTEM_H__*/
