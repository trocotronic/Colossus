/*
 * $Id: debug.c,v 1.1 2004-12-31 19:25:52 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#include <dbghelp.h>
#define BUFFERSIZE   0x200

__inline char *StackTrace(EXCEPTION_POINTERS *e) 
{
	static char buffer[5000];
	char curmodule[32];
	DWORD symOptions, dwDisp, frame;
	HANDLE hProcess = GetCurrentProcess();
	IMAGEHLP_SYMBOL *pSym = malloc(sizeof(IMAGEHLP_SYMBOL)+500);
	IMAGEHLP_LINE pLine;
	IMAGEHLP_MODULE pMod;
	STACKFRAME Stack;
	Stack.AddrPC.Offset = e->ContextRecord->Eip;
	Stack.AddrPC.Mode = AddrModeFlat;
	Stack.AddrFrame.Offset = e->ContextRecord->Ebp;
	Stack.AddrFrame.Mode = AddrModeFlat;
	if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		hProcess = (HANDLE)GetCurrentProcessId();
	else
		hProcess = GetCurrentProcess();	
	SymInitialize(hProcess, NULL, TRUE);
	SymSetOptions(SYMOPT_LOAD_LINES|SYMOPT_UNDNAME);
	bzero(pSym, sizeof(IMAGEHLP_SYMBOL)+500);
	pSym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
	pSym->MaxNameLength = 500;
	bzero(&pLine, sizeof(IMAGEHLP_LINE));
	pLine.SizeOfStruct = sizeof(IMAGEHLP_LINE);
	bzero(&pMod, sizeof(IMAGEHLP_MODULE));
	pMod.SizeOfStruct = sizeof(IMAGEHLP_MODULE);
	SymGetModuleInfo(hProcess, Stack.AddrPC.Offset, &pMod);
	strcpy(curmodule, pMod.ModuleName);
	sprintf(buffer, "\tModulo: %s\n", pMod.ModuleName);
	for (frame = 0; ; frame++) 
	{
		char buf[500];
		if (!StackWalk(IMAGE_FILE_MACHINE_I386, GetCurrentProcess(), GetCurrentThread(),
			&Stack, NULL, NULL, SymFunctionTableAccess, SymGetModuleBase, NULL))
			break;
		SymGetModuleInfo(hProcess, Stack.AddrPC.Offset, &pMod);
		if (strcmp(curmodule, pMod.ModuleName)) 
		{
			strcpy(curmodule, pMod.ModuleName);
			sprintf(buf, "\tModulo: %s\n", pMod.ModuleName);
			strcat(buffer, buf);
		}
		SymGetLineFromAddr(hProcess, Stack.AddrPC.Offset, &dwDisp, &pLine);
		SymGetSymFromAddr(hProcess, Stack.AddrPC.Offset, &dwDisp, pSym);
		sprintf(buf, "\t\t#%d %s:%d: %s\n", frame, pLine.FileName, pLine.LineNumber, 
		        pSym->Name);
		strcat(buffer, buf);
	}
	return buffer;

}
__inline char *GetRegisters(CONTEXT *context) 
{
	static char buffer[1024];
	sprintf(buffer, "\tEAX=0x%08x EBX=0x%08x ECX=0x%08x\n"
			"\tEDX=0x%08x ESI=0x%08x EDI=0x%08x\n"
			"\tEIP=0x%08x EBP=0x%08x ESP=0x%08x\n",
			context->Eax, context->Ebx, context->Ecx, context->Edx,
			context->Esi, context->Edi, context->Eip, context->Ebp,
			context->Esp);
	return buffer;
}
__inline char *GetException(DWORD code) 
{
	switch (code) 
	{
		case EXCEPTION_ACCESS_VIOLATION:
			return "Access Violation";
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
			return "Array Bounds Exceeded";
		case EXCEPTION_BREAKPOINT:
			return "Breakpoint";
		case EXCEPTION_DATATYPE_MISALIGNMENT:
			return "Datatype Misalignment";
		case EXCEPTION_FLT_DENORMAL_OPERAND:
			return "Floating Point Denormal Operand";
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:
			return "Floating Point Division By Zero";
		case EXCEPTION_FLT_INEXACT_RESULT:
			return "Floating Point Inexact Result";
		case EXCEPTION_FLT_INVALID_OPERATION:
			return "Floating Point Invalid Operation";
		case EXCEPTION_FLT_OVERFLOW:
			return "Floating Point Overflow";
		case EXCEPTION_FLT_STACK_CHECK:
			return "Floating Point Stack Overflow";
		case EXCEPTION_FLT_UNDERFLOW:
			return "Floating Point Underflow";
		case EXCEPTION_ILLEGAL_INSTRUCTION:
			return "Illegal Instruction";
		case EXCEPTION_IN_PAGE_ERROR:
			return "In Page Error";
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
			return "Integer Division By Zero";
		case EXCEPTION_INT_OVERFLOW:
			return "Integer Overflow";
		case EXCEPTION_INVALID_DISPOSITION:
			return "Invalid Disposition";
		case EXCEPTION_NONCONTINUABLE_EXCEPTION:
			return "Noncontinuable Exception";
		case EXCEPTION_PRIV_INSTRUCTION:
			return "Unallowed Instruction";
		case EXCEPTION_SINGLE_STEP:
			return "Single Step";
		case EXCEPTION_STACK_OVERFLOW:
			return "Stack Overflow";
	}
	return "Unknown Exception";
}
LONG __stdcall ExceptionFilter(EXCEPTION_POINTERS *e) 
{
	MEMORYSTATUS memStats;
	char file[512], text[1024];
	FILE *fd;
	time_t timet = time(NULL);
	sprintf(file, "colossus.%d.core", getpid());
	fd = fopen(file, "w");
	GlobalMemoryStatus(&memStats);
	fprintf(fd, "Generado el %s\n%s (%d.%d.%d)\n%s\n-----------------\nMemoria:\n"
	"\tFísica: (Disponible:%ldMB/Total:%ldMB)\n\tVirtual: (Disponible:%ldMB/Total:%ldMB)\n"
	"-----------------\nExcepciones:\n\t%s\n-----------------\nBuffer:\n\t%s\n"
	"-----------------\nRegistros:\n%s-----------------\nStack Trace:\n%s", asctime(gmtime(&timet)),
	SO, VerInfo.dwMajorVersion, VerInfo.dwMinorVersion, VerInfo.dwBuildNumber, 
	COLOSSUS_VERSION,
	memStats.dwAvailPhys/1048576, memStats.dwTotalPhys/1048576, 
	memStats.dwAvailVirtual/1048576, memStats.dwTotalVirtual/1048576, 
	GetException(e->ExceptionRecord->ExceptionCode), backupbuf, 
	GetRegisters(e->ContextRecord), StackTrace(e));
	sprintf(text, "Se ha producido un fallo grave y el programa se ha cerrado. Se ha escrito la información en"
		" %s", file);
	fclose(fd);
	MessageBox(NULL, text, "Error fatal", MB_OK);
	CleanUp();
	return EXCEPTION_EXECUTE_HANDLER;
}
void InitDebug(void) 
{
	SetUnhandledExceptionFilter(&ExceptionFilter);
}
