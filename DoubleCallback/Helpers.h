//DBG Help
#define DBG1 0
#ifdef DBG1
//#define wsp(a) DbgPrintEx(0, 0, "\nFACE WSTR: %ws\n", (a))
#define hp(a) DbgPrintEx(0, 0, "\nFACE HEX: 0x%p\n", (a))
#define sp(a) DbgPrintEx(0, 0, "\nFACE STR: %s\n", (a))
#define dp(a) DbgPrintEx(0, 0, "\nFACE DEC: %d\n", (a))
//#define fp(a) DbgPrintEx(0, 0, "\nFACE FP: %d.%d\n", (int)(a), abs((int)(((a) - (int)(a)) * 1000000)))
#endif

//bool KeAreInterruptsEnabled() {
//	return (__readeflags() & 0x200) != 0;
//}



//ptr utils
template <typename Type>
inl Type EPtr(Type Ptr) {
	auto Key = (ULONG64)SharedUserData->Cookie *
		SharedUserData->Cookie *
		SharedUserData->Cookie *
		SharedUserData->Cookie;
	return (Type)((ULONG64)Ptr /*^ Key*/);
}

template<typename Ret = void, typename... ArgT>
inl Ret CallPtr(PVOID Fn, ArgT... Args) {
	typedef Ret(*ShellFn)(ArgT...);
	return ((ShellFn)Fn)(Args...);
}

//kernel memory utils
inl PVOID KAlloc(ULONG Size, bool exec = false) {
	PVOID Buff = ImpCall(ExAllocatePoolWithTag, exec ? NonPagedPool : NonPagedPoolNx, Size, 'KgxD');
	MemZero(Buff, Size);
	return Buff;
}

inl void KFree(PVOID Ptr) {
	ImpCall(ExFreePoolWithTag, Ptr, 'KgxD');
}

//basic utils
PVOID FindSection(PVOID ModBase, const char* Name, PULONG SectSize = 0)
{
	//get & enum sections
	PIMAGE_NT_HEADERS NT_Header = NT_HEADER(ModBase);
	PIMAGE_SECTION_HEADER Sect = IMAGE_FIRST_SECTION(NT_Header);
	//sp("123");
	////sp(Sect->Name);
	//hp(ModBase);
	//hp(NT_Header);


	for (PIMAGE_SECTION_HEADER pSect = Sect; pSect < Sect + NT_Header->FileHeader.NumberOfSections; pSect++)
	{
		//copy section name
		char SectName[9]; SectName[8] = 0;
		*(ULONG64*)&SectName[0] = *(ULONG64*)&pSect->Name[0];
		
		//sp(pSect->Name);
		//check name
		if (StrICmp(Name, SectName, true))
		{
			//save size
			if (SectSize) {
				ULONG SSize = SizeAlign(max(pSect->Misc.VirtualSize, pSect->SizeOfRawData));
				*SectSize = SSize;
			}

			//ret full sect ptr
			return (PVOID)((ULONG64)ModBase + pSect->VirtualAddress);
		}
	}

	//no section
	return nullptr;
}

PVOID FindSection32(PVOID ModBase, const char* Name, PULONG SectSize = 0)
{
	//get & enum sections
	auto NT_Header = NT_HEADER32(ModBase);
	PIMAGE_SECTION_HEADER Sect = IMAGE_FIRST_SECTION(NT_Header);
	//sp("123");
	////sp(Sect->Name);
	//hp(ModBase);
	//hp(NT_Header);


	for (PIMAGE_SECTION_HEADER pSect = Sect; pSect < Sect + NT_Header->FileHeader.NumberOfSections; pSect++)
	{
		//copy section name
		char SectName[9]; SectName[8] = 0;
		*(ULONG64*)&SectName[0] = *(ULONG64*)&pSect->Name[0];

		//sp(pSect->Name);
		//check name
		if (StrICmp(Name, SectName, true))
		{
			//save size
			if (SectSize) {
				ULONG SSize = SizeAlign(max(pSect->Misc.VirtualSize, pSect->SizeOfRawData));
				*SectSize = SSize;
			}

			//ret full sect ptr
			return (PVOID)((ULONG64)ModBase + pSect->VirtualAddress);
		}
	}

	//no section
	return nullptr;
}

