#pragma pack(push, 1)
typedef union VIRT_ADDR_
{
	u64 value;
	void* pointer;

	struct {
		u64 offset : 12;
		u64 pt_index : 9;
		u64 pd_index : 9;
		u64 pdpt_index : 9;
		u64 pml4_index : 9;
		u64 reserved : 16;
	} s;

	struct {
		u64 offset : 21;
		u64 pd_index : 9;
		u64 pdpt_index : 9;
		u64 pml4_index : 9;
		u64 reserved : 16;
	} l;
} VIRT_ADDR;

typedef union PTE_
{
	u64 value;

	struct
	{
		u64 Valid : 1;                                           
		u64 Dirty1 : 1;                                          
		u64 Owner : 1;                                           
		u64 WriteThrough : 1;                                    
		u64 CacheDisable : 1;                                    
		u64 Accessed : 1;                                        
		u64 Dirty : 1;                                           
		u64 LargePage : 1;                                       
		u64 Global : 1;                                          
		u64 CopyOnWrite : 1;                                     
		u64 Unused : 1;                                          
		u64 Write : 1;                                           
		u64 PageFrameNumber : 36; 
		u64 ReservedForHardware : 4;
		u64 ReservedForSoftware : 4;                             
		u64 WsleAge : 4;                                         
		u64 WsleProtection : 3;
		u64 NoExecute : 1;                         
	};
} PTE;
#pragma pack(pop)

u64 g_pxe_base;
u64 g_ppe_base;
u64 g_pde_base;
u64 g_pte_base;

void InitializePteBase(u64 dirbase) {
	PHYSICAL_ADDRESS phAddr; u64 slot = 0;
	auto pfn = PTE{ dirbase }.PageFrameNumber;
	phAddr.QuadPart = pfn << PAGE_SHIFT;
	auto pml4 = (PTE*)ImpCall(MmGetVirtualForPhysical, phAddr);
	while (pml4[slot].PageFrameNumber != pfn) slot++;
	g_pte_base = (slot << 39) + 0xFFFF000000000000;
	g_pde_base = g_pte_base + (slot << 30);
	g_ppe_base = g_pde_base + (slot << 21);
	g_pxe_base = g_ppe_base + (slot << 12);
}

#define PTE_SHIFT 3
#define PTI_SHIFT 12
#define PDI_SHIFT 21
#define PPI_SHIFT 30
#define PXI_SHIFT 39
#define PXE_PER_PAGE 512
#define VIRTUAL_ADDRESS_BITS 48
#define PXI_MASK (PXE_PER_PAGE - 1)
#define VIRTUAL_ADDRESS_MASK ((((ULONG_PTR)1) << VIRTUAL_ADDRESS_BITS) - 1)

#define MiGetPxeOffset(va) \
    ((ULONG)(((ULONG_PTR)(va) >> PXI_SHIFT) & PXI_MASK))

#define MiGetPxeAddress(va)   \
    ((PTE*)g_pxe_base + MiGetPxeOffset(va))

#define MiGetPpeAddress(va)   \
    ((PTE*)(((((ULONG_PTR)(va) & VIRTUAL_ADDRESS_MASK) >> PPI_SHIFT) << PTE_SHIFT) + g_ppe_base))

#define MiGetPdeAddress(va) \
    ((PTE*)(((((ULONG_PTR)(va) & VIRTUAL_ADDRESS_MASK) >> PDI_SHIFT) << PTE_SHIFT) + g_pde_base))

#define MiGetPteAddress(va) \
    ((PTE*)(((((ULONG_PTR)(va) & VIRTUAL_ADDRESS_MASK) >> PTI_SHIFT) << PTE_SHIFT) + g_pte_base))


DECLSPEC_NOINLINE
bool FixPage(PVOID addr)
{
	typedef NTSTATUS(__fastcall* MmLoadFn)(
		IN  ULONG_PTR FaultStatus,
		IN  PVOID VirtualAddress,
		IN  KPROCESSOR_MODE PreviousMode,
		IN  PVOID TrapInformation);

	static MmLoadFn mmac = 0;
	if (!mmac) {
		mmac = (MmLoadFn)RVA(FindPatternSect2(Kbase2, ".text", "E8 ? ? ? ? 8B D8 40 84 ? 74 0C"), 5);
	}

	//sp("fix");

	return NT_SUCCESS(mmac(FALSE, addr, KernelMode, nullptr));
};

bool IsAddrValid(u64 addr, bool fixPage = false)
{
	//get dirs ptrs
	auto pxe = MiGetPxeAddress(addr);
	auto ppe = MiGetPpeAddress(addr);
	auto pde = MiGetPdeAddress(addr);
	auto pte = MiGetPdeAddress(addr);

	//get, fix pml4e
	if (!pxe->Valid && (!fixPage || !FixPage(pxe)))
		return false;

	//get, fix pdpte
	if (!ppe->Valid && (!fixPage || !FixPage(ppe)))
		return false;

	//get, fix pde
	if (!pde->Valid && (!fixPage || !FixPage(pde)))
		return false;

	//large page //2MB
	if (pde->LargePage)
		return (bool)(!fixPage || /*fix page*/FixPage((pv64)addr));
	
	//get, fix pte
	if (!pte->Valid && (!fixPage || !FixPage(pte)))
		return false;

	//small page //4KB
	return (bool)(!fixPage || /*fix page*/FixPage((pv64)addr)); 
}

