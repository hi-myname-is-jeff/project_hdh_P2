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
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "syscall.h"
#include "ksyscall.h"
#include "kernel.h"
#include "filesys.h"
#include <iostream>
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
// If you are handling a system call, don't forget to increment the pc
// before returning. (Or else you'll loop making the same system call forever!)
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	is in machine.h.
//----------------------------------------------------------------------

void IncreasePC()
{
/* set previous programm counter (debugging only)*/
kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

/* set programm counter to next instruction (all Instructions are 4 byte wide)*/
kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);
			
/* set next programm counter for brach execution */
kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg)+4);
}

int System2User(int virtAddr,int len,char* buffer)
{
	if (len < 0) return -1;
	if (len == 0)return len;
	int i = 0;
	int oneChar = 0 ;
	do	{
		oneChar= (int) buffer[i];
		kernel->machine->WriteMem(virtAddr+i,1,oneChar);
		i ++;
		}
	while(i < len && oneChar != 0);
		return i;
}

char* User2System(int virtAddr,int limit)
{
	int i;// index
	int oneChar;
	char* kernelBuf = NULL;
	kernelBuf = new char[limit +1];//need for terminal string
	if (kernelBuf == NULL)
	return kernelBuf;
	memset(kernelBuf,0,limit+1);
	//printf("\n Filename u2s:");
	for (i = 0 ; i < limit ;i++)
	{
		kernel->machine->ReadMem(virtAddr+i,1,&oneChar);
		kernelBuf[i] = (char)oneChar;
		//printf("%c",kernelBuf[i]);
		if (oneChar == 0)
		break;
	}
	return kernelBuf;
}

void
ExceptionHandler(ExceptionType which)
{
    int type = kernel->machine->ReadRegister(2);

    DEBUG(dbgSys, "Received Exception " << which << " type: " << type << "\n");
	cerr << "Received Exception " << which << " type: " << type << "\n";

    switch (which) 
	{
		case SyscallException:
		{
			switch(type)
			{
				case SC_Create:
				{
					int address, success;
					char* filename;
					address = kernel->machine->ReadRegister(4);

					filename = User2System(address, MaxFileLength + 1);
					success = SysCreate(filename);
					kernel->machine->WriteRegister(2, success);

					delete filename;
					IncreasePC();
					break;
				}
				case SC_Halt:
				{
					DEBUG(dbgSys, "Shutdown, initiated by user program.\n");
					SysHalt();
					ASSERTNOTREACHED();
					break;
				}
				case SC_Add:
				{
					cerr << "Add " << kernel->machine->ReadRegister(4) << " + " << kernel->machine->ReadRegister(5) << "\n";
					
					/* Process SysAdd Systemcall*/
					int result;
					result = SysAdd(/* int op1 */(int)kernel->machine->ReadRegister(4),
							/* int op2 */(int)kernel->machine->ReadRegister(5));

					DEBUG(dbgSys, "Add returning with " << result << "\n");
					/* Prepare Result */
					kernel->machine->WriteRegister(2, (int)result);
					
					/* Modify return point */
					IncreasePC();
					return;
					
					ASSERTNOTREACHED();

					break;
				}
				case SC_Open:
				{
					//OpenFileID Open(char *name, int type)
					int virtAddr = machine->ReadRegister(4); // Lay dia chi cua tham so name tu thanh ghi so 4
					int type = machine->ReadRegister(5); // Lay tham so type tu thanh ghi so 5
					char* filename;
					filename = User2System(virtAddr, MaxFileLength); // Copy chuoi tu vung nho User Space sang System Space voi bo dem name dai MaxFileLength
					//Kiem tra xem OS con mo dc file khong
					
					// update 4/1/2018
					int freeSlot = FileSystem->FindFreeSlot();
					if (freeSlot != -1) //Chi xu li khi con slot trong
					{
						if (type == 0 || type == 1) //chi xu li khi type = 0 hoac 1
						{
							if ((FileSystem->openf[freeSlot] = FileSystem->Open(filename, type)) != NULL) //Mo file thanh cong
							{
								machine->WriteRegister(2, freeSlot); //tra ve OpenFileID
							}
						}
						else if (type == 2) // xu li stdin voi type quy uoc la 2
						{
							machine->WriteRegister(2, 0); //tra ve OpenFileID
						}
						else // xu li stdout voi type quy uoc la 3
						{
							machine->WriteRegister(2, 1); //tra ve OpenFileID
						}
						delete[] filename;
						break;
					}
					machine->WriteRegister(2, -1); //Khong mo duoc file return -1
					IncreasePC();
					delete[] filename;
					break;
				}
			}
			
			break;
		}
		default:
		{
			cerr << "Unexpected user mode exception" << (int)which << "\n";
			ASSERTNOTREACHED();
			break;
		}
	}
}