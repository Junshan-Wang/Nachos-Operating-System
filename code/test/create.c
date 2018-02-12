#include "syscall.h"
int main()
{
	//int fd1, fd2;
	//char buffer1[4], buffer2[4];
	//buffer1[0]='W'; buffer1[1]='J'; buffer1[2]='S'; buffer1[3]='\0';
	Create("wjs.txt");
	//fd1=Open("wjs.txt");	
	//Write(buffer1,4,fd1);
	//fd2=Open("wjs.txt");
	//Read(buffer2,4,fd2);
	//Close(fd1);
	//Close(fd2);
	//fd3=Open("wjs.txt");
	//Close(fd3);
	//Halt();
	Exit(0);
}