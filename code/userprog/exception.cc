// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include <stdlib.h>
#include "copyright.h"
#include "system.h"
#include "syscall.h"

class SpacePC
{
public:
	AddrSpace *space;
	int PC;
};

void execFunction(int name)
{
    char *fileName=new char[256];
    fileName=(char*)name;
    //printf("name: %d\n", name);
    printf("execFunction: fileName: %s\n",fileName);
    OpenFile *openFile=fileSystem->Open(fileName);
    if (openFile==NULL) printf("Cannot open file.\n");
    AddrSpace *space=new AddrSpace(openFile);
    currentThread->space=space;
    //currentThread->fileName=fileName;
    space->InitRegisters();
    space->RestoreState();
    //currentThread->Yield();
    machine->Run();
}

void forkFunction(int pointer)
{
	SpacePC *p=(SpacePC*)pointer;
	AddrSpace *tmp=p->space;
	AddrSpace *space=new AddrSpace(*tmp);
	currentThread->space=space;
	int PC=p->PC;
	machine->WriteRegister(PCReg,PC);
	machine->WriteRegister(NextPCReg,PC+4);
	currentThread->SaveUserState();
	machine->Run();
}

void help()
{
    printf("Instruction:\n");
    printf("x [userprog]: execute user program\n");
    printf("q: quit\n");
    printf("pwd: print current path\n");
    printf("ls: print files and folders in current directory\n");
    printf("h: print help information\n");
    printf("rm [filename/foldername]: delete file or folder\n");
    printf("nw [filename]: create file\n");
    printf("mk [foldername]: create folder\n");
    printf("cd [foldername]: open folder\n");
}

