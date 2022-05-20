inl unsigned __int64 __readmsr(unsigned long __register) {
	unsigned long __edx;
	unsigned long __eax;
	__asm__ __volatile__("rdmsr" : "=d"(__edx), "=a"(__eax) : "c"(__register) : "memory");
	return (((unsigned __int64)__edx) << 32) | (unsigned __int64)__eax;
}

inl void __writemsr(unsigned long __register, unsigned __int64 val) {
	unsigned long __edx = (unsigned long)(((unsigned __int64)val) << 32);
	unsigned long __eax = (unsigned long)val;
	__asm__ __volatile__("wrmsr" : : "a"(__eax), "d"(__edx), "c"(__register) : "memory");
	//return (((unsigned __int64)__edx) << 32) | (unsigned __int64)__eax;
}

inl void __movsb(unsigned char* __dst, unsigned char const* __src, size_t __n) {
	__asm__ __volatile__("rep movsb" : "+D"(__dst), "+S"(__src), "+c"(__n) : : "memory");
}

inl unsigned __int64 __readcr3() {
	unsigned __int64 __cr3_val;
	__asm__ __volatile__("mov %%cr3, %0" : "=r"(__cr3_val) : : "memory");
	return __cr3_val;
}

inl unsigned __int64 __readcr4() {
	unsigned __int64 __cr4_val;
	__asm__ __volatile__("mov %%cr4, %0" : "=r"(__cr4_val) : : "memory");
	return __cr4_val;
}

inl unsigned __int64 __readcr0() {
	unsigned __int64 __cr0_val;
	__asm__ __volatile__("mov %%cr0, %0" : "=r"(__cr0_val) : : "memory");
	return __cr0_val;
}

inl unsigned __int64 __readcr8() {
	unsigned __int64 __cr8_val;
	__asm__ __volatile__("mov %%cr8, %0" : "=r"(__cr8_val) : : "memory");
	return __cr8_val;
}

inl void __writecr0(unsigned __int64 __cr0_val) {
	__asm__ __volatile__("mov %0, %%cr0" : : "r"(__cr0_val) : "memory");
}

inl void __writecr3(unsigned __int64 __cr3_val) {
	__asm__ __volatile__("mov %0, %%cr3" : : "r"(__cr3_val) : "memory");
}

inl void __writecr4(unsigned __int64 __cr4_val) {
	__asm__ __volatile__("mov %0, %%cr4" : : "r"(__cr4_val) : "memory");
}

inl void __writecr8(unsigned __int64 __cr0_val) {
	__asm__ __volatile__("mov %0, %%cr8" : : "r"(__cr0_val) : "memory");
}

inl void _disable() {
	__asm cli
}

inl void _enable() {
	__asm sti
}

inl void _cli() {
	__asm cli
}

inl void _sti() {
	__asm sti
}

naked unsigned __int64 _readeflags() {
	__asm {
		pushfq
		pop rax
		ret
	}
}

naked void _writeeflags(unsigned __int64 a1) {
	__asm {
		push rcx
		popfq
		ret
	}
}


naked unsigned __int64 _readdr7() {
	__asm {
		mov rax, dr7
		ret
	}
}

naked unsigned __int64 _readdr6() {
	__asm {
		mov rax, dr6
		ret
	}
}

naked unsigned __int64 _writedr7(__int64 a1) {
	__asm {
		mov dr7, rcx
		ret
	}
}

bool KeAreInterruptsEnabled() {
	return (_readeflags() & 0x200) != 0;
}

inl unsigned __int64 KeGetCurrentPrcb() {
	return __readgsqword(0x20);
}

inl auto AddressOfRetAddr() {
	return ((uint64_t*)__builtin_frame_address(0) + 1);
}

#pragma pack(1)
typedef struct _IDTR
{
	USHORT Limit;
	ULONG64 Base;
}IDTR, * PIDTR;

typedef union _IDT_ENTRY
{
	struct kidt
	{
		USHORT OffsetLow;
		USHORT Selector;
		USHORT IstIndex : 3;
		USHORT Reserved0 : 5;
		USHORT Type : 5;
		USHORT Dpl : 2;
		USHORT Present : 1;
		USHORT OffsetMiddle;
		ULONG OffsetHigh;
		ULONG Reserved1;
	}idt;
	UINT64 Alignment;
}IDT_ENTRY, * PIDT_ENTRY;
#pragma pack()