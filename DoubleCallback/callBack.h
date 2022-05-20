naked float __KiCallUserMode2(pv64 OutVarPtr, pv64 CallCtx, u64 KStackControl)
{
	__asm {
		//alloc stack
		sub rsp, 138h

		//save non volatile float regs
		movaps xmmword ptr [rsp + 70h], xmm10
		movaps xmmword ptr [rsp + 40h], xmm7
		lea rax, [rsp + 100h]
		movaps xmmword ptr [rax - 80h], xmm11
		movaps xmmword ptr [rax - 50h], xmm14
		movaps xmmword ptr [rsp + 30h], xmm6
		movaps xmmword ptr [rax - 60h], xmm13
		movaps xmmword ptr [rsp + 50h], xmm8
		movaps xmmword ptr [rax - 40h], xmm15
		movaps xmmword ptr [rsp + 60h], xmm9
		movaps xmmword ptr [rax - 70h], xmm12

		//save non volatile int regs
		mov [rax - 8], rbp
		mov rbp, rsp
		mov [rax], rbx
		mov [rax + 8], rdi
		mov [rax + 10h], rsi
		mov [rax + 20h], r13
		mov [rax + 30h], r15
		mov [rax + 18h], r12
		mov [rax + 28h], r14

		//save ret val vars
		mov [rbp + 0D8h], rcx
		lea rax, [r8 - 30h]
		mov [rbp + 0E0h], rax
		mov rbx, gs:[188h]
		mov [r8 + 20h], rsp
		mov rsi, [rbx + 90h]
		mov [rbp + 0D0h], rsi

		//save new stack vars
		cli
		mov [rbx + 28h], r8
		lea r9, [r8 + 50h] //calc new stack ptr
		mov [rbx + 38h], r9
		mov rdi, gs:[8]
		mov [rdi + 4], r8

		//save cur trap frame
		mov ecx, 6000h
		sub r9, rcx
		mov gs:[1A8h], r8
		mov [rbx + 30h], r9
		lea rsp, [r8 - 190h]
		mov rdi, rsp
		mov ecx, 32h //190h
		rep movsq

		//fix csr
		lea rbp, [rsi - 110h]
		ldmxcsr dword ptr [rbp - 54h]

		//restore rbp, r11
		mov r11, [rbp + 0F8h] //restore eflags
		mov rbp, [rbp + 0D8h]

		//load rip, rsp, rax
		mov rax, [rdx + 10h]
		mov rsp, [rdx + 8]
		mov rcx, [rdx]

		//load floats
		movss xmm0, dword ptr [rdx + 18h]
		movss xmm2, dword ptr [rdx + 28h]
		movss xmm1, dword ptr [rdx + 20h]
		movss xmm3, dword ptr [rdx + 30h]

		//load userctx x32
		cmp qword ptr [rdx + 38h], 0
		jz Sw2UserMode
		mov r13, [rdx + 38h]

		//goto usermode
		Sw2UserMode:
		swapgs
		sysretq
	}
}

class UsermodeCallback22
{
public:
//private:
	bool Wow64; PVOID PreCall, PreCall32, PostCall;
	typedef NTSTATUS(__fastcall* _etThreadCtxFn)(PETHREAD, PCONTEXT, MODE, MODE, MODE);
	typedef u64(NTAPI* MmCreateKernelStack_t)(u32, u16, PETHREAD);
	typedef void(NTAPI* MmDeleteKernelStack_t)(u64, u8);
	MmCreateKernelStack_t MmCreateKernelStack;
	MmDeleteKernelStack_t MmDeleteKernelStack;
	_etThreadCtxFn GetCtx, SetCtx;

