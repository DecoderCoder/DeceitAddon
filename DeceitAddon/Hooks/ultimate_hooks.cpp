#define USE_ZIDYS 0
#define DEBUG_RELLOCATION 0

#include "ultimate_hooks.h"
#include "makesyscall.h"
#include <thread>
//#include "../Utils.h"
#include "psapi.h"
#include "winternl.h"
#if USE_ZIDYS || DEBUG_RELLOCATION
#include "Zydis/Zydis.h"
#endif
std::vector<HookEntries> hookEntries;
UltimateHooks UltHook;
uintptr_t RtlInterlockedCompareExchange64Offst;
std::vector<EzHook> EzHooks;

//относительная кодировка E9 jmp используется следующим образом :
//
//CURRENT_RVA: jmp(DESTINATION_RVA - CURRENT_RVA - 5[sizeof(E9 xx xx xx xx)])

uintptr_t UltimateHooks::AddEzHook(uintptr_t target, size_t hookSize, void* hook) {
	return this->AddEzHook(target, hookSize, (uintptr_t)hook);
}

uintptr_t UltimateHooks::AddEzHook(uintptr_t target, size_t hookSize, uintptr_t hook) {
	if (hookSize > 1024 || hookSize < 12)
		return -1;
	DWORD oldProt;

	EzHook ezHook;
	ezHook.hooked = false;
	ezHook.hookSize = hookSize;
	ezHook.hookFunction = hook;

	uintptr_t allocation = (uintptr_t)VirtualAlloc(NULL, 1024, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (allocation == 0)
		return -1;


	memcpy((void*)allocation, (void*)target, hookSize);

	char jmp[] = { 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0xC3 };
	*((uintptr_t*)&jmp[2]) = (uintptr_t)(target + hookSize);
	memcpy((char*)((uintptr_t)allocation + hookSize), &jmp, sizeof(jmp));

	*((uintptr_t*)&jmp[2]) = (uintptr_t)hook;
	VirtualProtect((void*)target, hookSize, 0x40, &oldProt);
	memcpy((char*)((uintptr_t)target), &jmp, sizeof(jmp));
	VirtualProtect((void*)target, hookSize, oldProt, &oldProt);

	return allocation;

}

uintptr_t UltimateHooks::RemoveEzHook(uintptr_t origFunction) {
	for (auto obj : EzHooks) {
		if (obj.origFunc == origFunction) {
			DWORD oldProt;
			VirtualProtect((void*)obj.address, obj.hookSize, PAGE_EXECUTE_READWRITE, &oldProt);
			memcpy((void*)obj.address, (void*)obj.origFunc, obj.hookSize);
			VirtualProtect((void*)obj.address, obj.hookSize, oldProt, &oldProt);
			return true;
		}
	}
	return false;
}

bool inRange(unsigned low, unsigned high, unsigned x)
{
	return  ((x - low) <= (high - low));
}

bool memory_readable(void* ptr, size_t byteCount)
{
	MEMORY_BASIC_INFORMATION mbi;
	if (VirtualQuery(ptr, &mbi, sizeof(MEMORY_BASIC_INFORMATION)) == 0)
		return false;

	if (mbi.State != MEM_COMMIT)
		return false;

	if (mbi.Protect == PAGE_NOACCESS || mbi.Protect == PAGE_EXECUTE)
		return false;

	// This checks that the start of memory block is in the same "region" as the
	// end. If it isn't you "simplify" the problem into checking that the rest of 
	// the memory is readable.
	size_t blockOffset = (size_t)((char*)ptr - (char*)mbi.AllocationBase);
	size_t blockBytesPostPtr = mbi.RegionSize - blockOffset;

	if (blockBytesPostPtr < byteCount)
		return memory_readable((char*)ptr + blockBytesPostPtr,
			byteCount - blockBytesPostPtr);

	return true;
}

LONG __stdcall LeoHandler(EXCEPTION_POINTERS* pExceptionInfo)
{
	if (pExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && pExceptionInfo->ExceptionRecord->ExceptionInformation[0] == 8)
	{
		for (HookEntries hs : hookEntries)
		{
			for (HookDetails hd : hs.hookDetails)
			{
				if (hd.addressToHook == pExceptionInfo->ContextRecord->Rip) {
					pExceptionInfo->ContextRecord->Rip = hd.hookAddress;
					return EXCEPTION_CONTINUE_EXECUTION;
				}
			}
			if (inRange(hs.addressToHookMbiStart - 0x1000, hs.addressToHookMbiEnd + 0x1000, pExceptionInfo->ContextRecord->Rip)) {
				int offset = pExceptionInfo->ContextRecord->Rip - hs.addressToHookMbiStart;
				pExceptionInfo->ContextRecord->Rip = static_cast<uintptr_t>(hs.allocatedAddressStart + offset);
				return EXCEPTION_CONTINUE_EXECUTION;
			}
		}
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

bool UltimateHooks::deinit()
{
	uintptr_t old;
	if (VEH_Handle)
	{
		if (RemoveVectoredExceptionHandler(VEH_Handle))
		{

			for (HookEntries hs : hookEntries)
			{
				for (HookDetails hd : hs.hookDetails)
				{
					auto addr = (PVOID)hd.addressToHook;
					auto size = static_cast<SIZE_T>(static_cast<int>(1));
					if (NT_SUCCESS(
						makesyscall<NTSTATUS>(0x50, 0x00, 0x00, 0x00, "RtlInterlockedCompareExchange64", RtlInterlockedCompareExchange64Offst, 0xC2, 0x14,
							0x00)(GetCurrentProcess(), &addr, &size, hs.addressToHookOldProtect, &old)))
					{
					}
				}
			}
			hookEntries.clear();
			return true;
		}
	}

	return false;
}

ULONG UltimateHooks::ProtectVirtualMemory(PVOID addr, PSIZE_T size, ULONG protect, PULONG old) {
	typedef ULONG(WINAPI* TNtProtectVirtualMemory) (HANDLE, PVOID, PSIZE_T, ULONG, PULONG);
	TNtProtectVirtualMemory xNtProtectVirtualMemory;
	xNtProtectVirtualMemory = (TNtProtectVirtualMemory)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtProtectVirtualMemory");
	HMODULE jo = GetModuleHandleA("ntdll.dll");
	return xNtProtectVirtualMemory(GetCurrentProcess(), addr, size, protect, old);
}

uintptr_t UltimateHooks::RestoreNtProtectVirtualMemory() {
	HMODULE ntdll = GetModuleHandleA("ntdll.dll");

	uintptr_t NtProtectVirtualMemoryAddr = reinterpret_cast<uintptr_t>(
		GetProcAddress(ntdll, "NtProtectVirtualMemory"));

	BYTE ZwQIP[] = {
		0xB8, 0x50, 0x00, 0x00, 0x00
	};

	//memcpy((void*)NtProtectVirtualMemoryAddr, &ZwQIP, sizeof(ZwQIP));

	int i = 0;
	DWORD old = 0;

	VirtualProtect((LPVOID)NtProtectVirtualMemoryAddr, sizeof(ZwQIP), 0x40, &old);

	for (BYTE _byte : ZwQIP) {
		*(BYTE*)(NtProtectVirtualMemoryAddr + i) = _byte;
		i++;
	}
	VirtualProtect((LPVOID)NtProtectVirtualMemoryAddr, sizeof(ZwQIP), old, &old);
	return NtProtectVirtualMemoryAddr;
}

bool UltimateHooks::RestoreSysDll(string name)
{
	bool restored = false;
	HANDLE process = GetCurrentProcess();
	MODULEINFO mi = {};
	HMODULE ntdllModule = GetModuleHandleA(name.c_str());

	GetModuleInformation(process, ntdllModule, &mi, sizeof(mi));
	LPVOID ntdllBase = (LPVOID)mi.lpBaseOfDll;
	HANDLE ntdllFile = CreateFileA(("c:\\windows\\system32\\" + name).c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	HANDLE ntdllMapping = CreateFileMapping(ntdllFile, NULL, PAGE_READONLY | SEC_IMAGE, 0, 0, NULL);
	LPVOID ntdllMappingAddress = MapViewOfFile(ntdllMapping, FILE_MAP_READ, 0, 0, 0);

	PIMAGE_DOS_HEADER hookedDosHeader = (PIMAGE_DOS_HEADER)ntdllBase;
	PIMAGE_NT_HEADERS hookedNtHeader = (PIMAGE_NT_HEADERS)((uintptr_t)ntdllBase + hookedDosHeader->e_lfanew);

	for (WORD i = 0; i < hookedNtHeader->FileHeader.NumberOfSections; i++) {
		PIMAGE_SECTION_HEADER hookedSectionHeader = (PIMAGE_SECTION_HEADER)((uintptr_t)IMAGE_FIRST_SECTION(hookedNtHeader) + ((uintptr_t)IMAGE_SIZEOF_SECTION_HEADER * i));

		if (!strcmp((char*)hookedSectionHeader->Name, (char*)".text")) {
			DWORD oldProtection = 0;
			bool isProtected = VirtualProtect((LPVOID)((uintptr_t)ntdllBase + (uintptr_t)hookedSectionHeader->VirtualAddress), hookedSectionHeader->Misc.VirtualSize, PAGE_EXECUTE_READWRITE, &oldProtection);
			memcpy((LPVOID)((uintptr_t)ntdllBase + (uintptr_t)hookedSectionHeader->VirtualAddress), (LPVOID)((uintptr_t)ntdllMappingAddress + (uintptr_t)hookedSectionHeader->VirtualAddress), hookedSectionHeader->Misc.VirtualSize);
			isProtected = VirtualProtect((LPVOID)((uintptr_t)ntdllBase + (uintptr_t)hookedSectionHeader->VirtualAddress), hookedSectionHeader->Misc.VirtualSize, oldProtection, &oldProtection);
			restored = true;
		}
	}

	CloseHandle(process);
	CloseHandle(ntdllFile);
	CloseHandle(ntdllMapping);
	FreeLibrary(ntdllModule);
	return restored;
}

uintptr_t UltimateHooks::RestoreRtlAddVectoredExceptionHandler() {
	HMODULE ntdll = GetModuleHandleA("ntdll.dll");

	uintptr_t RtlAddVectoredExceptionHandlerAddr = reinterpret_cast<uintptr_t>(
		GetProcAddress(ntdll, "RtlAddVectoredExceptionHandler"));

	uintptr_t RtlInterlockedCompareExchange64Addr = reinterpret_cast<uintptr_t>(
		GetProcAddress(ntdll, "RtlInterlockedCompareExchange64"));

	if (*(BYTE*)(RtlInterlockedCompareExchange64Addr + VP_Offset) == (BYTE)0xFF && *(BYTE*)(RtlInterlockedCompareExchange64Addr + VP_Offset + 0x1) == 0x25)
		RtlInterlockedCompareExchange64Offst = VP_Offset;
	else
		RtlInterlockedCompareExchange64Offst = VP_Offset_old;

	BYTE RtlAVE[] = {
		0x8B, 0xFF, 0x55, 0x8B, 0xEC
	};

	uintptr_t oldProt;
	auto addr = (PVOID)RtlAddVectoredExceptionHandlerAddr;
	auto size = static_cast<SIZE_T>(5);
	auto A = makesyscall<NTSTATUS>(0x50, 0x00, 0x00, 0x00, "RtlInterlockedCompareExchange64", RtlInterlockedCompareExchange64Offst, 0xC2, 0x14, 0x00);
	auto B = A(GetCurrentProcess(), &addr, &size, PAGE_EXECUTE_READWRITE, &oldProt);

	if (B >= 0)
	{
		int i = 0;
		for (BYTE _byte : RtlAVE) {
			*(BYTE*)(RtlAddVectoredExceptionHandlerAddr + i) = _byte;
			i++;
		}
		NT_SUCCESS(
			makesyscall<NTSTATUS>(0x50, 0x00, 0x00, 0x00, "RtlInterlockedCompareExchange64", RtlInterlockedCompareExchange64Offst, 0xC2, 0x14, 0x00)(
				GetCurrentProcess(), &addr, &size, oldProt, &oldProt));
	}
	return RtlAddVectoredExceptionHandlerAddr;
}
uintptr_t UltimateHooks::RestoreZwQueryInformationProcess() {
	HMODULE ntdll = GetModuleHandleA("ntdll.dll");

	uintptr_t ZwQueryInformationProcessAddr = reinterpret_cast<uintptr_t>(
		GetProcAddress(ntdll, "ZwQueryInformationProcess"));

	BYTE ZwQIP[] = {
		0xB8, 0x19, 0x00, 0x00, 0x00
	};
	//*((uint64_t*)&ZwQIP[1]) = (uint64_t)&hNtQueryProcessInformation;
	int i = 0;
	DWORD old = -1;
	uintptr_t oldest = -1;
	ULONG ret = -1;

	ret = VirtualProtect((LPVOID)ZwQueryInformationProcessAddr, sizeof(ZwQIP), 0x40, &old);
	oldest = old;
	if (ret != 0) {
		memcpy((void*)ZwQueryInformationProcessAddr, &ZwQIP, sizeof(ZwQIP));
		for (BYTE _byte : ZwQIP) {
			VirtualProtect((LPVOID)ZwQueryInformationProcessAddr, sizeof(ZwQIP), 0x40, &old);
			*(BYTE*)(ZwQueryInformationProcessAddr + i) = _byte;
			i++;
		}
		//VirtualProtect((LPVOID)&ZwQueryInformationProcessAddr, sizeof(ZwQIP), oldest, &old);		
	}

	return ZwQueryInformationProcessAddr;
}

uintptr_t UltimateHooks::VirtualAllocateRegion(PVOID& NewFunction, uintptr_t OrigAddress, size_t size) {
	NewFunction = VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT,
		PAGE_EXECUTE_READWRITE);
	return (uintptr_t)NewFunction;
}

void UltimateHooks::CopyRegion(uintptr_t dest, uintptr_t source, size_t size) {
	DWORD oldP;
check:
	VirtualProtect((LPVOID)dest, size, PAGE_EXECUTE_READWRITE, &oldP);
	if (oldP != PAGE_EXECUTE_READWRITE) {
		VirtualProtect((LPVOID)dest, size, oldP, &oldP);
		std::this_thread::sleep_for(100ms);
		goto check;
	}

	(void)memcpy((void*)dest, (PVOID)source, size);
}

void UltimateHooks::FixFuncRellocation(uintptr_t OldFnAddress, uintptr_t OldFnAddressEnd, uintptr_t NewFnAddress, size_t size)
{
	//tried without zydis, but its very big work
	for (int i = 0; i < size; i++) {
		if (*(BYTE*)(NewFnAddress + i) == 0xE8) { // Very Bad realisation of searching CALL instruction
			uintptr_t oldOffset = 0xFFFFFFFF - *(uintptr_t*)(NewFnAddress + i + 1) - 4;
			uintptr_t funcPtr = (OldFnAddress + i) - oldOffset;
			uintptr_t newOffset = funcPtr - (NewFnAddress + i) - 5;
			*(uintptr_t*)(NewFnAddress + i + 1) = newOffset;
			i += 8;
		}
	}
	for (int i = 0; i < size; i++) {
		if (*(BYTE*)(NewFnAddress + i) == 0xE9) { // Very Bad realisation of searching JMP instruction
			uintptr_t oldOffset = 0xFFFFFFFF - *(uintptr_t*)(NewFnAddress + i + 1) - 4;
			uintptr_t funcPtr = (OldFnAddress + i) - oldOffset;
			uintptr_t newOffset = funcPtr - (NewFnAddress + i) - 5;
			i += 8;
		}
	}

	/* Utils::Log("> FixFuncRellocation");
	ZydisDecoder decoder;
	ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_COMPAT_32, ZYDIS_ADDRESS_WIDTH_32);
	ZydisFormatter formatter;
	ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL);

	ZyanU32 runtime_address = NewFnAddress;
	ZyanUSize offset = 0;
	const ZyanUSize length = size;
	ZydisDecodedInstruction instruction;
	int fixedAddressesCount = 0;
	Utils::Log("> FixFuncRellocation: Init: Ok");
	while (ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(&decoder, (PVOID)(NewFnAddress + offset), length - offset, &instruction)))
	{
		Utils::Log("> FixFuncRellocation: Decode buffer");
		char buffer[256];
		ZydisFormatterFormatInstruction(&formatter, &instruction, buffer, sizeof(buffer),
			runtime_address);

		std::string mnemonic(buffer);

		if (mnemonic.find("call 0x") != std::string::npos) {
			uintptr_t hex = std::strtoul((mnemonic.substr(5, 10)).c_str(), NULL, 16);

			uintptr_t originalCall = 0;
			{
				ZydisDecoder decoder1;
				ZydisDecoderInit(&decoder1, ZYDIS_MACHINE_MODE_LONG_COMPAT_32, ZYDIS_ADDRESS_WIDTH_32);

				ZydisFormatter formatter1;
				ZydisFormatterInit(&formatter1, ZYDIS_FORMATTER_STYLE_INTEL);

				const ZyanUSize length = size;
				ZydisDecodedInstruction instruction1;
				{
					if (ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(&decoder1, (PVOID)(OldFnAddress + offset), length - offset, &instruction1))) {
						char buffer1[256];
						ZydisFormatterFormatInstruction(&formatter1, &instruction1, buffer1, sizeof(buffer1), OldFnAddress + offset);

						std::string mnemonic1(buffer1);

						if (mnemonic1.find("call 0x") != std::string::npos) {
							originalCall = std::strtoul((mnemonic1.substr(5, 10)).c_str(), NULL, 16);
						}
					}
				}
			}
			{
				uintptr_t calc1 = (runtime_address - originalCall + 4);
				uintptr_t calc = 0xFFFFFFFF - calc1;
				*(uintptr_t*)(runtime_address + 1) = calc;

				if (ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(&decoder, (PVOID)(NewFnAddress + offset), length - offset, &instruction))) {

					char buffer[256];
					ZydisFormatterFormatInstruction(&formatter, &instruction, buffer, sizeof(buffer), runtime_address);

					std::string mnemonic(buffer);
					{
						uintptr_t hex = std::strtoul((mnemonic.substr(5, 10)).c_str(), NULL, 16);
						if ((hex >= OldFnAddress) && (hex <= OldFnAddressEnd)) {
							uintptr_t calc1 = (runtime_address - hex + 4);
							uintptr_t calc = 0xFFFFFFFF - calc1;
							*(uintptr_t*)(runtime_address + 1) = calc;

							if (ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(&decoder, (PVOID)(NewFnAddress + offset), length - offset, &instruction))) {

								char buffer[256];
								ZydisFormatterFormatInstruction(&formatter, &instruction, buffer, sizeof(buffer), runtime_address);

								std::string mnemonic(buffer);
							}

						}
					}
				}
			}
			fixedAddressesCount++;
		}
		else if (mnemonic.find("int3") != std::string::npos) {
			*(BYTE*)(NewFnAddress + offset) = 0x90;
			fixedAddressesCount++;
		}
		else if ((mnemonic.find("jmp 0x") != std::string::npos) && (*(BYTE*)(runtime_address) == 0xe9)) {
			uintptr_t hex = std::strtoul((mnemonic.substr(5, 10)).c_str(), NULL, 16);
			uintptr_t originalCall = 0;
			{
				ZydisDecoder decoder1;
				ZydisDecoderInit(&decoder1, ZYDIS_MACHINE_MODE_LONG_COMPAT_32, ZYDIS_ADDRESS_WIDTH_32);
				ZydisFormatter formatter1;
				ZydisFormatterInit(&formatter1, ZYDIS_FORMATTER_STYLE_INTEL);
				const ZyanUSize length = size;
				ZydisDecodedInstruction instruction1;
				{
					if (ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(&decoder1, (PVOID)(OldFnAddress + offset), length - offset, &instruction1))) {
						char buffer1[256];
						ZydisFormatterFormatInstruction(&formatter1, &instruction1, buffer1, sizeof(buffer1), OldFnAddress + offset);

						std::string mnemonic1(buffer1);
						if (mnemonic1.find("jmp 0x") != std::string::npos) {
							originalCall = std::strtoul((mnemonic1.substr(4, 10)).c_str(), NULL, 16);
						}
					}
				}
			}
			{
				uintptr_t calcx = originalCall - (OldFnAddress + offset);
				uintptr_t calcy = calcx + (OldFnAddress + offset);
				uintptr_t calc = calcy - runtime_address - 0x5;

				*(uintptr_t*)(runtime_address + 1) = calc;

				if (ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(&decoder, (PVOID)(NewFnAddress + offset), length - offset, &instruction))) {

					char buffer[256];
					ZydisFormatterFormatInstruction(&formatter, &instruction, buffer, sizeof(buffer), runtime_address);

					std::string mnemonic(buffer);
					{
						uintptr_t hex = std::strtoul((mnemonic.substr(4, 10)).c_str(), NULL, 16);
						if ((hex >= OldFnAddress) && (hex <= OldFnAddressEnd)) {
							uintptr_t calc = calcx - 0x5;
							*(uintptr_t*)(runtime_address + 1) = calc;

							if (ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(&decoder, (PVOID)(NewFnAddress + offset), length - offset, &instruction))) {

								char buffer[256];
								ZydisFormatterFormatInstruction(&formatter, &instruction, buffer, sizeof(buffer), runtime_address);

								std::string mnemonic(buffer);
							}

						}
					}
				}
			}
			fixedAddressesCount++;
		}

		offset += instruction.length;
		runtime_address += instruction.length;
	}*/
}

bool UltimateHooks::addHook(uintptr_t address, uintptr_t hkAddress, size_t offset)
{
	if (Hook(address, hkAddress, offset))
	{
		return true;
	}
	return false;
}

bool UltimateHooks::Hook(uintptr_t original_fun, uintptr_t hooked_fun, size_t offset)
{
	auto mbi = MEMORY_BASIC_INFORMATION{ 0 };
	if (!VirtualQuery(reinterpret_cast<void*>(original_fun), &mbi, sizeof(mbi))) {
		return false;
	}

	HookEntries hs;
	HookDetails hd;

	hd.addressToHook = original_fun;
	hd.hookAddress = hooked_fun;
	hs.addressToHookOldProtect = mbi.Protect;

	std::vector<HookEntries> _hookEntries;
	bool isFound = false;

	for (HookEntries hs : hookEntries) {
		if (hs.addressToHookMbiStart == (uintptr_t)mbi.BaseAddress) {
			bool isExisting = false;
			for (HookDetails hd : hs.hookDetails) {
				if (original_fun == hd.addressToHook) {
					isExisting = true;
				}
			}
			if (!isExisting) {
				isFound = true;
				hs.hookDetails.push_back(hd);
			}
		}

		_hookEntries.push_back(hs);
	}
	if (isFound) {
		hookEntries = _hookEntries;
		return true;
	}
	hs.addressToHookMbiStart = ((uintptr_t)mbi.BaseAddress);
	hs.addressToHookMbiEnd = ((uintptr_t)mbi.BaseAddress) + 0x1000;
	hs.addressToHookMbiSize = 0x1000;
	PVOID NewRegionPVOID = nullptr;
	int i = 0;

	uintptr_t NewRegion = VirtualAllocateRegion(NewRegionPVOID, ((uintptr_t)mbi.BaseAddress - 0x1000), 0x3000) + 0x1000;

	while (*(int*)(mbi.BaseAddress) == 0x0) {
		Sleep(10);
	}

	CopyRegion((uintptr_t)NewRegionPVOID, ((uintptr_t)mbi.BaseAddress - 0x1000), 0x3000);

	FixRellocation(((uintptr_t)mbi.BaseAddress - 0x1000), ((uintptr_t)mbi.BaseAddress - 0x1000) + 0x3000, (uintptr_t)NewRegionPVOID, 0x3000, offset);
	hs.allocatedAddressStart = NewRegion;
	hs.allocatedAddressEnd = NewRegion + 0x1000;
	hs.allocatedAddressSize = 0x1000;
	hs.addressToHookoffsetFromStart = original_fun - ((uintptr_t)mbi.BaseAddress);
	hs.hookDetails.push_back(hd);

	for (HookEntries he : hookEntries) {
		if ((he.addressToHookMbiStart == hs.addressToHookMbiStart) &&
			(he.addressToHookMbiEnd == hs.addressToHookMbiEnd))
		{
			return true;
		}
	}

	if (!IsDoneInit)
	{
		VEH_Handle = AddVectoredExceptionHandler(true, static_cast<PTOP_LEVEL_EXCEPTION_FILTER>(LeoHandler));
		IsDoneInit = true;
	}

	if (VEH_Handle)
	{
		auto addr = (PVOID)original_fun;
		auto size = static_cast<SIZE_T>(static_cast<int>(1));
		if (NT_SUCCESS(
			makesyscall<NTSTATUS>(0x50, 0x00, 0x00, 0x00, "RtlInterlockedCompareExchange64", RtlInterlockedCompareExchange64Offst, 0xC2, 0x14, 0x00)(
				GetCurrentProcess(), &addr, &size, PAGE_READONLY, &hs.addressToHookOldProtect)))
		{
			hookEntries.push_back(hs);
			return true;
		}
	}
	return false;
}

MODULEINFO GetModuleInfo2(HMODULE m)
{
	MODULEINFO modinfo = { 0 };
	HMODULE hModule;
	if (m)
		hModule = m;
	else
		hModule = GetModuleHandle(NULL);
	if (hModule == 0)
		return modinfo;
	GetModuleInformation(GetCurrentProcess(), hModule, &modinfo, sizeof(MODULEINFO));
	return modinfo;
}



void UltimateHooks::FixRellocation(uintptr_t OldFnAddress, uintptr_t OldFnAddressEnd, uintptr_t NewFnAddress, size_t size, size_t _offset)
{
	long RellocationOffset = OldFnAddress - NewFnAddress;
#if (!(USE_ZIDYS) || DEBUG_RELLOCATION)


	OldFnAddress += (uintptr_t)_offset;
	for (int i = 0; i < size; i++) {
		uintptr_t runtime_addr = (NewFnAddress + i);
		BYTE* runtime = (BYTE*)runtime_addr;
		bool isCall = *(BYTE*)runtime_addr == 0xE8;
		bool isJmp = *(BYTE*)runtime_addr == 0xE9;

		if (isCall || isJmp) { // Very Bad realisation of searching CALL instruction
			long oldOffset = *(long*)(runtime_addr + 1);
			//Utils::Log("Old offset: " + to_string(oldOffset));
			//if (oldOffset >= 0x1000 || oldOffset <= -1000) {
			uintptr_t funcPtr = (OldFnAddress + i) + oldOffset + 5;
			uintptr_t calcx = funcPtr - (OldFnAddress);

			//if ((long)(OldFnAddress + i) - oldOffset <= 0)
			//	continue;

			BYTE* function = (BYTE*)funcPtr;



			if (runtime[-1] == 0x80) // if sub instruction
				continue;
			if (runtime[-1] == 0x83) // if sub instruction
				continue;
			if (runtime[-1] == 0x8B) // if mov instruction
				continue;
			if (runtime[-2] == 0x8D) // if lea instruction
				continue;
			if (runtime[-2] == 0x81) // if add instruction
				continue;
			if (runtime[-2] == 0x0F) // if subps instruction
				continue;
			if (runtime[-4] == 0xF3) // if movss instruction
				continue;
			if (runtime[-1] == 0xC1) // if shr instruction
				continue;
			if (runtime[-1] == 0xD0) // if shr instruction
				continue;

			long newOffset = funcPtr - runtime_addr - 6;

			if ((funcPtr >= OldFnAddress + 0x1000) && (funcPtr <= OldFnAddressEnd - 0x1000)) {
				//MessageBoxA(0, ("oldOffset: " + to_hex(runtime_addr) + "\n\noldOffset: " + to_string(oldOffset)).c_str(), "My Rellocation", 0);				
				continue;
			}



#if !(DEBUG_RELLOCATION)
			/** (uintptr_t*)(NewFnAddress + i + 1) = newOffset;*/
#endif
			i += 3;
		}
	}
	OldFnAddress -= (uintptr_t)_offset;
#endif

#if USE_ZIDYS || DEBUG_RELLOCATION
	ZydisDecoder decoder;
	ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_COMPAT_32, ZYDIS_ADDRESS_WIDTH_32);
	ZydisFormatter formatter;
	ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL);

	ZyanU32 runtime_address = NewFnAddress + _offset;
	ZyanUSize offset = _offset;
	const ZyanUSize length = size;
	ZydisDecodedInstruction instruction;
	int fixedAddressesCount = 0;

	while (ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(&decoder, (PVOID)(NewFnAddress + offset), length - offset, &instruction)))
	{
		char buffer[256];
		ZydisFormatterFormatInstruction(&formatter, &instruction, buffer, sizeof(buffer),
			runtime_address);

		std::string mnemonic(buffer);
		if (mnemonic.find("call 0x") != std::string::npos) {
			uintptr_t hex = std::strtoul((mnemonic.substr(5, 10)).c_str(), NULL, 16);

			uintptr_t originalCall = 0;
			{
				ZydisDecoder decoder1;
				ZydisDecoderInit(&decoder1, ZYDIS_MACHINE_MODE_LONG_COMPAT_32, ZYDIS_ADDRESS_WIDTH_32);
				ZydisFormatter formatter1;
				ZydisFormatterInit(&formatter1, ZYDIS_FORMATTER_STYLE_INTEL);
				const ZyanUSize length = size;
				ZydisDecodedInstruction instruction1;
				{
					if (ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(&decoder1, (PVOID)(OldFnAddress + offset), length - offset, &instruction1))) {
						char buffer1[256];
						ZydisFormatterFormatInstruction(&formatter1, &instruction1, buffer1, sizeof(buffer1), OldFnAddress + offset);

						std::string mnemonic1(buffer1);

						if (mnemonic1.find("call 0x") != std::string::npos) {
							originalCall = std::strtoul((mnemonic1.substr(5, 10)).c_str(), NULL, 16);
						}
					}
				}
			}
			{

				uintptr_t calc1 = (runtime_address - originalCall + 4);
				uintptr_t calc = 0xFFFFFFFF - calc1;
				long oldOffset = *(uintptr_t*)(runtime_address + 1);
				long newOffset = calc;
				*(uintptr_t*)(runtime_address + 1) = calc;
				Utils::Log("FixRellocation: " + to_hex(runtime_address) + "  |  CALL" + "  |  oldOffset: " + to_string(oldOffset) + "  |  newOffset: " + to_string(newOffset));
				if (ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(&decoder, (PVOID)(NewFnAddress + offset), length - offset, &instruction))) {
					char buffer[256];
					ZydisFormatterFormatInstruction(&formatter, &instruction, buffer, sizeof(buffer), runtime_address);

					std::string mnemonic(buffer);
					{
						uintptr_t hex = std::strtoul((mnemonic.substr(5, 10)).c_str(), NULL, 16);
						if ((hex >= OldFnAddress + 0x1000) && (hex <= OldFnAddressEnd - 0x1000)) {
							uintptr_t calc1 = (runtime_address - hex + 4);
							uintptr_t calc = 0xFFFFFFFF - calc1;
							*(uintptr_t*)(runtime_address + 1) = calc;
							Utils::Log("FixRellocation: " + to_hex(runtime_address) + "  |  CALL1");
							if (ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(&decoder, (PVOID)(NewFnAddress + offset), length - offset, &instruction))) {
								char buffer[256];
								ZydisFormatterFormatInstruction(&formatter, &instruction, buffer, sizeof(buffer), runtime_address);

								std::string mnemonic(buffer);
							}

						}
					}
				}
			}
			fixedAddressesCount++;
		}
		else if (mnemonic.find("int3") != std::string::npos) {
			*(BYTE*)(NewFnAddress + offset) = 0x90;
			fixedAddressesCount++;
		}
		else if ((mnemonic.find("jmp 0x") != std::string::npos) && (*(BYTE*)(runtime_address) == 0xe9)) {
			uintptr_t hex = std::strtoul((mnemonic.substr(5, 10)).c_str(), NULL, 16);

			uintptr_t originalCall = 0;
			{
				ZydisDecoder decoder1;
				ZydisDecoderInit(&decoder1, ZYDIS_MACHINE_MODE_LONG_COMPAT_32, ZYDIS_ADDRESS_WIDTH_32);

				ZydisFormatter formatter1;
				ZydisFormatterInit(&formatter1, ZYDIS_FORMATTER_STYLE_INTEL);

				const ZyanUSize length = size;
				ZydisDecodedInstruction instruction1;
				{
					if (ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(&decoder1, (PVOID)(OldFnAddress + offset), length - offset, &instruction1))) {
						char buffer1[256];
						ZydisFormatterFormatInstruction(&formatter1, &instruction1, buffer1, sizeof(buffer1), OldFnAddress + offset);

						std::string mnemonic1(buffer1);
						if (mnemonic1.find("jmp 0x") != std::string::npos) {
							originalCall = std::strtoul((mnemonic1.substr(4, 10)).c_str(), NULL, 16);
						}
					}
				}
			}
			{

				uintptr_t calcx = originalCall - (OldFnAddress + offset);
				uintptr_t calcy = calcx + (OldFnAddress + offset);
				uintptr_t calc = calcy - runtime_address - 0x5;
				long oldOffset = *(uintptr_t*)(runtime_address + 1);
				long newOffset = calc;
				*(uintptr_t*)(runtime_address + 1) = calc;
				Utils::Log("FixRellocation: " + to_hex(runtime_address) + "  |  JMP" + "  |  oldOffset: " + to_string(oldOffset) + "  |  newOffset: " + to_string(newOffset));
				if (ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(&decoder, (PVOID)(NewFnAddress + offset), length - offset, &instruction))) {

					char buffer[256];
					ZydisFormatterFormatInstruction(&formatter, &instruction, buffer, sizeof(buffer), runtime_address);

					std::string mnemonic(buffer);
					{
						uintptr_t hex = std::strtoul((mnemonic.substr(4, 10)).c_str(), NULL, 16);
						if ((hex >= OldFnAddress + 0x1000) && (hex <= OldFnAddressEnd - 0x1000)) {
							uintptr_t calc = calcx - 0x5;
							*(uintptr_t*)(runtime_address + 1) = calc;
							//Utils::Log("FixRellocation JMP2 - " + to_hex(runtime_address));
							Utils::Log("FixRellocation: " + to_hex(runtime_address) + "  |  JMP1");
							if (ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(&decoder, (PVOID)(NewFnAddress + offset), length - offset, &instruction))) {
								char buffer[256];

								ZydisFormatterFormatInstruction(&formatter, &instruction, buffer, sizeof(buffer), runtime_address);
								std::string mnemonic(buffer);
							}

						}
					}
				}

			}
			fixedAddressesCount++;

		}

		offset += instruction.length;
		runtime_address += instruction.length;
	}
#endif
}