bool MakeValid(PVOID addr)
{
	auto VirtualQuery = [](PVOID Addr, MEMORY_BASIC_INFORMATION* MemInfo) {
		size_t outSz;
		auto ret = !ZwQueryVirtualMemory((HANDLE)-1, Addr, MemoryBasicInformation, MemInfo, sizeof(MEMORY_BASIC_INFORMATION), &outSz);
		return ret;
	};

	if (!MmIsAddressValid(addr)) {
		SIZE_T RetSize;
		MEMORY_BASIC_INFORMATION MemInfo{};
		if (VirtualQuery((PVOID)addr, &MemInfo)) {
			if ((MemInfo.State == MEM_COMMIT) &&
				(MemInfo.Protect & PAGE_EXECUTE_READ)) {

				*(volatile char*)addr;
				//sp("ok!");

				return 0;
				return MmIsAddressValid(addr);
			}

			else
				return 0;
		}
		else
			return 0;
	}

	return 1;
}

bool FixUserMemory(PVOID ModBase, ULONG RegSize = 0)
{
	#define LOCK_VM_IN_RAM 2
	#define LOCK_VM_IN_WORKING_SET 1

	//get modsize
	if (!RegSize) {
		PIMAGE_NT_HEADERS NT_Header = NT_HEADER(ModBase);
		PIMAGE_SECTION_HEADER Sect = IMAGE_FIRST_SECTION(NT_Header);
		RegSize = NT_Header->OptionalHeader.SizeOfImage;
	}

	//fix workset size
	QUOTA_LIMITS_EX ql{};
	ql.MinimumWorkingSetSize = 0x200000000; //8GB
	ql.MaximumWorkingSetSize = 0x200000000; //8GB
	
	QUOTA_LIMITS ql2{};
	ZwQueryInformationProcess((HANDLE)-1i64, ProcessQuotaLimits, &ql2, sizeof(ql2));
	
	//hp(ql2.MinimumWorkingSetSize);
	//hp(ql2.MaximumWorkingSetSize);


	//ZwSetInformationProcess((HANDLE)-1i64, ProcessQuotaLimits, &ql, sizeof(ql));

	//fix mod pages
	for (size_t i = 0; i < RegSize; i += PAGE_SIZE) {
		PVOID v4 = (PVOID)((ULONG64)ModBase + i); size_t v5 = PAGE_SIZE;
		if (!NT_SUCCESS(ZwLockVirtualMemory((HANDLE)-1i64, &v4, &v5, LOCK_VM_IN_WORKING_SET | LOCK_VM_IN_RAM)))
			return false;
	}
	//sp("ok!");
	//ok!!!
	return true;
}

PUCHAR FindPatternSect2(PVOID ModBase, const char* SectName, const char* Pattern)
{
	//find pattern utils
#define InRange(x, a, b) (x >= a && x <= b) 
#define GetBits(x) (InRange(x, '0', '9') ? (x - '0') : ((x - 'A') + 0xA))
#define GetByte(x) ((UCHAR)(GetBits(x[0]) << 4 | GetBits(x[1])))

//get sect range
	ULONG SectSize;
	PUCHAR ModuleStart = (PUCHAR)FindSection(ModBase, SectName, &SectSize);
	PUCHAR ModuleEnd = ModuleStart + SectSize;

	//scan pattern main
	PUCHAR FirstMatch = nullptr;
	const char* CurPatt = Pattern;
	for (; ModuleStart < ModuleEnd; ++ModuleStart) {
		bool SkipByte = (*CurPatt == '\?');
		if (SkipByte || *ModuleStart == GetByte(CurPatt)) {
			if (!FirstMatch) FirstMatch = ModuleStart;
			SkipByte ? CurPatt += 2 : CurPatt += 3;
			if (CurPatt[-1] == 0) return FirstMatch;
		}

		else if (FirstMatch) {
			ModuleStart = FirstMatch;
			FirstMatch = nullptr;
			CurPatt = Pattern;
		}
	}

	//fail
	return nullptr;
}

#include "Memory.h"

bool readByte(PVOID addr, UCHAR* ret)
{
	//if (!MakePageValid(addr)) {
	//	
	//	return false;
	//}

	*ret = *(volatile char*)addr;
	return true;
}


