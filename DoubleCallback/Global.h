//#pragma comment(linker, "/MERGE:.rdata=INIT") //hz
//#pragma comment(linker, "/MERGE:.pdata=INIT")

#define _AMD64_ 1
#define _KERNEL_MODE 1
extern "C" int _fltused = 0;

#include <ntifs.h>
#include <windef.h>
#include <stdint.h>
#include <functional>
#include <bitset>
#include <tuple>

PVOID Kbase3;
volatile PVOID Kbase2;
volatile ULONG64 Kbase333;

#include "Internals.h"
#include "FIntrin.h"

extern "C" {
	volatile __int64 dwmRet_;
	volatile __int64 dwmThId_;
	volatile __int64 dwmHook_;

	void __fixcall(u64* ctx, NTSTATUS);
	void __fix2(u64* ctx);
	void __fix3(); 
	void __fixGG();

	//NTSTATUS kDwmPresent(PVOID, REG_NOTIFY_CLASS Argument1, PVOID Argument2);
}

//#include "CPU_Stop.h"

#include "Math.h"

#include "CryptSTR.h"
#include "CRT.h"
#include "HideImport.h"
#include "Helpers.h"
#include "callBack.h"
//#include "KGDI.h"
#include "LegacyRender.h"
#include "StrongDwmRender.h"

//#include "VmtHook.h"
//#include "BoundHook.h"