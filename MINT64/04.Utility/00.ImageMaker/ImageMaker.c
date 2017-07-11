#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<io.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<errno.h>
#include<string.h>

#define BYTESOFSECTOR 512

int AdjustInSectorSize(int iFd, int iSourceSize);
void WriteKernelInformation(int iTargetFd, int iTotalKernelSectorCount,int iKernel32SectorCount);
int CopyFile(int iSourceFd, int iTargetFd);

int main(int argc, char* argv[]) {
	int iSourceFd;
	int iTargetFd;
	int iBootLoaderSize;
	int iKernel32SectorCount;
	int iKernel64SectorCount;
	int iSourceSize;

	if (argc<4) {
		fprintf(stderr, "Error\n");
		exit(-1);
	}
	//Source File open
	if((iTargetFd = open("Disk.img",O_RDWR | O_CREAT | O_TRUNC | O_BINARY
			,S_IREAD | S_IWRITE ))==-1) {
		fprintf(stderr,"Disk image open fail\n");
		exit(-1);
	}
	//Target file open
	printf("Copy Boot loader to image file\n");
	if((iSourceFd = open(argv[1], O_RDONLY | O_BINARY))==-1) {
		fprintf(stderr, "open fail\n");
		exit(-1);
	}
	//BootLoader를 Target에 복사를 한다
	iSourceSize = CopyFile(iSourceFd, iTargetFd);
	close(iSourceFd);

	//섹터는 512바이트 단위이므로 부족한 부분을 0으로 채워넣어 512 단위로 맞춤
	iBootLoaderSize = AdjustInSectorSize(iTargetFd, iSourceSize);
	printf("%s size = %d and sector count = %d\n",argv[1],iSourceSize,iBootLoaderSize);

	//kernel32 파일을 이미지파일에 복사
	printf("Copy protected mode kernel to image file\n");
	if ((iSourceFd = open(argv[2], O_RDONLY | O_BINARY))==-1) {
		fprintf(stderr, "%s open fail\n",argv[2]);
		exit(-1);
	}
	iSourceSize = CopyFile(iSourceFd, iTargetFd);
	close(iSourceFd);

	//512 단위로 맞춤. 빈 부분을 0으로 채워넣음
	iKernel32SectorCount = AdjustInSectorSize(iTargetFd, iSourceSize);
	printf("%s size = %d and sector count = %d\n",argv[2], iSourceSize, iKernel32SectorCount);

	//IA-32e
	printf("Copy IA-32e mode kernel to image file\n");
	if((iSourceFd = open(argv[3], O_RDONLY | O_BINARY))==-1) {
		fprintf(stderr, "open fail\n");
		exit(-1);
	}
	iSourceSize = CopyFile(iSourceFd, iTargetFd);
	close(iSourceFd);

	iKernel64SectorCount = AdjustInSectorSize(iTargetFd, iSourceSize);
	printf("%s size = %d and sector count = %d\n",argv[3], iSourceSize, iKernel64SectorCount);


	//디스크 이미지에 커널 정보 갱신
	printf("Start to write kernel information\n");
	WriteKernelInformation(iTargetFd, iKernel32SectorCount + iKernel64SectorCount, iKernel32SectorCount);
	printf("Image file create complete\n");
	close(iTargetFd);
	return 0;
}

int AdjustInSectorSize(int iFd, int iSourceSize) {
	int i;
	int iAdjustSizeToSector;
	char cCh;
	int iSectorCount;
	iAdjustSizeToSector = iSourceSize % BYTESOFSECTOR;
	cCh=0x00;
	if (iAdjustSizeToSector !=0) {
		iAdjustSizeToSector = 512 - iAdjustSizeToSector;
		printf("file size %lu and fill %u byte\n",iSourceSize, iAdjustSizeToSector);
		for (i=0; i<iAdjustSizeToSector; i++) {
			write(iFd,&cCh, 1);
		}
	}
	else {
		printf("File size is aligned 512 byte");
	}
	iSectorCount = (iSourceSize + iAdjustSizeToSector)/BYTESOFSECTOR;
	return iSectorCount;
}

void WriteKernelInformation(int iTargetFd, int iTotalKernelSectorCount, int iKernel32SectorCount) {
	unsigned short usData;
	long lPosition;
	unsigned short value;
	lPosition = lseek(iTargetFd, (off_t)5, SEEK_SET);
	if(lPosition == -1) {
		fprintf(stderr,"lseek fail. return value = %d, errno = %d, %d\n",lPosition, errno,SEEK_SET);
		exit(-1);
	}

	usData = (unsigned short)iTotalKernelSectorCount;
	write(iTargetFd, &usData,2);
	usData = (unsigned short)iKernel32SectorCount;
	write(iTargetFd, &usData,2);

	lPosition = lseek(iTargetFd, (off_t)7, SEEK_SET);
	read(iTargetFd, &value, 2);
	if (value!=usData)
		printf("not match! value is %d\n",value);
	else
		printf("match!!\n");
	printf("sector count except boot loader %d\n",iTotalKernelSectorCount);
	printf("sector count of protected mode kernel %d\n",iKernel32SectorCount);

}

int CopyFile(int iSourceFd, int iTargetFd) {
	int iSourceFileSize;
	int iRead;
	int iWrite;
	char vcBuffer[BYTESOFSECTOR];
	iSourceFileSize = 0;
	while (1) {
		iRead = read(iSourceFd, vcBuffer, sizeof(vcBuffer));
		iWrite = write(iTargetFd, vcBuffer, iRead);

		if(iRead!=iWrite) {
			fprintf(stderr, "iRead !=iWrite...\n");
			exit(-1);
		}
		iSourceFileSize+=iRead;
		if (iRead !=sizeof(vcBuffer)) {
			break;
		}
	}
	return iSourceFileSize;
}

