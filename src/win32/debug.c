/*
 * $Id: debug.c,v 1.9 2006-02-17 19:19:03 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#include <dbghelp.h>
#define BUFFERSIZE   0x200

BOOL CALLBACK SymEnumerateModulesProc64(PCSTR ModuleName, DWORD64 BaseOfDll, PVOID UserContext)
{
    SymUnloadModule((HANDLE)UserContext, (DWORD)BaseOfDll);
    return TRUE;
}
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
	if (!SymGetModuleInfo(hProcess, Stack.AddrPC.Offset, &pMod))
	{
		sprintf(buf, "\tFalla SymGetModuleInfo (%i)\n", GetLastError());
		strcat(buffer, buf);
	}
	strcpy(curmodule, pMod.ModuleName);
	sprintf(buffer, "\tMódulo: %s (%s)\n", pMod.ModuleName, pMod.LoadedImageName);
	for (frame = 0; ; frame++) 
	{
		char buf[500];
		if (!StackWalk(IMAGE_FILE_MACHINE_I386, GetCurrentProcess(), GetCurrentThread(),
			&Stack, NULL, NULL, SymFunctionTableAccess, SymGetModuleBase, NULL))
			break;
		if (!SymGetModuleInfo(hProcess, Stack.AddrPC.Offset, &pMod))
		{
			sprintf(buf, "\tFalla SymGetModuleInfo (%i)\n", GetLastError());
			strcat(buffer, buf);
		}
		if (strcmp(curmodule, pMod.ModuleName)) 
		{
			strcpy(curmodule, pMod.ModuleName);
			sprintf(buf, "\tMódulo: %s (%s)\n", pMod.ModuleName, pMod.LoadedImageName);
			strcat(buffer, buf);
		}
		if (!SymGetLineFromAddr(hProcess, Stack.AddrPC.Offset, &dwDisp, &pLine))
		{
			sprintf(buf, "\t\tFalla SymGetLineFromAddr (%i)\n", GetLastError());
			strcat(buffer, buf);
		}
		if (!SymGetSymFromAddr(hProcess, Stack.AddrPC.Offset, &dwDisp, pSym))
		{
			sprintf(buf, "\t\tFalla SymGetSymFromAddr (%i)\n", GetLastError());
			strcat(buffer, buf);
		}
		sprintf(buf, "\t\t#%d %s:%d: %s\n", frame, pLine.FileName, pLine.LineNumber, pSym->Name);
		strcat(buffer, buf);
		pLine.FileName = NULL;
		pLine.LineNumber = 0;
	}
	SymEnumerateModules64(hProcess, SymEnumerateModulesProc64, hProcess);
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
int GetLogicalAddress(PVOID addr, PTSTR szModule, DWORD len, DWORD *section, DWORD *offset)
{
	MEMORY_BASIC_INFORMATION mbi;
	DWORD hMod, rva;
	PIMAGE_DOS_HEADER pDosHdr;
	PIMAGE_NT_HEADERS pNtHdr;
	PIMAGE_SECTION_HEADER pSection;
	unsigned int i;
    	if ( !VirtualQuery( addr, &mbi, sizeof(mbi) ) )
        	return 0;
        hMod = (DWORD)mbi.AllocationBase;
    	if ( !GetModuleFileName( (HMODULE)hMod, szModule, len ) )
        	return 0;
    	pDosHdr = (PIMAGE_DOS_HEADER)hMod;
	pNtHdr = (PIMAGE_NT_HEADERS)(hMod + pDosHdr->e_lfanew);
    	pSection = IMAGE_FIRST_SECTION( pNtHdr );
    	rva = (DWORD)addr - hMod; // RVA is offset from module load address
    	for (i = 0; i < pNtHdr->FileHeader.NumberOfSections; i++, pSection++ )
    	{
        	DWORD sectionStart = pSection->VirtualAddress;
        	DWORD sectionEnd = sectionStart + max(pSection->SizeOfRawData, pSection->Misc.VirtualSize);
		if ( (rva >= sectionStart) && (rva <= sectionEnd) )
        	{
            		// Yes, address is in the section.  Calculate section and offset,
            		// and store in the "section" & "offset" params, which were
            		// passed by reference.
            		*section = i+1;
            		*offset = rva - sectionStart;
            		return 1;
        	}
    	}
    	return 0;
}
__inline char *MyStackWalk(PCONTEXT pContext)
{
	DWORD pc = pContext->Eip;
	PDWORD pFrame, pPrevFrame;
	static char buffer[5000];
        pFrame = (PDWORD)pContext->Ebp;
        do
    	{
        	TCHAR szModule[MAX_PATH];
        	DWORD section = 0, offset = 0;
        	if (!GetLogicalAddress((PVOID)pc, szModule,sizeof(szModule),&section,&offset ))
        		break;
		sprintf(buf, "\t%08X  %08X  %04X:%08X %s\n", pc, pFrame, section, offset, szModule );
		strcat(buffer, buf);
	        pc = pFrame[1];
	        pPrevFrame = pFrame;
	        pFrame = (PDWORD)pFrame[0]; // precede to next higher frame on stack
	        if ( (DWORD)pFrame & 3 )    // Frame pointer must be aligned on a
        		break;                  // DWORD boundary.  Bail if not so.
	        if ( pFrame <= pPrevFrame )
        		break;
	        // Can two DWORDs be read from the supposed frame address?          
        	if ( IsBadWritePtr(pFrame, sizeof(PVOID)*2) )
            		break;
    	} while ( 1 );
    	return buffer;
}   
LONG __stdcall ExceptionFilter(EXCEPTION_POINTERS *e) 
{
	MEMORYSTATUS memStats;
	char file[512], text[1024];
	FILE *fd;
	time_t timet = time(NULL);
	WIN32_FIND_DATA FindFileData;
 	HANDLE hFind = INVALID_HANDLE_VALUE;
 	hFind = FindFirstFile("tmp\\*", &FindFileData);
 	while (FindNextFile(hFind, &FindFileData) != 0) 
      	{
      		char *pdb;
      		if ((pdb = strrchr(FindFileData.cFileName, '.')) && !strcmp(pdb+1, "pdb"))
      		{
      			sprintf(buf, "tmp/%s", FindFileData.cFileName);
      			CopyFile(buf, FindFileData.cFileName, 0);
      		}
      	}
      	FindClose(hFind);
	sprintf(file, "colossus.%d.core", getpid());
	fd = fopen(file, "w");
	GlobalMemoryStatus(&memStats);
	fprintf(fd, "Generado el %s\n%s (%d.%d.%d)\n%s\n-----------------\nMemoria:\n"
	"\tFísica: (Disponible: %ldMB / Total: %ldMB)\n\tVirtual: (Disponible: %ld MB / Total: %ldMB)\n"
	"-----------------\nExcepción:\n\t%s\n-----------------\nBuffer:\n\t%s\n"
	"-----------------\nRegistros:\n%s-----------------\nStack Trace:\n%s", asctime(gmtime(&timet)),
	SO, VerInfo.dwMajorVersion, VerInfo.dwMinorVersion, VerInfo.dwBuildNumber, 
	COLOSSUS_VERSION,
	memStats.dwAvailPhys/1048576, memStats.dwTotalPhys/1048576, 
	memStats.dwAvailVirtual/1048576, memStats.dwTotalVirtual/1048576, 
	GetException(e->ExceptionRecord->ExceptionCode), backupbuf, 
	GetRegisters(e->ContextRecord), StackTrace(e));
	fprintf(fd, "-----------------\nStack Walk:\n%s", MyStackWalk(e->ContextRecord));
	sprintf(text, "Se ha producido un fallo grave y el programa se ha cerrado. Se ha escrito la información en"
		" %s", file);
	fclose(fd);
	CleanUp();
	hFind = FindFirstFile(".\\*", &FindFileData);
 	while (FindNextFile(hFind, &FindFileData) != 0) 
      	{
      		char *pdb;
      		if ((pdb = strrchr(FindFileData.cFileName, '.')) && !strcmp(pdb+1, "pdb") && strcmp(FindFileData.cFileName, "Colossus.pdb") && strcmp(FindFileData.cFileName, "vc70.pdb"))
      			DeleteFile(FindFileData.cFileName);
      	}
      	FindClose(hFind);
      	MessageBox(hwMain, text, "Error fatal", MB_OK);
	return EXCEPTION_EXECUTE_HANDLER;
}
void InitDebug(void) 
{
	//SetUnhandledExceptionFilter(&ExceptionFilter);
}
