#include "StrongDX.h"

//
PVOID* PlaceVmt;
PVOID* IDXGISwapChainDWM;
u32 off1 = 0; u32 off2 = 0;

void FixCfg(u64 Addr)
{
	//resolve cfg bitmap
	auto ntdll = GetUserModuleBase(IoGetCurrentProcess(), "ntdll");
	auto LdrControlFlowGuardEnforced = GetProcAdress(ntdll, "LdrControlFlowGuardEnforced");
	auto cfgBitMap = *(u64*)RVA2(LdrControlFlowGuardEnforced, 8, 3);

	//bit helpers
	auto toBit = [](u64 val) { return val % 64; };
	auto force_bitset = [&](i64* ptr, int bit) {
		auto val = *ptr;
		_bittestandset64(&val, bit);
		ForceWrite(ptr, &val, 8);
	};

	//get bit & bit offset
	auto curBit = Addr >> 3;
	auto bitPos = (i64*)(cfgBitMap + 8 * (Addr >> 9));

	//align va (16 bytes)
	if ((Addr & 0xF) == 0) {
		auto bit = toBit(curBit);
		if (!_bittest64(bitPos, bit))
			goto fixSBit;
	}

	//not align va (16 bytes)
	else
	{
		//first
		fixMBit: {
		curBit &= ~1ui64;
		auto bit = toBit(curBit);
			if (!_bittest64(bitPos, bit)) {
				force_bitset(bitPos, bit);
			}
		}

		//second
		fixSBit:
		curBit |= 1;
		auto bit = toBit(curBit);
		if (!_bittest64(bitPos, bit)) {
			force_bitset(bitPos, bit);
		}
	}
}

//swapchain utils
u64 classTest;
u64 TestPage(u64 addr1) {
	PVOID* addr = (PVOID*)addr1;
	for (size_t i = 0; i < 512; ++i) {
		if (EPtr(IDXGISwapChainDWM) == addr[i]) {
			auto classPtr = (u64)(&addr[i]);
			//if (*(u64*)(classPtr + 0x10)) 
			{
				classTest = classPtr;
				if (!off1)
					return 1;
				
				auto class1 = *(u64*)(classTest + off1);
				if (*(pv64*)(class1 + off2) != EPtr(PlaceVmt)) {
					*(pv64*)(class1 + off2) = EPtr(PlaceVmt); 
					return 1;
				}
			}
		}
	}

	return 0;
};
u64 FindSwapClass() {
	return ScanUserMemory(TestPage);
}
void FindSwapChain()
{
	PVOID modbase;
	auto pProcess = GetProcessWModule("dwm.exe", "dxgi.dll", &modbase);

	//resolve swapchain
	IDXGISwapChainDWM = (PVOID*)EPtr(RVA(FindPatternSect(modbase, ".text", "48 8B D3 E8 ? ? ? ? 48 83 A7") + 16, 7));

	//find class based on vmt
	FindSwapClass();

	//resolve class offsets
	auto test_instr = FindPatternSect(modbase, ".text", "41 F6 ? 70 0F 85"); //test reg, 70h
	for (int i = 0; i < 50; ++i) {
		if (*(u16*)(test_instr + i) == 0x8B48) { //mov rax, off
			off1 = *(u32*)(test_instr + i + 3);
		}
		else if (*(u16*)(test_instr + i) == 0x8D48) { //lea rcx, off
			off2 = *(u32*)(test_instr + i + 3);
			break;
		}
	}

	//create new vmt
	auto xor_eax_eax_ret = FindPatternSect(modbase, ".text", "33 C0 C3");
	auto place = (pv64*)RVA(FindPatternSect(modbase, ".text", "48 8D 0D ? ? ? ? FF 15 ? ? ? ? 48 8B 05"), 7);
	
	//auto ShouldTerminateOnIndirectStall = (pv64)RVA(FindPatternSect(modbase, ".text", "75 ? E8 ? ? ? ? 84 C0 74 ? 48") + 2, 5);
	auto ntdll = GetUserModuleBase(IoGetCurrentProcess(), "ntdll.dll");
	auto ShouldTerminateOnIndirectStall = (pv64)RVA(FindPatternSect(ntdll, ".text", "E8 ? ? ? ? 84 C0 0F 84 ? ? ? ? 45 33 C9"), 5);
	
	place[0] = ShouldTerminateOnIndirectStall; place[1] = xor_eax_eax_ret;
	PlaceVmt = EPtr(place);

	//add to cfg
	FixCfg((u64)xor_eax_eax_ret);
	FixCfg((u64)ShouldTerminateOnIndirectStall);

	//apply vmt
	FindSwapClass();
	
	//init usercallback
	UserCallback2.Init(0);

	DetachFromProcess(pProcess);
}

