void DxgkPresentHook_internal()
{
	if (!Memory::IsTargetProcess())
	{
		return;
	}
  
	//
	// Write your code here
	//
	
	return;
}

void InstallDxgkPresentHook()
{
	BYTE hook[] =
	{
		0x48, 0xB8, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, //mov rax, dxgkrnl.DxgkPresent
		0xFF, 0xE0 //jmp rax
	};
	*(__int64*)(hook + 2) = (__int64)DxgkPresentHook;
  
	original_DxgkPresent = (__int64)((__int64)func + 0x13);
	auto dxgkrnl = GetKernelModuleBase("dxgkrnl.sys");
	auto func = GetProcAddress(dxgkrnl, "DxgkPresent");
	PMDL pMdl = IoAllocateMdl(func, 16, FALSE, FALSE, NULL);
	if (pMdl == NULL)
	{
		return;
	}
		
	MmProbeAndLockPages(pMdl, KernelMode, IoReadAccess);
	PVOID MappingData = MmMapLockedPagesSpecifyCache(pMdl, KernelMode, MmNonCached, NULL, FALSE, NormalPagePriority);
	if (MappingData == NULL)
	{

		MmUnlockPages(pMdl);
		IoFreeMdl(pMdl);
		return false;
	}

	if (!NT_SUCCESS(MmProtectMdlSystemAddress(pMdl, PAGE_READWRITE)))
	{

		MmUnmapLockedPages(MappingData, pMdl);
		MmUnlockPages(pMdl);
		IoFreeMdl(pMdl);
		return;
	}

	RtlCopyMemory(MappingData, hook, 12);

	MmUnmapLockedPages(MappingData, pMdl);
	MmUnlockPages(pMdl);
	IoFreeMdl(pMdl);
	return;
}

bool FindProcess()
{	
	UNICODE_STRING TargetImageName = RTL_CONSTANT_STRING(L"d3d11.exe");
	PSYSTEM_PROCESS_INFO Spi = (PSYSTEM_PROCESS_INFO)NQSI(SystemProcessInformation);
	if (void* Buffer = Spi)
	{
		while (Spi->NextEntryOffset)
		{
			if (!RtlCompareUnicodeString(&Spi->ImageName, &TargetImageName, FALSE))
			{
				PEPROCESS Process = nullptr;
				PsLookupProcessByProcessId(Spi->UniqueProcessId, &Process);
				if (Process)
				{
					KAPC_STATE apc{};
					KeStackAttachProcess(Process, &apc);
					InstallDxgkPresentHook();
					KeUnstackDetachProcess(&apc);
					ObDereferenceObject(Process);
					ExFreePool(Buffer);
					return true;
				}
			}

			Spi = PSYSTEM_PROCESS_INFO((char*)Spi + Spi->NextEntryOffset);
		}

		ExFreePool(Buffer);
	}
	return false;
}

NTSTATUS DriverEntry()
{
	while (true)
	{
		if (FindProcess())
			break;
	}

	return STATUS_SUCCESS;
}