void remove(int base)
{

}
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if ((which == SyscallException) && (type == SC_Halt)) {
	DEBUG('a', "Shutdown, initiated by user program.\n");
   	interrupt->Halt();
    } 
	else if ((which == SyscallException) && (type == SC_Create)) {
        DEBUG('a', "Initiated by user program.\n");
        int base=machine->ReadRegister(4);
        int value;
        int count=0;
        do{
            bool readSuccess=machine->ReadMem(base++,1,&value);
            count++;
            if (!readSuccess) printf("Read Name Failed~\n");
        }while(value!=0);
        base=base-count;
        char fileName[count];
        for (int i=0;i<count;++i){
            machine->ReadMem(base+i,1,&value);
            fileName[i]=(char)value;
        }
        printf("Create: Filename %s.\n", fileName);
        bool success=fileSystem->Create(fileName, 256);
        if (!success)
            printf("Create Failed!\n");
        else
            printf("Create Succeed!\n\n");
        machine->PCAdvanced();
    } 
	else if ((which == SyscallException) && (type == SC_Open)) {
        DEBUG('a', "Opened by user program.\n");
        int base=machine->ReadRegister(4);
        int value;
        int count=0;
        do{
            machine->ReadMem(base++,1,&value);
            count++;
        }while(value!=0);
        base=base-count;
        char fileName[count];
        for (int i=0;i<count;++i){
            machine->ReadMem(base+i,1,&value);
            fileName[i]=(char)value;
        }
        OpenFile *openFile=fileSystem->Open(fileName);
        if (openFile==NULL)
            printf("Open Failed!\n");
        else{
        	printf("Open: openFile: %d, FileName: %s\n", (int)openFile, fileName);
            printf("Open Succeed!\n\n");
            machine->WriteRegister(2,(int)openFile);
        }
        machine->PCAdvanced();
    } 
    else if ((which == SyscallException) && (type == SC_Read)) {
        DEBUG('a', "Read by user program.\n");
        int base=machine->ReadRegister(4);
        int size=machine->ReadRegister(5);
        int fd=machine->ReadRegister(6);
        int value;
        OpenFile *openFile=(OpenFile*)fd;
        if (openFile==NULL)
            printf("Cannot open the file.\n");
        else{
            //printf("Reading...\n");
            char content[size];

            if (fd==1){
                for (int i=0;i<size;++i)
                    content[i]=getchar();
            }
            else{
                openFile->Read(content,size);
                content[size]='\0';
            }         
            //printf("Read: openFile: %d, Content: %s\n", (int)openFile, content);
            for (int i=0;i<size;++i){
                value=(int)content[i];
                machine->WriteMem(base+i,1,value);
            }
            //printf("Read Succeed!\n\n");
        }
        machine->PCAdvanced();
    } 
    else if ((which == SyscallException) && (type == SC_Write)) {
        DEBUG('a', "Writed by user program.\n");
        int base=machine->ReadRegister(4);
        int size=machine->ReadRegister(5);
        int fd=machine->ReadRegister(6);
        int value;

        OpenFile *openFile=(OpenFile*)fd;
        if (openFile==NULL)
            printf("Cannot open the file.\n");
        else {
            //printf("Writing...\n");
            char content[size];
            for (int i=0;i<size;++i){
                machine->ReadMem(base+i,1,&value);
                content[i]=(char)value;
            }
            //printf("Write: openFile: %d, Content: %s\n", (int)openFile, content);
            if (fd==2){
                for (int i=0;i<size;++i)
                    putchar(content[i]);
            }
            else{
                openFile->Write(content,size);
            }
            //printf("Write Succeed!\n\n");
        }      
        machine->PCAdvanced();
    } 
    else if ((which == SyscallException) && (type == SC_Close)) {
        DEBUG('a', "Closed by user program.\n");
        int fd=machine->ReadRegister(4);
        OpenFile *openFile=(OpenFile*)fd;
        printf("Close: openFile %d\n", (int)openFile);
        delete openFile;
        printf("Close Succeed!\n\n");    
        machine->PCAdvanced();
    }
    else if ((which == SyscallException) && (type == SC_Exec)) {
        DEBUG('a', "Executed by user program.\n");
        int base=machine->ReadRegister(4);
        int value;
        int count=0;
        do{
            machine->ReadMem(base++,1,&value);
            count++;
        }while(value!=0);
        base=base-count;
        char *fileName=new char[count];
        for (int i=0;i<count;++i){
            machine->ReadMem(base+i,1,&value);
            fileName[i]=(char)value;
        }
        fileName[count]='\0';

        Thread *t=new Thread("Forked Thread");
        machine->WriteRegister(2,(int)t);
        t->Fork(execFunction, (int)fileName);
        //currentThread->Yield();

        printf("Execute Succeed!\n\n");
        machine->PCAdvanced();
    }
    else if ((which == SyscallException) && (type == SC_Fork)) {
    	SpacePC *p=new SpacePC;
    	p->space=currentThread->space;
    	p->PC=machine->ReadRegister(4);

    	Thread *t=new Thread("Forked Thread");
    	machine->WriteRegister(2,(int)t);

    	t->Fork(forkFunction,(int)p);
    	//currentThread->Yield();

    	printf("Fork Succeed!\n\n");
    	machine->PCAdvanced();
    }
    else if ((which == SyscallException) && (type == SC_Yield)) {
    	machine->PCAdvanced();
    	printf("Yield: Thread Name: %s\n\n", currentThread->getName());
    	currentThread->Yield();
    }
    else if ((which == SyscallException) && (type == SC_Join)) {
        int id=machine->ReadRegister(4);
        printf("Start Join.\n");
        //Thread *t=(Thread*)id;
        //while(t!=NULL){
        machine->PCAdvanced();
        currentThread->Yield();
        //}
        printf("Join Succeed!\n\n");
        //machine->PCAdvanced();
    }
    else if ((which == SyscallException) && (type == SC_Exit)) {
        printf("Exit: Thread Name: %s\n\n", currentThread->getName());
        //if (!strcmp(currentThread->getName(),"main"))
        //	machine->PCAdvanced();
        //else 
        int id=machine->ReadRegister(4);
        if (id==1){
            system("pwd");
            machine->PCAdvanced();
        }
        else if (id==2){
            system("ls");
            machine->PCAdvanced();
        }
        else if (id==3){
            help();
            machine->PCAdvanced();
        }
        else if (id==4){
            int base=machine->ReadRegister(4);
            int count=0;
            int value;
            do{
                machine->ReadMem(base++,1,&value);
                count++;
            }while(value!=0);
            base=base-count;
            char name[count];
            for (int i=0;i<count;++i){
                machine->ReadMem(base+i,1,&value);
                name[i]=(char)value;
            }
            printf("Remove Name %s.\n", name);
            fileSystem->Remove(name);
            machine->PCAdvanced();
        }
        else if (id==5){
            //int base=machine->ReadRegister(4);
        }
        else 
            currentThread->Finish();

        //printf("Exit Succeed!\n\n");
        //machine->PCAdvanced();
    }
    else if ((which == SyscallException) && (type == SC_Pwd)) {
        system("pwd");
        machine->PCAdvanced();
    }
    else if ((which == SyscallException) && (type == SC_Ls)) {
        
    }

    else {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}