PUCHAR FindPatternSect(PVOID ModBase, const char* SectName, const char* Pattern)
{
	//find pattern utils
	#define InRange(x, a, b) (x >= a && x <= b) 
	#define GetBits(x) (InRange(x, '0', '9') ? (x - '0') : ((x - 'A') + 0xA))
	#define GetByte(x) ((UCHAR)(GetBits(x[0]) << 4 | GetBits(x[1])))

	//get sect range
	ULONG SectSize;
	PUCHAR ModuleStart = (PUCHAR)FindSection(ModBase, SectName, &SectSize);
	PUCHAR ModuleEnd = ModuleStart + SectSize;
	
	//scan pattern main
	PUCHAR FirstMatch = nullptr;
	const char* CurPatt = Pattern;
	for (; ModuleStart < ModuleEnd; ++ModuleStart)
	{
		bool SkipByte = (*CurPatt == '\?');

		//hp(ModuleStart);
		UCHAR byte1;
		if (!readByte(ModuleStart, &byte1)) {
			auto addr2 = (u64)ModuleStart;
			addr2 &= 0xFFFFFFFFFFFFF000;
			addr2 += 0xFFF;
			ModuleStart = (PUCHAR)addr2;
			//sp("123");
			goto Skip;
		}
	
		if (SkipByte || byte1 == GetByte(CurPatt)) {
			if (!FirstMatch) FirstMatch = ModuleStart;
			SkipByte ? CurPatt += 2 : CurPatt += 3;
			if (CurPatt[-1] == 0) return FirstMatch;
		}

		else if (FirstMatch) {
			ModuleStart = FirstMatch;
			Skip:
			FirstMatch = nullptr;
			CurPatt = Pattern;
		}
	}

	//failed
	return nullptr;
}



PUCHAR FindPatternRange(PVOID Start, u32 size, const char* Pattern)
{
	//find pattern utils
#define InRange(x, a, b) (x >= a && x <= b) 
#define GetBits(x) (InRange(x, '0', '9') ? (x - '0') : ((x - 'A') + 0xA))
#define GetByte(x) ((UCHAR)(GetBits(x[0]) << 4 | GetBits(x[1])))

//get sect range
	ULONG SectSize;
	PUCHAR ModuleStart = (PUCHAR)Start;
	PUCHAR ModuleEnd = ModuleStart + size;

	//scan pattern main
	PUCHAR FirstMatch = nullptr;
	const char* CurPatt = Pattern;
	for (; ModuleStart < ModuleEnd; ++ModuleStart)
	{
		bool SkipByte = (*CurPatt == '\?');

		//hp(ModuleStart);
		UCHAR byte1;
		if (!readByte(ModuleStart, &byte1)) {
			auto addr2 = (u64)ModuleStart;
			addr2 &= 0xFFFFFFFFFFFFF000;
			addr2 += 0xFFF;
			ModuleStart = (PUCHAR)addr2;
			//sp("123");
			goto Skip;
		}

		if (SkipByte || byte1 == GetByte(CurPatt)) {
			if (!FirstMatch) FirstMatch = ModuleStart;
			SkipByte ? CurPatt += 2 : CurPatt += 3;
			if (CurPatt[-1] == 0) return FirstMatch;
		}

		else if (FirstMatch) {
			ModuleStart = FirstMatch;
		Skip:
			FirstMatch = nullptr;
			CurPatt = Pattern;
		}
	}

	//failed
	return nullptr;
}

PVOID NQSI(SYSTEM_INFORMATION_CLASS Class) {
	ULONG ret_size = 1024 * 1024;
	PVOID pInfo = KAlloc(ret_size);
	ImpCall(ZwQuerySystemInformation, Class, pInfo, ret_size, &ret_size);
	return pInfo;
}

noinl PVOID GetProcAdress(PVOID ModBase, const char* Name)
{
	//parse headers
	PIMAGE_NT_HEADERS NT_Head = NT_HEADER(ModBase);
	PIMAGE_EXPORT_DIRECTORY ExportDir = (PIMAGE_EXPORT_DIRECTORY)((ULONG64)ModBase + NT_Head->OptionalHeader.DataDirectory[0].VirtualAddress);

	//process records
	for (ULONG i = 0; i < ExportDir->NumberOfNames; i++)
	{
		//get ordinal & name
		USHORT Ordinal = ((USHORT*)((ULONG64)ModBase + ExportDir->AddressOfNameOrdinals))[i];
		const char* ExpName = (const char*)ModBase + ((ULONG*)((ULONG64)ModBase + ExportDir->AddressOfNames))[i];

		//check export name
		if (StrICmp(Name, ExpName, true))
			return (PVOID)((ULONG64)ModBase + ((ULONG*)((ULONG64)ModBase + ExportDir->AddressOfFunctions))[Ordinal]);
	}

	//no export
	return nullptr;
}

