#define BB_POOL_TAG 'enoB'

BOOLEAN BBSkipThread(IN PETHREAD pThread, IN BOOLEAN isWow64)
{
    PUCHAR pTeb64 = (PUCHAR)PsGetThreadTeb(pThread);
    if (!pTeb64)
        return TRUE;

    // Skip GUI treads. APC to GUI thread causes ZwUserGetMessage to fail
    // TEB64 + 0x78  = Win32ThreadInfo
    if (*(PULONG64)(pTeb64 + 0x78) != 0)
        return TRUE;

    // Skip threads with no ActivationContext
    // Skip threads with no TLS pointer
    if (isWow64)
    {
        PUCHAR pTeb32 = pTeb64 + 0x2000;

        // TEB32 + 0x1A8 = ActivationContextStackPointer
        if (*(PULONG32)(pTeb32 + 0x1A8) == 0)
            return TRUE;

        // TEB64 + 0x2C = ThreadLocalStoragePointer
        if (*(PULONG32)(pTeb32 + 0x2C) == 0)
            return TRUE;
    }
    else
    {
        // TEB64 + 0x2C8 = ActivationContextStackPointer
        if (*(PULONG64)(pTeb64 + 0x2C8) == 0)
            return TRUE;

        // TEB64 + 0x58 = ThreadLocalStoragePointer
        if (*(PULONG64)(pTeb64 + 0x58) == 0)
            return TRUE;
    }

    return FALSE;
}

NTSTATUS BBLookupProcessThread(IN PEPROCESS pProcess, OUT PETHREAD* ppThread)
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE pid = PsGetProcessId(pProcess);
    PVOID pBuf = KAlloc(1024 * 1024);//ExAllocatePoolWithTag(NonPagedPool, 1024 * 1024, BB_POOL_TAG);
    PSYSTEM_PROCESS_INFO pInfo = (PSYSTEM_PROCESS_INFO)pBuf;

    ASSERT(ppThread != NULL);
    if (ppThread == NULL)
        return STATUS_INVALID_PARAMETER;

    if (!pInfo)
    {
        //DPRINT("BlackBone: %s: Failed to allocate memory for process list\n", __FUNCTION__);
        return STATUS_NO_MEMORY;
    }

    // Get the process thread list
    status = ZwQuerySystemInformation(SystemProcessInformation, pInfo, 1024 * 1024, NULL);
    if (!NT_SUCCESS(status)) {
        KFree(pBuf);
        return status;
    }

    // Find target thread
    if (NT_SUCCESS(status))
    {
        status = STATUS_NOT_FOUND;
        for (;;)
        {
            if (pInfo->UniqueProcessId == pid)
            {
                status = STATUS_SUCCESS;
                break;
            }
            else if (pInfo->NextEntryOffset)
                pInfo = (PSYSTEM_PROCESS_INFO)((PUCHAR)pInfo + pInfo->NextEntryOffset);
            else
                break;
        }
    }

    BOOLEAN wow64 = PsGetProcessWow64Process(pProcess) != NULL;

    // Reference target thread
    if (NT_SUCCESS(status))
    {
        status = STATUS_NOT_FOUND;

        // Get first thread
        for (ULONG i = 0; i < pInfo->NumberOfThreads; i++)
        {
            // Skip current thread
            if (/*pInfo->Threads[i].WaitReason == Suspended ||
                 pInfo->Threads[i].ThreadState == 5 ||*/
                pInfo->Threads[i].ClientId.UniqueThread == PsGetCurrentThreadId())
            {
                continue;
            }

            status = PsLookupThreadByThreadId(pInfo->Threads[i].ClientId.UniqueThread, ppThread);

            // Skip specific threads
            if (*ppThread && BBSkipThread(*ppThread, wow64))
            {
                ObDereferenceObject(*ppThread);
                *ppThread = NULL;
                continue;
            }

            break;
        }
    }
    //else
    //   DPRINT("BlackBone: %s: Failed to locate process\n", __FUNCTION__);

    if (pBuf)
        KFree(pBuf);//ExFreePoolWithTag(pBuf, BB_POOL_TAG);

    // No suitable thread
    if (!*ppThread)
        status = STATUS_NOT_FOUND;

    return status;
}