bool IsExec(u64 addr) {
	if (!IsAddrValid(addr))
		return false;

	auto pte =  MiGetPteAddress(addr);
	return !pte->NoExecute;
}

u64 ScanUserMemory(u64(*p)(u64))
{
	//scan pml4
	for (int a = 0; a < 512; ++a) {
		VIRT_ADDR gg{ 0 }; gg.s.pml4_index = a;
		auto pml4e = MiGetPxeAddress(gg.value);
		if (pml4e->Valid && pml4e->Owner) { //valid & only usermode
			//scan pdpt
			for (int b = 0; b < 512; ++b) {
				gg.s.pdpt_index = b;
				auto pdpte = MiGetPpeAddress(gg.value);
				if (pdpte->Valid && pdpte->Owner) { //valid & only usermode
					//scan pde
					for (int c = 0; c < 512; ++c) {
						gg.s.pd_index = c;
						auto pde = MiGetPdeAddress(gg.value);
						if (pde->Valid && pde->Owner) { //valid & only usermode
							if (pde->LargePage) {
								auto ret = p(gg.value);
								if (ret) return ret;
							}

							else {
								//scan pte
								for (int d = 0; d < 512; ++d) {
									gg.s.pt_index = d;
									auto pte = MiGetPteAddress(gg.value);
									if (pte->Valid && pte->Owner) { //valid & only usermode
										auto ret = p(gg.value);
										if (ret) return ret;
									}
								}
							}
						}
					}
				}
			}
		}
	};

	//not found
	return 0;
}

class FMemV2
{
private:
	inline static _IDTR IdtNorm;
	inline static _IDTR IdtPatch;

	ULONG64 SetIdtAddr(ULONG64 IdtBaseAddr, UCHAR Index, ULONG64 NewAddr)
	{
		PIDT_ENTRY Pidt = (PIDT_ENTRY)(IdtBaseAddr);
		Pidt = Pidt + Index;
		ULONG64 OffsetHigh, OffsetMiddle, OffsetLow, ret;

		OffsetHigh = Pidt->idt.OffsetHigh;
		OffsetHigh = OffsetHigh << 32;
		OffsetMiddle = Pidt->idt.OffsetMiddle;
		OffsetMiddle = OffsetMiddle << 16;

		OffsetLow = Pidt->idt.OffsetLow;
		ret = OffsetHigh + OffsetMiddle + OffsetLow;
		Pidt->idt.OffsetHigh = NewAddr >> 32;
		Pidt->idt.OffsetMiddle = NewAddr << 32 >> 48;
		Pidt->idt.OffsetLow = NewAddr & 0xFFFF;
		return ret;
	}

	static naked void HandlePF() {
		__asm {
			add rsp, 8
			add qword ptr [rsp], 7
			iretq
		}
	}

public:
	void Init() {
		_asm { sidt IdtNorm }
		auto newIdtEntry = (PIDT_ENTRY)KAlloc(0x1000);
		memcpy(newIdtEntry, (pv64)IdtNorm.Base, 0x1000);
		SetIdtAddr((u64)newIdtEntry, 0xE/*#PF*/, (u64)HandlePF);
		IdtPatch.Base = (u64)newIdtEntry; IdtPatch.Limit = 0xFFF;
	}

	inl bool MemCpy(pv64 dst, pv64 src, int size) 
	{
		_asm {
			pushfq
			cli
			lidt IdtPatch
		}

		__asm__ __volatile__("rep movsb" : "+D"(dst), "+S"(src), "+c"(size) :: "memory");

		_asm {
			mov byte ptr [rsp + 4], 1
			lidt IdtNorm
			popfq
		}

		bool state;
		__asm__ __volatile__("mov -4(%%rsp), %0" : "=r"(state));
		return state;
	}

	inl void/*bool*/ ReadArr(uintptr_t addr, void* arr, size_t size) {
		MemCpy(arr, (pv64)addr, size);
		// return 1;
	}

	template<typename ReadType>
	inl ReadType Read(uintptr_t addr, size_t forceSize = 0) {
		ReadType ReadData{};
		ReadArr(addr, &ReadData, forceSize ? forceSize : sizeof(ReadType));
		return ReadData;
	}

	inl void/*bool*/ WriteArr(uintptr_t addr, void* arr, size_t size) {
		MemCpy((pv64)addr, arr, size);
		//return 1;
	}

	template<typename WriteType>
	inl void/*bool*/ Write(uintptr_t addr, const WriteType& data, size_t forceSize = 0) {
		return WriteArr(addr, (void*)&data, forceSize ? forceSize : sizeof(WriteType));
	}
} fmem;