PVOID GetProcAdress32(PVOID ModBase, const char* Name)
{
	//parse headers
	auto NT_Head = NT_HEADER32(ModBase);
	PIMAGE_EXPORT_DIRECTORY ExportDir = (PIMAGE_EXPORT_DIRECTORY)((ULONG64)ModBase + NT_Head->OptionalHeader.DataDirectory[0].VirtualAddress);

	//process records
	for (ULONG i = 0; i < ExportDir->NumberOfNames; i++)
	{
		//get ordinal & name
		USHORT Ordinal = ((USHORT*)((ULONG64)ModBase + ExportDir->AddressOfNameOrdinals))[i];
		const char* ExpName = (const char*)ModBase + ((ULONG*)((ULONG64)ModBase + ExportDir->AddressOfNames))[i];

		//check export name
		if (StrICmp(Name, ExpName, true))
			return (PVOID)((ULONG64)ModBase + ((ULONG*)((ULONG64)ModBase + ExportDir->AddressOfFunctions))[Ordinal]);
	}

	//no export
	return nullptr;
}

inl void Sleep(LONG64 MSec) {
	LARGE_INTEGER Delay; Delay.QuadPart = -MSec * 10000;
	ImpCall(KeDelayExecutionThread, KernelMode, false, &Delay);
}

//process utils
inl PEPROCESS AttachToProcess(HANDLE PID)
{
	//get eprocess
	PEPROCESS Process = nullptr;
	if (ImpCall(PsLookupProcessByProcessId, PID, &Process) || !Process)
		return nullptr;

	//take process lock
	if (ImpCall(PsAcquireProcessExitSynchronization, Process))
	{
		//process lock failed
		ImpCall(ObfDereferenceObject, Process);
		return nullptr;
	}

	//attach to process
	ImpCall(KeAttachProcess, Process);
	return Process;
}

inl void DetachFromProcess(PEPROCESS Process)
{
	//check valid process
	if (Process != nullptr)
	{
		//de-attach to process
		ImpCall(KeDetachProcess);

		//cleanup & process unlock
		ImpCall(ObfDereferenceObject, Process);
		ImpCall(PsReleaseProcessExitSynchronization, Process);
	}
}

