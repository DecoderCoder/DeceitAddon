#include "Structs.h"
#include "Globals.h"

IActor* IEntity::GetActor()
{
	IActorSystem* as = pGameFramework->GetActorSystem();
	if (!as)
		return nullptr;
	return as->GetActor(this->GetId());
}

int64 IEntity::GetTeam() {
	//CGameRulesStandardRounds::ShowRoundStartingMessage() primaryTeam=%d; localTeam=%d
	#define GetTeam 0x26A8
	typedef void* (__fastcall* somemagicFunc)();
	somemagicFunc magic = (somemagicFunc)((uintptr_t)GetModuleHandle(L"Game.dll") + 0x1FDFC); // 8B D8 E8 ? ? ? ? 8B D3 
	auto a1 = magic();

	typedef int(__fastcall* getTeamFunc)(void*, unsigned int);
	getTeamFunc f = (getTeamFunc)((uintptr_t)GetModuleHandle(L"Game.dll") + GetTeam);
	return f(a1, this->GetId());
}