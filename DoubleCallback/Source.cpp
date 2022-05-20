#include "Global.h"

NTSTATUS DriverEntry(PVOID a1, PVOID KBase)
{
	Kbase2 = KBase;

	ImpSet(IoIs32bitProcess);
	ImpSet(ZwProtectVirtualMemory);
	ImpSet(PsGetProcessWow64Process);
	ImpSet(KeRegisterBoundCallback);
	ImpSet(KeDeregisterBoundCallback);
	ImpSet(ZwQueryObject);
	ImpSet(ExAllocatePoolWithTag);
	ImpSet(ExFreePoolWithTag);
	ImpSet(IoGetCurrentProcess);
	ImpSet(KeAttachProcess);
	ImpSet(KeDelayExecutionThread);
	ImpSet(KeDetachProcess);
	ImpSet(KeEnterGuardedRegion);
	ImpSet(KeLeaveGuardedRegion);
	ImpSet(KeQueryAuxiliaryCounterFrequency);
	ImpSet(KeUserModeCallback);
	ImpSet(MmIsAddressValid);
	ImpSet(ObfDereferenceObject);
	ImpSet(PsAcquireProcessExitSynchronization);
	ImpSet(PsGetProcessPeb);
	ImpSet(PsLookupProcessByProcessId);
	ImpSet(PsLookupThreadByThreadId);
	ImpSet(PsReleaseProcessExitSynchronization);
	ImpSet(RtlCreateUserThread);
	ImpSet(ZwAllocateVirtualMemory);
	ImpSet(ZwClose);
	ImpSet(ZwFreeVirtualMemory);
	ImpSet(ZwQuerySystemInformation);
	ImpSet(ZwQueryVirtualMemory);
	ImpSet(MmSecureVirtualMemory);
	ImpSet(ZwSetInformationVirtualMemory);
	ImpSet(MmGetVirtualForPhysical);
	ImpSet(PsCreateSystemThread);
	ImpSet(PsGetCurrentThreadId);
	ImpSet(IoAllocateMdl);
	ImpSet(MmBuildMdlForNonPagedPool);
	ImpSet(MmMapLockedPages);
	ImpSet(MmUnmapLockedPages);
	ImpSet(IoFreeMdl);

	InitializePteBase(__readcr3());
	
	DwmInit();

	//sp("Strong OK!!!");
	return STATUS_SUCCESS;
}