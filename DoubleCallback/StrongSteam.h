typedef NTSTATUS(__fastcall* ZwReleaseMutantFn)(HANDLE, PLONG);

struct StreamHeader
{
	DWORD BufferStartIndex;
	DWORD BufferEndIndex;
	DWORD BufferCapacity;
	DWORD BufferSize;
};

class OSRender
{
private:
	PVOID OverlayBase;
	PVOID PaintBuffer;
	HANDLE InputMutex;
	HANDLE PaintCmdMutex;
	HANDLE InputAvailable;
	ZwReleaseMutantFn ZwReleaseMutant;

public:
	void OSRender1(PVOID OverlayBase1) {
		OverlayBase = OverlayBase1;
		ZwReleaseMutant = (ZwReleaseMutantFn)RVA(FindPatternSect(Kbase2, "PAGE", "48 83 F9 FF 74 07") + 8, 5);
		auto paintCmd = csgoProc.HazeDumper(gor_Dll, "A3 ? ? ? ? 8D 45 B4", { 1, 0 });
			//(//*(ULONG64*)(RVA(FindPatternSect(OverlayBase, ".text", "48 89 05 ? ? ? ? BA ? ? ? ? 48"), 7));
		auto inputCmd = csgoProc.HazeDumper(gor_Dll, "8B 0D ? ? ? ? 8D 55 F8", { 2, 0 });
			//*(ULONG64*)(RVA(FindPatternSect(OverlayBase, ".text", "48 8B 0D ? ? ? ? 41 B8 ? ? ? ? 48 8D"), 7));
	
		PaintCmdMutex = (HANDLE)*(u32*)(*(u32*)(paintCmd + 0x104/*0x108*/) + 8/*0x10*/);
		PaintBuffer = (HANDLE)*(u32*)(*(u32*)(paintCmd + 0x108/*0x110*/) + 8/*0x10*/);
		InputMutex = (HANDLE)*(u32*)(*(u32*)(inputCmd + 0x104/*0x108*/) + 8/*0x10*/);
		InputAvailable = (HANDLE)*(u32*)(*(u32*)(inputCmd + 0x110/*0x120*/) + 8/*0x10*/);
	}

	bool WaitLockFrame() {
		auto Header = (StreamHeader*)PaintBuffer;
		if (Header->BufferSize > 8)
			return false;
			
		HANDLE handles[] = { PaintCmdMutex, InputMutex, InputAvailable };
		LARGE_INTEGER time; time.QuadPart = 0;
		ZwWaitForMultipleObjects(3, handles, WaitAll, 0, nullptr);
		return true;
	}

	void PutCmd(const void* source, DWORD size)
	{
		
		auto Header = (StreamHeader*)PaintBuffer;
		auto Buffer = (char*)Header + sizeof(StreamHeader);

		//if (source == nullptr)
		//{
		//}	//throw std::invalid_argument("Source is null.");
		//
		//if (size > GetUnusedCapacity())
		//{
		//}	//throw std::out_of_range("Size exceeds the unused capacity of the buffer.");

		char* destination;

		//dp(Header->BufferSize);

		Header->BufferSize += size;

		// write is wrapped
		if (size > Header->BufferStartIndex)
		{
			DWORD postwrapSize = size - Header->BufferStartIndex;
			// update size
			size = Header->BufferStartIndex;
			// wrap buffer start index
			Header->BufferStartIndex = Header->BufferCapacity - postwrapSize;

			memcpy(Buffer + Header->BufferStartIndex, source, postwrapSize);
			// update source
			source = (char*)source + postwrapSize;
			// finish write at start
			destination = Buffer;
		}

		else
		{
			Header->BufferStartIndex -= size;
			destination = Buffer + Header->BufferStartIndex;
		}

		memcpy(destination, source, size);
	}

	void ReleaseFrame() {
		ZwReleaseMutant(InputMutex, 0);
		ZwReleaseMutant(PaintCmdMutex, 0);
	}
};
