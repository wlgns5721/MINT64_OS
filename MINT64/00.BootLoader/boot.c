#include<stdio.h>

int main() {
	int i=0;
	char* pcVideoMemory = (char*)0xb8000;

	while(1) {
		pcVideoMemory[i] = 0;
		pcVideoMemory[i+1] = 0x0a;
		i+=2;
		if (i>=80*25*2)
			break;
	}

	return 1;
}