	noinl u64 Call2(pv64 ptr, char* userRsp, void** callData, float& retFloat)
	{
		//alloc new kernel stack
		auto CurrentThread = (u64)KeGetCurrentThread();
		auto KernelStack = MmCreateKernelStack(false, 0, 0);
		if (!KernelStack) return 0;

		//fill kernel stack control
		auto KernelStackControl = KernelStack - 0x50;
		*(u64*)(KernelStack - 0x50) = KernelStack;
		*(u64*)(KernelStack - 0x48) = KernelStack - 0x6000;
		*(u64*)(KernelStack - 0x40) = *(u64*)(CurrentThread + 0x38);
		*(u64*)(KernelStack - 0x38) = *(u64*)(CurrentThread + 0x30);
		*(u64*)(KernelStack - 0x28) = *(u64*)(CurrentThread + 0x28);
		
		//fill kernel shadow stack control
		*(u64*)(KernelStack - 0x20) = 0;
		*(u64*)(KernelStack - 0x18) = 0;
		*(u64*)(KernelStack - 0x10) = 0;
		*(u64*)(KernelStack - 0x08) = 0;
		
		//push ret to kernel addr
		if (!Wow64) {
			*(pv64*)(userRsp + 0x48) = PostCall;
		}

		//goto usermode
		callData[2] = ptr;
		callData[1] = userRsp;
		callData[0] = PreCall;

		//get apc state
		auto apcIndex = *(u8*)(CurrentThread + 0x24A);
		auto v1 = *(u16*)(CurrentThread + 0x1E4);
		auto v2 = *(u16*)(CurrentThread + 0x1E6);

		//fix apc state
		*(u16*)(CurrentThread + 0x1E4) = 0;
		*(u16*)(CurrentThread + 0x1E6) = 0;
		*(u8*)(CurrentThread + 0x24A) = 0;
		
		//goto usermode
		u64 retValPtr = 0;
		retFloat = __KiCallUserMode2(&retValPtr, callData, KernelStackControl);

		//restore apc state
		*(u16*)(CurrentThread + 0x1E4) = v1;
		*(u16*)(CurrentThread + 0x1E6) = v2;
		*(u8*)(CurrentThread + 0x24A) = apcIndex;
		
		//delete kernel stack
		MmDeleteKernelStack(KernelStack, false);
		return *(u64*)retValPtr;
	}

	void GetCtx1(PCONTEXT ctx) {
		ctx->ContextFlags = CONTEXT_FULL;
		GetCtx(KeGetCurrentThread(), ctx, KernelMode, UserMode, KernelMode);
	}

	void SetCtx1(PCONTEXT ctx) {
		SetCtx(KeGetCurrentThread(), ctx, KernelMode, UserMode, KernelMode);
	}

	

public:
	auto GetWow64Ctx() {
		return (WOW64_CONTEXT*)(*(u64*)((u64)PsGetCurrentThreadTeb() + 0x1488) + 4);
	}

	auto GetWow64Flags() {
		return (u16*)(*(u64*)((u64)PsGetCurrentThreadTeb() + 0x1488));
	}

