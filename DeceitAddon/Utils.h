#pragma once
#include <string>
#include <algorithm>
#include <Windows.h>
#include <vector>
#include <iostream>
#include <functional>
#include <list>
#include <iostream>
#include <ranges>
#include <sstream>
#include <iomanip>

#define DEFINE_RVA(address) (baseAddr + (DWORD)address)
#define STR_MERGE_IMPL(x, y)                x##y
#define STR_MERGE(x,y)                        STR_MERGE_IMPL(x,y)
#define MAKE_PAD(size)                        BYTE STR_MERGE(pad_, __COUNTER__) [ size ]
#define DEFINE_MEMBER_0(x)                    x;
#define DEFINE_MEMBER_N(x,offset)            struct { MAKE_PAD((DWORD)offset); x; };

template<typename FuncType>
__forceinline static FuncType CallVFunction(void* ppClass, int index)
{
	DWORD64* pVTable = *(DWORD64**)ppClass;
	DWORD64 dwAddress = pVTable[index];
	return (FuncType)(dwAddress);
}

template< typename T >
std::string to_hex(T i)
{
	std::stringstream stream;
	stream
		<< std::setfill('0') << std::setw(sizeof(T) * 2)
		<< std::hex << i;
	return stream.str();
}

inline std::vector<std::string> explode(std::string const& s, char delim)
{
	std::vector<std::string> result;

	if (s.find(delim) == std::string::npos) {
		return result;
	}

	std::istringstream iss(s);

	for (std::string token; std::getline(iss, token, delim); )
	{
		result.push_back(std::move(token));
	}

	return result;
}

inline void* HookVTableFunction(void* pVTable, void* fnHookFunc, int nOffset) // https://guidedhacking.com/threads/how-to-hook-directinput-emulate-key-presses.14011/
{
	intptr_t ptrVtable = *((intptr_t*)pVTable);
	intptr_t ptrFunction = ptrVtable + sizeof(intptr_t) * nOffset;
	intptr_t ptrOriginal = *((intptr_t*)ptrFunction);
	MEMORY_BASIC_INFORMATION mbi;
	VirtualQuery((LPCVOID)ptrFunction, &mbi, sizeof(mbi));
	VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_EXECUTE_READWRITE, &mbi.Protect);
	*((intptr_t*)ptrFunction) = (intptr_t)fnHookFunc;
	VirtualProtect(mbi.BaseAddress, mbi.RegionSize, mbi.Protect, &mbi.Protect);
	return (void*)ptrOriginal;
}