VOID KernelApcInjectCallback(
    PKAPC Apc,
    PKNORMAL_ROUTINE* NormalRoutine,
    PVOID* NormalContext,
    PVOID* SystemArgument1,
    PVOID* SystemArgument2
)
{
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    //DPRINT( "BlackBone: %s: Called. NormalRoutine = 0x%p\n", __FUNCTION__, *NormalRoutine );

    // Skip execution
    if (PsIsThreadTerminating(PsGetCurrentThread()))
        *NormalRoutine = NULL;

    // Fix Wow64 APC
    if (PsGetCurrentProcessWow64Process() != NULL)
        PsWrapApcWow64Thread(NormalContext, (PVOID*)NormalRoutine);

    ExFreePoolWithTag(Apc, BB_POOL_TAG);
}

VOID KernelApcPrepareCallback(
    PKAPC Apc,
    PKNORMAL_ROUTINE* NormalRoutine,
    PVOID* NormalContext,
    PVOID* SystemArgument1,
    PVOID* SystemArgument2
)
{
    UNREFERENCED_PARAMETER(NormalRoutine);
    UNREFERENCED_PARAMETER(NormalContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    //DPRINT( "BlackBone: %s: Called\n", __FUNCTION__ );

    // Alert current thread
    KeTestAlertThread(UserMode);
    ExFreePoolWithTag(Apc, BB_POOL_TAG);
}

NTSTATUS BBQueueUserApc(
    IN PETHREAD pThread,
    IN PVOID pUserFunc,
    IN PVOID Arg1,
    IN PVOID Arg2,
    IN PVOID Arg3,
    IN BOOLEAN bForce
)
{
    ASSERT(pThread != NULL);
    if (pThread == NULL)
        return STATUS_INVALID_PARAMETER;

    // Allocate APC
    PKAPC pPrepareApc = NULL;
    PKAPC pInjectApc = (PKAPC)ExAllocatePoolWithTag(NonPagedPool, sizeof(KAPC), BB_POOL_TAG);

    if (pInjectApc == NULL)
    {
        //DPRINT("BlackBone: %s: Failed to allocate APC\n", __FUNCTION__);
        return STATUS_NO_MEMORY;
    }

    // Actual APC
    KeInitializeApc(
        pInjectApc, (PKTHREAD)pThread,
        OriginalApcEnvironment, &KernelApcInjectCallback,
        NULL, (PKNORMAL_ROUTINE)(ULONG_PTR)pUserFunc, UserMode, Arg1
    );

    // Setup force-delivery APC
    if (bForce)
    {
        pPrepareApc = (PKAPC)ExAllocatePoolWithTag(NonPagedPool, sizeof(KAPC), BB_POOL_TAG);
        KeInitializeApc(
            pPrepareApc, (PKTHREAD)pThread,
            OriginalApcEnvironment, &KernelApcPrepareCallback,
            NULL, NULL, KernelMode, NULL
        );
    }

    // Insert APC
    if (KeInsertQueueApc(pInjectApc, Arg2, Arg3, 0))
    {
        if (bForce && pPrepareApc)
            KeInsertQueueApc(pPrepareApc, NULL, NULL, 0);

        return STATUS_SUCCESS;
    }
    else
    {
        //DPRINT("BlackBone: %s: Failed to insert APC\n", __FUNCTION__);

        ExFreePoolWithTag(pInjectApc, BB_POOL_TAG);

        if (pPrepareApc)
            ExFreePoolWithTag(pPrepareApc, BB_POOL_TAG);

        return STATUS_NOT_CAPABLE;
    }
}