//dwm init base
int GetDwmPresentThreadId()
{
	auto pProcess = GetProcessWModule("dwm.exe", "user32.dll", 0);
	HANDLE pid = PsGetProcessId(pProcess);
	auto pInfo2 = (PSYSTEM_PROCESS_INFO)NQSI(SystemProcessInformation);
	auto pInfo = pInfo2;

	while (true)
	{
		if (pInfo->UniqueProcessId == pid) {
			break;
		}

		else if (pInfo->NextEntryOffset)
			pInfo = (PSYSTEM_PROCESS_INFO)((PUCHAR)pInfo + pInfo->NextEntryOffset);

		else
			break;
	}

	int tdid = 0;
	__int64 times = 0;
	for (ULONG i = 0; i < pInfo->NumberOfThreads; i++) {
		PETHREAD ppThread = 0;
		PsLookupThreadByThreadId(pInfo->Threads[i].ClientId.UniqueThread, &ppThread);

		if (ppThread)
		{
			auto times2 = *(__int64*)((__int64)ppThread + 0x48);
			if (times2 > times) {
				times = times2;
				tdid = (int)PsGetThreadId(ppThread);
			}

			ObDereferenceObject(ppThread);
		}
	}

	KFree(pInfo2);

	DetachFromProcess(pProcess);

	return tdid;
}

//dwm render hook
void Present1()
{
	//if (FindSwapClass())
	//{
	//	return;
	//}

	

	if (DirectX.Init((PVOID)classTest))
	{
		//dp(1);
		DirectX.Tick();
	}


	return;


	//get swapchain
	//FindSwapClass();
	//
	////render
	//auto ret = (u64*)UserCallback.GetUserStackPtr();
	//
	//u64 userRetAddr = 0;
	//bool skipOne = 1;
	//for (size_t i = 0; i < 0x100; ++i)
	//{
	//	if (IsExec(ret[i]))
	//	{
	//		if (!skipOne) {
	//			userRetAddr = ret[i];
	//			break;
	//		}
	//
	//		if ((ret[i] > retCheck2) && (ret[i] < (retCheck2 + 0x200))){
	//			skipOne = false;
	//		}
	//	}
	//}
	//
	//DirectX.Init((PVOID)classTest);
	//DirectX.Tick();
	//UserCallback.Call(CHwFullScreenRenderTarget65[16], classTest/*, 0, 0, 0, 0, 0, 0, 0, 0*/);
	//return;
	//
	////ohohohoh strong memez
	//int state1 = FMf.retAddr(userRetAddr, 50);
	//
	//if (state1 == 2) {
	//	DirectX.UpdateDesktopWnd();
	//	UserCallback.Call(CHwFullScreenRenderTarget65[16], classTest/*, 0, 0, 0, 0, 0, 0, 0, 0*/);
	//}
	//
	//if (state1 == 1) {
	//	DirectX.Init((PVOID)classTest);
	//	DirectX.Tick();
	//	UserCallback.Call(CHwFullScreenRenderTarget65[16], classTest/*, 0, 0, 0, 0, 0, 0, 0, 0*/);
	//}
	//
	//return;

	//hp(userRetAddr);
	////hp(retCheck);
	//
	//if (userRetAddr == 0x00007FF870D91268)
	//{
	//	DirectX.Init((PVOID)classTest);
	//	DirectX.Tick();
	//	UserCallback.Call(CHwFullScreenRenderTarget65[16], classTest/*, 0, 0, 0, 0, 0, 0, 0, 0*/);
	//}
	//
	//return;

	//for (size_t i = 0; i < 0x100; ++i)
	//{
	//	//find DrawingContext::PresentDWM(...) ret addr
	//	if ((ret[i] > retCheck) && (ret[i] < (retCheck + 0x200)))
	//	{
	//		//render
	//		DirectX.Init((PVOID)classTest);
	//		DirectX.Tick();
	//		UserCallback.Call(CHwFullScreenRenderTarget65[16], classTest/*, 0, 0, 0, 0, 0, 0, 0, 0*/);
	//
	//		//exit
	//		break;
	//	}
	//}
}