	void Init(bool is32Bit = false) 
	{
		//get ctx funcs
		auto setctxb = (u8*)GetProcAdress(Kbase2, "PsSetContextThread");
		auto getctxb = (u8*)GetProcAdress(Kbase2, "PsGetContextThread");
		while (*setctxb != 0xE8) setctxb++;
		while (*getctxb != 0xE8) getctxb++;
		SetCtx = (_etThreadCtxFn)RVA(setctxb, 5);
		GetCtx = (_etThreadCtxFn)RVA(getctxb, 5);

		//
		MmDeleteKernelStack = (MmDeleteKernelStack_t)RVA(FindPatternSect(Kbase2, E("PAGE"), E("8B D5 E8 ? ? ? ? 48 8B 05 ? ? ? ? 48 05")) + 2, 5);
		MmCreateKernelStack = (MmCreateKernelStack_t)RVA(FindPatternSect(Kbase2, E("PAGE"), E("E8 ? ? ? ? 48 85 C0 0F 84 ? ? ? ? 49 89 06 FF")), 5);
		
		//
		auto nt = GetUserModuleBase(IoGetCurrentProcess(), E("ntdll"), 0, 1);
		//auto longProc = GetProcessWModule("dwm.exe", "ntdll", &nt);
		PreCall = FindPatternSect(nt, E(".text"), E("48 8B 4C 24 ? 48 8B 54 24 ? 4C"));
		//DetachFromProcess(longProc);

		auto user32 = GetUserModuleBase(IoGetCurrentProcess(), E("user32"));
		
		if (is32Bit) 
		{
			auto wow64 = GetUserModuleBase(IoGetCurrentProcess(), "wow64cpu", 0, 1);

			PostCall = FindPatternRange((pv64)((u32)user32 + 0x1000), 0x6400000, "6A 00 89 45 E8");
			PreCall32 = FindPatternRange((pv64)((u32)wow64 + 0x1000), 0x6400000, "41 57");
		
			//hp(PostCall);
			//hp(PreCall32);


			Wow64 = is32Bit;
		}

		else {
			PostCall = FindPatternSect(user32, E(".text"), E("45 33 C0 48 89 44 24 20 48 8D 4C 24 20"));
		}
		
	}

	inl u64& GetUserStackPtr() {
		auto CurrentThread = (u64)KeGetCurrentThread();
		auto trapFrame = *(KTRAP_FRAME**)(CurrentThread + 0x90);
		return *(u64*)&trapFrame->Rsp;
	}

	void Call32(pv64 func, u32 arg1)
	{
		//CONTEXT ctx2;
		//GetCtx1(&ctx2);


		auto wow64ptr = GetWow64Ctx();
		auto wow64 = *wow64ptr;

		//alloc user mode stack
		//FIXME: Need calc stack alloc size, not magic static //sizeof...(va)
		const auto NewUserRsp = (char*)((wow64ptr->Esp - 0x98) & 0xFFFFFFFFFFFFFFF0);
		//*(u64*)(NewUserRsp + 0x20) = arg1;

		//auto stack = (u32)UAlloc(0x1000) + 0x1000 - 0x800;
		//auto stack2 = (u32)UAlloc(0x1000) + 0x1000 - 0x800;
		//auto stack3 = (u32)UAlloc(0x1000) + 0x1000 - 0x800;
		
		wow64ptr->Eip = (u32)func;
		wow64ptr->Esp = (u32)NewUserRsp - 0x28;
		wow64ptr->Ecx = arg1;

		auto stack2 = (u32)wow64ptr->Esp;
		*(u32*)((stack2)/*ret addr*/) = (u32)PostCall;
		*(u32*)((stack2)+4/*1 arg*/) = 0;
		*(u32*)((stack2)+8/*2 arg*/) = 0;
		*(u32*)((stack2)+12/*3 arg*/) = 0;
		*(u32*)((stack2)+16/*4 arg*/) = 0;

		////fill 64 to 32 ctx
		//auto ctx = (u32)stack3;
		//*(u32*)(ctx + 0x38/*ebp*/) = (u32)stack;
		//*(u32*)(ctx + 0x48/*esp*/) = (u32)stack2;
		//*(u32*)(ctx + 0x3C/*rip*/) = (u32)func;
		//
		////fill user stack
		//*(u32*)((stack)/*ret addr*/) = (u32)PostCall;
		//*(u32*)((stack) + 4/*1 arg*/) = 0;
		//*(u32*)((stack) + 8/*2 arg*/) = 0;
		//*(u32*)((stack) + 12/*3 arg*/) = 0;
		//*(u32*)((stack) + 16/*4 arg*/) = 0;
		//
		//*(u32*)((stack2)/*ret addr*/) = (u32)PostCall;
		//*(u32*)((stack2)+4/*1 arg*/) = 0;
		//*(u32*)((stack2)+8/*2 arg*/) = 0;
		//*(u32*)((stack2)+12/*3 arg*/) = 0;
		//*(u32*)((stack2)+16/*4 arg*/) = 0;

		void* cData[8]{};
		//cData[7] = (pv64)ctx;

		float retFloat;
		Call2(PreCall32, (char*)NewUserRsp, cData, retFloat);
	}

