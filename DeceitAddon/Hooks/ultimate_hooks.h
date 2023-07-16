#pragma once
#include <Windows.h>
#include <vector>
#include <VersionHelpers.h>
#include <string>
#include <stddef.h>
//#include "../Utils.h"

#pragma warning(4:4596)

#define VP_Offset_old (0x170)
#define VP_Offset (0x180)

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

struct HookDetails {
	DWORD64 hookAddress;
	DWORD64 addressToHook;
};

struct HookEntries
{
	std::vector<HookDetails> hookDetails;
	DWORD64 addressToHookOldProtect;
	DWORD64 addressToHookMbiStart;
	DWORD64 addressToHookMbiEnd;
	DWORD64 addressToHookMbiSize;
	DWORD64 allocatedAddressStart;
	DWORD64 allocatedAddressEnd;
	DWORD64 allocatedAddressSize;
	DWORD64 addressToHookoffsetFromStart;
};

struct EzHook {
	bool hooked;
	bool JMP;
	DWORD64 address;
	size_t hookSize;

	DWORD64 hookFunction;
	DWORD64 origFunc;
};

LONG WINAPI LeoHandler(EXCEPTION_POINTERS* pExceptionInfo);

class UltimateHooks {
public:
	template <class fnType>
	bool DEPAddHook(DWORD64 Address, DWORD64 hk_Address, fnType& OldAddress, size_t Size, PVOID& Allocation, uint8_t Offset) {
		DWORD64 NewOnprocessSpellAddr = VirtualAllocateRegion(Allocation, Address, Size);
		CopyRegion((DWORD64)Allocation, Address, Size);
		FixFuncRellocation(Address, (Address + Size), (DWORD64)Allocation, Size);
		OldAddress = (fnType)(NewOnprocessSpellAddr);
		auto res = addHook(Address, (DWORD64)hk_Address, Offset);
		//MessageBoxA(0, to_hex((int)Allocation).c_str(), to_hex((int)Address).c_str(), 0);
		return res;
	}
	bool deinit();
	DWORD64 RestoreRtlAddVectoredExceptionHandler();
	DWORD64 RestoreZwQueryInformationProcess();
	DWORD64 RestoreNtProtectVirtualMemory();
	bool RestoreSysDll(std::string name);

	uintptr_t AddEzHook(uintptr_t target, size_t hookSize, void* hook);
	DWORD64 AddEzHook(uintptr_t target, size_t hookSize, uintptr_t hook);
	DWORD64 RemoveEzHook(DWORD64 target);

	ULONG ProtectVirtualMemory(PVOID addr, PSIZE_T size, ULONG protect, PULONG old);
private:
	bool IsDoneInit = false;
	PVOID VEH_Handle = nullptr;

	DWORD64 VirtualAllocateRegion(PVOID& NewFunction, DWORD64 OrigAddress, size_t size);
	void CopyRegion(DWORD64 dest, DWORD64 source, size_t size);
	void FixFuncRellocation(DWORD64 OldFnAddress, DWORD64 OldFnAddressEnd, DWORD64 NewFnAddress, size_t size);
	bool addHook(DWORD64 address, DWORD64 hkAddress, size_t offset);
	bool Hook(DWORD64 original_fun, DWORD64 hooked_fun, size_t offset);
	void FixRellocation(DWORD64 OldFnAddress, DWORD64 OldFnAddressEnd, DWORD64 NewFnAddress, size_t size, size_t _offset);
};
extern UltimateHooks UltHook;