PVOID GetUserModuleBase(PEPROCESS Process, const char* ModName, ULONG* ModSize = 0, bool force64 = 0)
{
	PPEB32 pPeb32 = (PPEB32)ImpCall(PsGetProcessWow64Process, Process);
	
	if (pPeb32 && !force64)
	{
		if (!pPeb32->Ldr)
			return nullptr;

		for (PLIST_ENTRY32 pListEntry = (PLIST_ENTRY32)((PPEB_LDR_DATA32)pPeb32->Ldr)->InLoadOrderModuleList.Flink;
						   pListEntry != &((PPEB_LDR_DATA32)pPeb32->Ldr)->InLoadOrderModuleList;
						   pListEntry = (PLIST_ENTRY32)pListEntry->Flink) {
			PLDR_DATA_TABLE_ENTRY32 pEntry = CONTAINING_RECORD(pListEntry, LDR_DATA_TABLE_ENTRY32, InLoadOrderLinks);

			if (StrICmp(ModName, (PWCH)pEntry->BaseDllName.Buffer, false)) {
				if (ModSize) {
					*ModSize = pEntry->SizeOfImage;
				}

				return (PVOID)pEntry->DllBase;
			}
		}
	}

	else {
		PPEB PEB = ImpCall(PsGetProcessPeb, Process);
		if (!PEB->Ldr)
			return nullptr;

		for (PLIST_ENTRY pListEntry = PEB->Ldr->InLoadOrderModuleList.Flink;
						 pListEntry != &PEB->Ldr->InLoadOrderModuleList;
						 pListEntry = pListEntry->Flink)
		{
			PLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD(pListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

			if (StrICmp(ModName, pEntry->BaseDllName.Buffer, false))
			{
				if (ModSize) {
					*ModSize = pEntry->SizeOfImage;
				}

				return (PVOID)pEntry->DllBase;
			}
		}
	}

	return nullptr;
}

PVOID GetUserModuleBase1(PEPROCESS Process, const char* ModName)
{
	//get peb & ldr
	PPEB PEB = ImpCall(PsGetProcessPeb, Process);
	if (!PEB || !PEB->Ldr) return nullptr;

	//process modules list (with peb->ldr)
	for (PLIST_ENTRY pListEntry = PEB->Ldr->InLoadOrderModuleList.Flink;
		pListEntry != &PEB->Ldr->InLoadOrderModuleList;
		pListEntry = pListEntry->Flink)
	{
		PLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD(pListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
		if (StrICmp(ModName, pEntry->BaseDllName.Buffer, false))
			return pEntry->DllBase;
	}

	//no module
	return nullptr;
}

DWORD Protect(PVOID addr, DWORD Prot)
{
	DWORD oldProt;
	auto addr2 = addr; SIZE_T SizeUL = 1;
	ZwProtectVirtualMemory((HANDLE)-1, &addr2, &SizeUL, Prot, &oldProt);
	return oldProt;
}

inl PVOID UAlloc(ULONG Size, ULONG Protect = PAGE_READWRITE, bool load = true) {
	PVOID AllocBase = nullptr; SIZE_T SizeUL = SizeAlign(Size);
#define LOCK_VM_IN_RAM 2
#define LOCK_VM_IN_WORKING_SET 1
	if (!ImpCall(ZwAllocateVirtualMemory, ZwCurrentProcess(), &AllocBase, 0, &SizeUL, MEM_COMMIT, Protect)) {
		//ZwLockVirtualMemory(ZwCurrentProcess(), &AllocBase, &SizeUL, LOCK_VM_IN_WORKING_SET | LOCK_VM_IN_RAM);
		if (load)
			MemZero(AllocBase, SizeUL);
	}
	return AllocBase;
}

inl void UFree(PVOID Ptr) {
	SIZE_T SizeUL = 0;
	ImpCall(ZwFreeVirtualMemory, ZwCurrentProcess(), &Ptr, &SizeUL, MEM_RELEASE);
}

void CallUserMode(PVOID Func)
{
	struct KEUSER_CALLBACK {
		ULONG64 Arg1;
		ULONG64 Arg2;
		ULONG64 Arg3;
		ULONG64 Arg4;
		ULONG64 Arg5;
		PVOID   Func;
	};

	KEUSER_CALLBACK UserData{};
	//UserData.Arg1 = (ULONG64)ContextUser;
	//UserData.Arg5 = (ULONG64)a5;
	//UserData.Arg2 = 0;
	//UserData.Func = NtContinue;

	UserData.Func = Func;
	UserData.Arg1 = 0;
	UserData.Arg2 = 0;
	UserData.Arg3 = 0;
	UserData.Arg4 = 0;

	//fix apc state
	//auto CurrentThread = (ULONG64)KeGetCurrentThread();

	//auto proc = *(ULONG64*)(CurrentThread + 0x220);
	////*(ULONG64*)(CurrentThread + 0x220) = (ULONG64)Ptr1;
	//
	//auto ApcStateIndex = *(BYTE*)(CurrentThread + 0x24A);
	//*(BYTE*)(CurrentThread + 0x24A) = 0;
	
	//get apc state
	auto CurrentThread = (u64)KeGetCurrentThread();
	auto apcIndex = *(u8*)(CurrentThread + 0x24A);
	auto v1 = *(u16*)(CurrentThread + 0x1E4);
	auto v2 = *(u16*)(CurrentThread + 0x1E6);

	//fix apc state
	*(u16*)(CurrentThread + 0x1E4) = 0;
	*(u16*)(CurrentThread + 0x1E6) = 0;
	*(u8*)(CurrentThread + 0x24A) = 0;

	//call in usermode
	KeUserModeCallback(2, &UserData, sizeof(UserData), &UserData, &UserData);

	//restore apc state
	*(u16*)(CurrentThread + 0x1E4) = v1;
	*(u16*)(CurrentThread + 0x1E6) = v2;
	*(u8*)(CurrentThread + 0x24A) = apcIndex;
}

void CallUserMode32(PVOID Func, u32 arg1, u32 arg2, u32 arg3, u32 arg4)
{
	struct KEUSER_CALLBACK {
		u32 Arg1;
		u32 Arg2;
		u32 Arg3;
		u32 Arg4;
		u32 Arg5; 
		u32 Arg6;
		u32 Func;
	};

	KEUSER_CALLBACK UserData;
	//UserData.Arg1 = (ULONG64)ContextUser;
	//UserData.Arg5 = (ULONG64)a5;
	//UserData.Arg2 = 0;
	//UserData.Func = NtContinue;

	UserData.Func = (u32)Func;
	UserData.Arg1 = arg1;
	UserData.Arg2 = arg2;
	UserData.Arg3 = arg3;
	UserData.Arg4 = arg4;

	//fix apc state
	//auto CurrentThread = (ULONG64)KeGetCurrentThread();

	//auto proc = *(ULONG64*)(CurrentThread + 0x220);
	////*(ULONG64*)(CurrentThread + 0x220) = (ULONG64)Ptr1;
	//
	//auto ApcStateIndex = *(BYTE*)(CurrentThread + 0x24A);
	//*(BYTE*)(CurrentThread + 0x24A) = 0;

	//get apc state
	auto CurrentThread = (u64)KeGetCurrentThread();
	auto apcIndex = *(u8*)(CurrentThread + 0x24A);
	auto v1 = *(u16*)(CurrentThread + 0x1E4);
	auto v2 = *(u16*)(CurrentThread + 0x1E6);

	//fix apc state
	*(u16*)(CurrentThread + 0x1E4) = 0;
	*(u16*)(CurrentThread + 0x1E6) = 0;
	*(u8*)(CurrentThread + 0x24A) = 0;

	//goto usermode
	KeUserModeCallback(2, &UserData, sizeof(UserData), &UserData, &UserData);

	//restore apc state
	*(u16*)(CurrentThread + 0x1E4) = v1;
	*(u16*)(CurrentThread + 0x1E6) = v2;
	*(u8*)(CurrentThread + 0x24A) = apcIndex;
}



//kernel utils
PEPROCESS GetProcessWModule(const char* ProcName, const char* ModName, PVOID* WaitModBase = nullptr)
{
	//get process list
	PEPROCESS EProc = nullptr;
	PSYSTEM_PROCESS_INFO pInfo = (PSYSTEM_PROCESS_INFO)NQSI(SystemProcessInformation), pInfoCur = pInfo;

	while (true)
	{
		//get process name
		const wchar_t* ProcessName = pInfoCur->ImageName.Buffer;
		if (ImpCall(MmIsAddressValid, (PVOID)ProcessName))
		{
			//check process name
			if (StrICmp(ProcName, ProcessName, true)) 
			{
				//attach to process
				PEPROCESS Process = AttachToProcess(pInfoCur->UniqueProcessId);
				if (Process != nullptr)
				{
					//check wait module
					PVOID ModBase = GetUserModuleBase(Process, ModName);
					if (ModBase) {
						//check save modbase
						if (WaitModBase)
							*WaitModBase = ModBase;

						EProc = Process;
						break;
					}

					//failed, no wait module
					DetachFromProcess(Process);
				}
			}
		}

		//goto next process entry
		if (!pInfoCur->NextEntryOffset) break;
		pInfoCur = (PSYSTEM_PROCESS_INFO)((ULONG64)pInfoCur + pInfoCur->NextEntryOffset);
	}

	//cleanup
	KFree(pInfo);
	return EProc;
}

PVOID GetKernelModuleBase(const char* ModName, ULONG* ModSize = nullptr)
{
	//get module list
	PSYSTEM_MODULE_INFORMATION ModuleList = (PSYSTEM_MODULE_INFORMATION)NQSI(SystemModuleInformation);

	//process module list
	PVOID ModuleBase = 0;
	for (ULONG64 i = 0; i < ModuleList->ulModuleCount; i++) {
		SYSTEM_MODULE Module = ModuleList->Modules[i];
		//if (Module.Flags & 0x20) {
		//	sp(&Module.ImageName[Module.ModuleNameOffset]);
		//}
		//
		if (StrICmp(&Module.ImageName[Module.ModuleNameOffset], ModName, true)) {
			ModuleBase = Module.Base;
			if (ModSize) {
				*ModSize = Module.Size;
			} break;
		}
	}

	//cleanup
	KFree(ModuleList);
	return ModuleBase;
}

PUCHAR FindPatternInProcess(ULONG ModBase, const char* Pattern)
{
	//find pattern utils
	#define InRange(x, a, b) (x >= a && x <= b) 
	#define GetBits(x) (InRange(x, '0', '9') ? (x - '0') : ((x - 'A') + 0xA))
	#define GetByte(x) ((UCHAR)(GetBits(x[0]) << 4 | GetBits(x[1])))

	//get module range
	PUCHAR ModuleStart = (PUCHAR)ModBase; if (!ModuleStart) return nullptr;
	PIMAGE_NT_HEADERS NtHeader = ((PIMAGE_NT_HEADERS)(ModuleStart + ((PIMAGE_DOS_HEADER)ModuleStart)->e_lfanew));
	PUCHAR ModuleEnd = (PUCHAR)(ModuleStart + NtHeader->OptionalHeader.SizeOfImage - 0x1000); ModuleStart += 0x1000;

	//scan pattern main
	PUCHAR FirstMatch = nullptr;
	const char* CurPatt = Pattern;
	for (; ModuleStart < ModuleEnd; ++ModuleStart)
	{
		bool SkipByte = (*CurPatt == '\?');
		if (SkipByte || *ModuleStart == GetByte(CurPatt))
		{
			if (!FirstMatch)
				FirstMatch = ModuleStart;

			SkipByte ? CurPatt += 2 : CurPatt += 3;

			if (CurPatt[-1] == 0)
				return FirstMatch;
		}

		else if (FirstMatch)
		{
			ModuleStart = FirstMatch;
			FirstMatch = nullptr;
			CurPatt = Pattern;
		}
	}

	return nullptr;
}

inl void ForceWrite(PVOID Dst, PVOID Src, ULONG Size) 
{
	//bool enable = KeAreInterruptsEnabled();
	_disable();
	auto cr0 = __readcr0();
	__writecr0(cr0 & 0xFFFEFFFF);
	memcpy(Dst, Src, Size);
	__writecr0(cr0);
	//if(enable)
		_enable();
}

//memory
template<typename ReadType>
__forceinline ReadType Read(DWORD Addr)
{
	ReadType ReadData{};
	if (Addr && ImpCall(MmIsAddressValid, (PVOID)Addr)) {
		ReadData = *(ReadType*)Addr;
	}

	return ReadData;
}

__forceinline bool ReadArr(DWORD Addr, PVOID Buff, ULONG Size)
{
	if (ImpCall(MmIsAddressValid, (PVOID)Addr)) {
		MemCpy(Buff, (PVOID)Addr, Size);
		return true;
	}

	return false;
}

template<typename WriteType>
__forceinline void Write(DWORD Addr, WriteType Data)
{
	if (ImpCall(MmIsAddressValid, (PVOID)Addr)) {
		*(WriteType*)Addr = Data;
	}
}

__forceinline void WriteArr(DWORD Addr, PVOID Buff, ULONG Size){
	if (ImpCall(MmIsAddressValid, (PVOID)Addr)) {
		MemCpy((PVOID)Addr, Buff, Size);
	}
}

template<typename WriteType>
__forceinline bool WriteProt(DWORD Addr, /*const*/ WriteType/*&*/ Data) 
{
	PVOID Addr1 = (PVOID)Addr;
	SIZE_T Size1 = sizeof(WriteType);
	
	//hp(Addr1);

	ULONG oldProt;
	if (!ImpCall(ZwProtectVirtualMemory, ZwCurrentProcess(), &Addr1, &Size1, PAGE_EXECUTE_READWRITE, &oldProt))
	{
		auto data1 = Data;
		MemCpy((PVOID)Addr, &data1, sizeof(WriteType));

		//hp(Addr1);
		ImpCall(ZwProtectVirtualMemory, ZwCurrentProcess(), &Addr1, &Size1, oldProt, &oldProt);

		return true;
	}

	return false;
}

void flush1()
{
	auto v9 = __readcr4();
	if ((v9 & 0x20080) != 0) {
		__writecr4(v9 ^ 0x80);
		__writecr4(v9);
	}

	else {
		v9 = __readcr3();
		__writecr3(v9);
	}
}