//ud reg hook
u32 GetThreadOff;
u64 DwmPresentThId;
u64 HookFakeStack;
u64 CallBackRetFix;
u64 PtrXorKey;

naked void _callback_out(NTSTATUS ret) {
	__asm {
		mov rax, gs:[188h]
		mov rax, [rax + 60h]
		lea rsp, [rax + 1A0h]
		mov rax, rcx
		xor r9, r9
		iretq
	}
}

void CmRegisterHook(pv64, REG_NOTIFY_CLASS Argument1, REG_CREATE_KEY_INFORMATION* Argument2)
{
	if (Argument1 == RegNtPreOpenKeyEx) 
	{
		//
		if (!wcscmp(Argument2->CompleteName->Buffer, L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Compatibility Assistant\\"))
		{
			//if (rehook) {
			//	FindSwapClass();
			//}
			//else 
			//{
				Present1();
			//}
			//sp("hook");
		}

		else
		{
			//wsp(Argument2->CompleteName->Buffer);
			//sp("re hook");
			
		}

		//

		//_callback_out(STATUS_ACCESS_DENIED);
	}
	_callback_out(STATUS_ACCESS_DENIED);
	//_callback_out(STATUS_SUCCESS);
}

naked void _callback_in() {
	__asm {
		//save iret frame
		mov rax, gs:[188h]
		mov rcx, [rax + 60h]
		lea rcx, [rcx + 1A0h]
		mov r9, PtrXorKey
		mov rax, CallBackRetFix
		xor rax, r9
		mov [rcx], rax
		mov [rcx + 8], cs
		mov qword ptr [rcx + 10h], 040282h
		lea rax, [rsp + 8]
		mov [rcx + 18h], rax
		mov [rcx + 20h], ss

		//check thid
		mov rax, gs:[188h]
		add rax, GetThreadOff
		mov rax, [rax]
		cmp rax, DwmPresentThId
		jnz callout

		//call hook
		mov rsp, HookFakeStack
		xor rsp, r9
		jmp CmRegisterHook

		//exit hook
		callout:
		xor ecx, ecx
		jmp _callback_out
	}
}

LARGE_INTEGER Cookie;
void InitStrongCallBack()
{
	//init vars
	const size_t stack_size = 0x1000 * 8;
	PHYSICAL_ADDRESS add; add.QuadPart = ULLONG_MAX;
	const uintptr_t real_stack = (uintptr_t)MmAllocateContiguousMemory(stack_size, add);
	auto FakeStack = real_stack + stack_size - 0x28;
	auto RetFix = (u64)FindPatternRange(ExNotifyCallback, 0x300, "E9 ? ? ? ? CC");
	GetThreadOff = *(u32*)((u64)PsGetCurrentThreadId + 12);

	//xor vars
	PtrXorKey = FakeStack ^ RetFix * __rdtsc();
	HookFakeStack = FakeStack ^ PtrXorKey;
	CallBackRetFix = RetFix ^ PtrXorKey;

	//register main callback
	PCALLBACK_OBJECT callback;
	OBJECT_ATTRIBUTES objAttr{ sizeof(OBJECT_ATTRIBUTES) };
	ExCreateCallback(&callback, &objAttr, true, true);

	//set call ptr
	*(pv64*)callback = _callback_in;

	//add cli, call hook chain
	auto cli_ret = FindPatternSect(Kbase2, ".text", "FA C3");
	auto jmp_rcx_ptr = FindPatternSect(Kbase2, ".text", "FF 21");
	ExRegisterCallback(callback, (PCALLBACK_FUNCTION)cli_ret, nullptr);
	ExRegisterCallback(callback, (PCALLBACK_FUNCTION)jmp_rcx_ptr, callback);

	//setup registry hook
	CmRegisterCallback((PEX_CALLBACK_FUNCTION)ExNotifyCallback, callback, &Cookie);
}

//dwm init main
void DwmInit()
{
	DwmPresentThId = GetDwmPresentThreadId();

	FindSwapChain();

	InitStrongCallBack();
}