	template<typename Ret = pv64, typename ptrType, typename... va>
	inl Ret Call(ptrType Ptr, va... Args)
	{
		//alloc user mode stack
		//FIXME: Need calc stack alloc size, not magic static //sizeof...(va)
		const auto NewUserRsp = (char*)((GetUserStackPtr() - 0x98) & 0xFFFFFFFFFFFFFFF0);

		//prep vars
		void* cData[9]{}; auto am = std::make_tuple(std::forward<va>(Args)...);
		auto select_tuple = []<size_t Off, typename Tuple, std::size_t... Ints>
			(std::integral_constant<size_t, Off>, Tuple && tuple, std::index_sequence<Ints...>) {
			return std::make_tuple(std::get<Off + Ints>(std::forward<Tuple>(tuple))...);
		}; auto ret_index = std::make_index_sequence<sizeof...(va) >= 4 ? 4 : sizeof...(va)>();
		auto reg_vars = select_tuple(std::integral_constant<size_t, 0>{}, am, ret_index);
		auto remapRegVars = [&](int& indx, auto var) {
			if constexpr (!std::is_same<decltype(var), float>::value) 
			*(u64*)(NewUserRsp + 0x20 + (indx++ * 8)) = *(u64*)&var;
			else cData[3 + indx++] = *(pv64*)&var;
		}; 
		
		//remap vars
		if constexpr (sizeof...(va) >= 5) {
			auto stackVars = select_tuple(std::integral_constant<size_t, 4>{}, am, std::make_index_sequence<sizeof...(Args) - 4>());
			auto remapStackVars = [&](int& indx, auto var) { *(u64*)(NewUserRsp + 0x70 + (indx++ * 8)) = *(u64*)&var; };
			std::apply([&](auto&&... xs) { int indx = 0; (remapStackVars(indx, xs), ...); }, stackVars);
		} std::apply([&](auto&&... xs) { int indx = 0; (remapRegVars(indx, xs), ...); }, reg_vars);

		//call usermode
		float retFloat;
		auto ret = Call2((pv64)Ptr, NewUserRsp, cData, retFloat);
		if constexpr (std::is_same<Ret, float>::value) return retFloat;
		return (Ret)ret;
	}
} UserCallback2;

class ThreadCtxMgr 
{
private:
	typedef NTSTATUS(__fastcall* _etThreadCtxFn)(PETHREAD, PCONTEXT, MODE, MODE, MODE);
	_etThreadCtxFn GetCtx1, SetCtx1;

public:
	void Init() {
		auto setctxb = (u8*)GetProcAdress(Kbase2, E("PsSetContextThread"));
		auto getctxb = (u8*)GetProcAdress(Kbase2, E("PsGetContextThread"));
		while (*setctxb != 0xE8) setctxb++;
		while (*getctxb != 0xE8) getctxb++;
		SetCtx1 = (_etThreadCtxFn)RVA(setctxb, 5);
		GetCtx1 = (_etThreadCtxFn)RVA(getctxb, 5);
	}

	void GetCtx(PCONTEXT ctx, u32 flags = CONTEXT_INTEGER | CONTEXT_CONTROL) {
		ctx->ContextFlags = flags;
		GetCtx1(KeGetCurrentThread(), ctx, KernelMode, UserMode, KernelMode);
	}

	void SetCtx(PCONTEXT ctx) {
		SetCtx1(KeGetCurrentThread(), ctx, KernelMode, UserMode, KernelMode);
	}
	void SetCtx2(PETHREAD th, PCONTEXT ctx) {
		SetCtx1(th, ctx, KernelMode, UserMode, KernelMode);
	}
} ThCtx;