# DxgkPresentHook
Probably supports all of the old windows 10 to 21H2.
This function can be used as an alternative to the DxgkSubmitCommand hook. If you're going to render something, this hook would be more suitable.

# CPP
```cpp
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
```

## Assembly
```asm
DxgkPresentHook PROC
	push r9
	push r8
	push rdx
	push rcx
	push rbx
	push rsi
	push rdi
	call DxgkPresentHook_internal
	pop rdi
	pop rsi
	pop rbx
	pop rcx
	pop rdx
	pop r8
	pop r9
	mov [rsp+0010h],rbx
	mov [rsp+0018h],rsi
	push rdi
	push r12
	push r13
	push r14
	push r15
	;sub rsp, 00000290h
	jmp qword ptr [original_DxgkPresent]
DxgkPresentHook ENDP
```
