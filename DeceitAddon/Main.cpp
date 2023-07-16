#include "Main.h"
#include "Structs.h"
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <fstream>
#include <mutex>
#include "Globals.h"
#include "Hooks/ultimate_hooks.h"
#include <list>

using namespace std;

#define baseAddr ((uintptr_t)GetModuleHandle(NULL))
#define gameAddr ((uintptr_t)GetModuleHandle(L"Game.dll"))
#define GetAsyncKeyStateN(key) GetAsyncKeyState(key) & 0x8000

//283B8D0

int AimFov = 150;
float AimSpeed = 2;
bool aimlock = true;
bool TpToBlood = false;
bool doLog = false;

uintptr_t o_rwi = 0;

typedef signed int		 int32;

std::map<IActor*, string> realName;

enum e_srwi_flags
{
	e_melee = 0x307,
	e_ammo_flags = 0x31F,
	e_ammo_wall_shot = 0x10,
	e_ammo_wall_shot_f = 0x75,
	e_ammo_wall_shot_e = 0x7E,
	e_ammo_wall_shot_o = 0x14,
	e_actors_flags = 0x100 | 1
};

struct s_rwi_params
{
	char pad_0x0000[0x18]; //0x0000
	Vector3 dir; //0x0018 
	Vector3 org; //0x0024 
	int32 obj_types; //0x0030 
}; //Size=0x0840

typedef int32(__fastcall* m_rwi)(IPhysicalWorld*, s_rwi_params&, char*, int32);
m_rwi p_rwi = nullptr;

//typedef int32(__fastcall* ftGetSpectatorMode)(IEntity*);
//m_rwi p_rwi = nullptr;

typedef int64(__fastcall* ftCryLog)(void*, uintptr_t, const char*, va_list);
ftCryLog pCryLog = nullptr;

typedef __int64(__fastcall* m_ClProcessHit)(__int64, char*, unsigned int, unsigned int, int, int, unsigned __int8);
m_ClProcessHit p_ClProcessHit = nullptr;

IEntity* aimbotTarget;
IEntity* localPlayer = nullptr;

std::list<string> console;

std::mutex fileWrite;

void writeToFile(string FileName, string text) {
	fileWrite.lock();
	ofstream myfile;
	myfile.open(FileName, std::ios_base::app);
	myfile << text << "\n";
	myfile.close();
	fileWrite.unlock();
}

bool ret0 = false;

int32 __fastcall h_rwi(IPhysicalWorld* a1, s_rwi_params& srwi, char* p_name_tag, int32 i)
{
	//if (srwi.obj_types == e_srwi_flags::e_melee)
	//writeToFile("h_rwi.txt", to_hex(srwi.obj_types));
	if (srwi.obj_types == e_srwi_flags::e_ammo_flags || srwi.obj_types == 0x11f)
	{
		if (aimbotTarget)
		{
			//if (Settings::Aimbot::WallShot)
			srwi.obj_types = e_srwi_flags::e_ammo_wall_shot;

			//Vector3 AimPos = pRender->ProjectToScreen(aimbotTarget->getBonePosition("Bip01 Head"));
			srwi.dir = aimbotTarget->getBonePosition("Bip01 Head") - srwi.org;
		}
	}
	auto resp = p_rwi(a1, srwi, p_name_tag, i);

	return resp;
}

int64 __fastcall hCryLog(void* a1, uintptr_t a2, const char* a3, va_list a4) {
	if (doLog && strlen(a3) > 0) {
		string str = string(a3);
		char buff[0x5000];
		vsprintf(buff, a3, a4);
		str = string(buff);

		string skipText[] = { "---Shader Cache:", "CryAnimation:: Trying to load", "Loading ", "<Flash>" };
		bool skip = false;
		for (auto text : skipText)
		{
			if (str.find(text) != string::npos) {
				skip = true;
				break;
			}
		}

		if (!skip) {

			console.insert(console.begin(), str);
			writeToFile("log.txt", str);
		}
	}
	return pCryLog(a1, a2, a3, a4);
}


void* origGetEntitiesAround = nullptr;

class twoVectors {
public:
	twoVectors(Vector3 v1, Vector3 v2) {
		this->v1 = v1;
		this->v2 = v2;
	}

	Vector3 v1;
	Vector3 v2;
};

std::vector<twoVectors> entitiesAround;

__int64 HookFunc(__int64 a1, Vector3* ptmin, Vector3* ptmax, uintptr_t* pList, int objtypes, __int64 pPetitioner, int szListPrealoc, unsigned int iCaller) {

	typedef __int64(__fastcall* func)(__int64, Vector3*, Vector3*, uintptr_t*, int, __int64, int, unsigned int);

	auto resp = ((func)origGetEntitiesAround)(a1, ptmin, ptmax, pList, objtypes, pPetitioner, szListPrealoc, iCaller);

	return resp;
}

__int64 __fastcall h_ClProcessHit(
	__int64 a1,
	char* a2,
	unsigned int shooterId,
	unsigned int weaponId,
	int a5,
	int a6,
	unsigned __int8 a7) {

	string shooterName = "";
	string itemName = "";
	for (auto player : pEntitySystem->GetPlayers()) {
		if (player->GetId() == shooterId) {
			auto name = player->Name;
			if (name)
			{
				auto actor = player->GetActor();
				if (actor) {
					shooterName = string(name) + " | " + actor->GetCharacterName();
					auto item = actor->GetCurrentItem();
					if (item)
						itemName = item->GetName();
				}
			}

			break;
		}
	}

	console.insert(console.begin(), "[" + (shooterName != "" ? shooterName : to_string(shooterId)) + "] shooted you with " + to_string(weaponId) + (itemName != "" ? (" - " + itemName) : ""));

	return p_ClProcessHit(a1, a2, shooterId, weaponId, a5, a6, a7);
}

typedef void(__fastcall* ExecuteString)(void*, const char*, const bool, const bool);
ExecuteString oExecuteString = nullptr;

void hExecuteString(void* _this, const char* command, const bool bSilentMode, const bool bDeferExecution) {
	console.insert(console.begin(), "ExecuteString: " + string(command));
	writeToFile("ExecuteString.txt", "ExecuteString: " + string(command));
	return oExecuteString(_this, command, bSilentMode, bDeferExecution);
}
//
//typedef int(__fastcall* EndFunction)(CFunctionHandler*);
//typedef bool(__fastcall* EndFunction1)(CFunctionHandler*, const uintptr_t&);
//typedef void* (__fastcall* EndFunction2)(CFunctionHandler*);
//typedef void* (__fastcall* EndFunction3)(CFunctionHandler*);
//
//
//EndFunction oEndFunction = nullptr;
//EndFunction1 oEndFunction1 = nullptr;
//EndFunction2 oEndFunction2 = nullptr;
//EndFunction3 oEndFunction3 = nullptr;
//
//int hEndFunction(CFunctionHandler* _this) {
//	auto str = _this->GetFuncName();
//	if (str) {
//		console.push_back("EndFunction: " + string(str));
//		writeToFile("EndFunction.txt", "EndFunction: " + string(str));
//	}
//	return oEndFunction(_this);
//}
//
//
//bool hEndFunction1(CFunctionHandler* _this, const uintptr_t& a2) {
//	console.push_back("EndFunction1: " + string(_this->GetFuncName()));
//	writeToFile("EndFunction.txt", "EndFunction1: " + string(_this->GetFuncName()));
//	return oEndFunction2(_this);
//}
//
//void* hEndFunction2(CFunctionHandler* _this) {
//	console.push_back("EndFunction2: " + string(_this->GetFuncName()));
//	writeToFile("EndFunction.txt", "EndFunction2: " + string(_this->GetFuncName()));
//	return oEndFunction2(_this);
//}
//
//void* hEndFunction3(CFunctionHandler* _this) {
//	console.push_back("EndFunction3: " + string(_this->GetFuncName()));
//	writeToFile("EndFunction.txt", "EndFunction3: " + string(_this->GetFuncName()));
//	return oEndFunction3(_this);
//}

//typedef int(__fastcall* CScriptSystem__BeginCall1)(void*, void*);
//typedef int(__fastcall* CScriptSystem__BeginCall2)(void*, const char*);
//typedef int(__fastcall* CScriptSystem__BeginCall3)(void*, void*, const char*);
//typedef int(__fastcall* CScriptSystem__BeginCall4)(void*, const char*, const char*);
//
//CScriptSystem__BeginCall1 oCScriptSystem__BeginCall1 = nullptr;
//CScriptSystem__BeginCall2 oCScriptSystem__BeginCall2 = nullptr;
//CScriptSystem__BeginCall3 oCScriptSystem__BeginCall3 = nullptr;
//CScriptSystem__BeginCall4 oCScriptSystem__BeginCall4 = nullptr;
//
//int hCScriptSystem__BeginCall1(void* _this, void* hFunc) {
//	console.insert(console.begin(), "BeginCall1: " + to_hex(hFunc));
//	writeToFile("BeginCall.txt", "BeginCall1: " + to_hex(hFunc));
//	return oCScriptSystem__BeginCall1(_this, hFunc);
//}
//
//int hCScriptSystem__BeginCall2(void* _this, const char* sFuncName) {
//	console.insert(console.begin(), "BeginCall2: " + string(sFuncName));
//	writeToFile("BeginCall.txt", "BeginCall2: " + string(sFuncName));
//	return oCScriptSystem__BeginCall2(_this, sFuncName);
//}
//
//int hCScriptSystem__BeginCall3(void* _this, void* pTable, const char* sFuncName) {
//	console.insert(console.begin(), "BeginCall3: " + string(sFuncName));
//	writeToFile("BeginCall.txt", "BeginCall3: " + string(sFuncName));
//	return oCScriptSystem__BeginCall3(_this, pTable, sFuncName);
//}
//
//int hCScriptSystem__BeginCall4(void* _this, const char* sTableName, const char* sFuncName) {
//	console.insert(console.begin(), "BeginCall4: " + string(sFuncName));
//	writeToFile("BeginCall.txt", "BeginCall4: " + string(sFuncName));
//	return oCScriptSystem__BeginCall4(_this, sTableName, sFuncName);
//}

void Main::Init() {
	System = *(SystemEnvironment**)(gameAddr + (uintptr_t)0xCEC558);
	pScriptSystem = *(IScriptSystem**)(baseAddr + 0x283B868);
	Console = *(IConsole**)(baseAddr + 0x283B8D8);
	pGameFramework = *(IGameFramework**)(baseAddr + pGameFramework_offset);
	pEntitySystem = *(IEntitySystem**)(baseAddr + pEntity_offset);
	pRender = *(IRender**)(baseAddr + pRender_offset);
	pPhysicalWorld = *(IPhysicalWorld**)(baseAddr + 0x283B870);
	pGameRules = pGameFramework->GetGameRules();

	pGameRulesMPDamageHandling = CGameRulesMPDamageHandling::GetInstance();

	p_rwi = (m_rwi)HookVTableFunction(pPhysicalWorld, &h_rwi, 288 / 8);
	p_ClProcessHit = (m_ClProcessHit)HookVTableFunction(pGameRulesMPDamageHandling, &h_ClProcessHit, 11);
	pCryLog = (ftCryLog)HookVTableFunction(*(uintptr_t /* LogSystem */**)(System + 0xC0), &hCryLog, 8 / 8);

	oExecuteString = (ExecuteString)HookVTableFunction(Console, &hExecuteString, 25);

	//oCScriptSystem__BeginCall1 = (CScriptSystem__BeginCall1)HookVTableFunction(pScriptSystem, &hCScriptSystem__BeginCall1, 8 / 8);
	//oCScriptSystem__BeginCall2 = (CScriptSystem__BeginCall2)HookVTableFunction(pScriptSystem, &hCScriptSystem__BeginCall2, 120 / 8);
	//oCScriptSystem__BeginCall3 = (CScriptSystem__BeginCall3)HookVTableFunction(pScriptSystem, &hCScriptSystem__BeginCall3, 104 / 8);
	//oCScriptSystem__BeginCall4 = (CScriptSystem__BeginCall4)HookVTableFunction(pScriptSystem, &hCScriptSystem__BeginCall4, 112 / 8);

	//oEndFunction = (EndFunction)HookVTableFunction(&functionHandler, &hEndFunction, 11);
	//oEndFunction1 = (EndFunction1)HookVTableFunction(&functionHandler, &hEndFunction1, 10);
	//oEndFunction2 = (EndFunction2)HookVTableFunction(&functionHandler, &hEndFunction2, 9);
	//oEndFunction3 = (EndFunction3)HookVTableFunction(&functionHandler, &hEndFunction3, 8);

	//int* CanMoveF = (int*)((uintptr_t)GetModuleHandle(L"Game.dll") + (uintptr_t)CanMove_offset);// = 0xC300B0; ret 1

	//VirtualProtect(CanMoveF, sizeof(int), 0x40, &old);
	//*CanMoveF = 0xC301B0;
	//VirtualProtect(CanMoveF, sizeof(int), old, &old);
}

void Main::Deinit() {
	HookVTableFunction(pPhysicalWorld, (void*)p_rwi, 288 / 8);
	HookVTableFunction(pGameRulesMPDamageHandling, (void*)p_ClProcessHit, 11);
	HookVTableFunction(Console, (void*)oExecuteString, 25);
	HookVTableFunction(*(SystemEnvironment**)(*(uintptr_t*)((uintptr_t)GetModuleHandle(L"Game.dll") + (uintptr_t)0xCEC558) + 0xC0), (void*)pCryLog, 8 / 8);
	//uintptr_t GetEntitiesAroundFunc = baseAddr + GetEntitiesAround;

	DWORD old;
	/*VirtualProtect((void*)GetEntitiesAroundFunc, 16, 0x40, &old);
	memcpy((char*)((uintptr_t)GetEntitiesAroundFunc), origGetEntitiesAround, 15);
	VirtualProtect((void*)GetEntitiesAroundFunc, 16, old, &old);*/

}

void CopyToClipboard(char* str) {
	const size_t len = strlen(str) + 1;
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
	memcpy(GlobalLock(hMem), str, len);
	GlobalUnlock(hMem);
	OpenClipboard(0);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hMem);
	CloseClipboard();
}

std::vector<IEntity*> detectedBags;
std::vector<IEntity*> detectedPlayers;


bool ShowAllNames = false;


bool DrankBlood(IEntity* player) {
	return std::find(detectedPlayers.begin(), detectedPlayers.end(), player) != detectedPlayers.end();
}

int lastSetPos = 0;

bool ESP = true;
bool Aim = true;
bool ShowInfect = true;

IEntity* closestPlayerToAim(float distance) {
	Vector3 center = Vector3(Render::RenderWidth / 2, Render::RenderHeight / 2, 0);
	float closest = FLT_MAX;
	IEntity* closestPlayer = nullptr;

	for (auto player : pEntitySystem->GetAll()) {
		IActorSystem* sys = pGameFramework->GetActorSystem();
		IActor* actor = sys->GetActor(player->GetId());

		if (!actor || actor->IsDead())
			continue;

		Vector3 in = player->getBonePosition("Bip01 Head");
		Vector3 out;
		if (!pRender->ProjectToScreen(in, &out))
			continue;
		out.z = 0;

		float dist = center.distanceTo(out);

		if (dist > distance)
			continue;

		if (dist < closest) {
			closest = dist;
			closestPlayer = player;
		}
	}
	return closestPlayer;
}

char commandBuffer[1000];

void Main::MainWindow() {
	//int i = 0;
	//Render::BeginOverlayTab("BloodBags");
	//for (auto& obj : pEntitySystem->GetAll("BloodBagConsumable")) {		
	//	if (ImGui::Button((to_string(i) + " | ").c_str())) {
	//		CopyToClipboard((char*)to_hex((uintptr_t)obj).c_str());
	//	}
	//	i++;
	//}
	//Render::EndOverlayTab();

	//i = 0;
	//Render::BeginOverlay();
	//for (auto& obj : pEntitySystem->GetAll("BloodBagConsumable")) {
	//	Render::Draw_Text(pRender->ProjectToScreen(obj->Position), to_string(i));
	//	i++;
	//}
	//Render::EndOverlay();

	Render::BeginOverlayTab("CryLog");

	for (auto str : console)
	{
		ImGui::TextUnformatted(str.c_str());
	}

	Render::EndOverlayTab();

	if (localPlayer)
	{
		auto weapon = localPlayer->GetActor()->GetCurrentItem();
		if (weapon) {
			auto aaa = weapon->GetWeapon();
			if (aaa) {
				auto name = weapon->GetName();
				if (string(name).find("DeceitKnife") != string::npos) {
					auto melee = weapon->GetWeapon()->Melee->MeleeParam;
					melee->Delay = 0;
					melee->Duration = 0;
					melee->Range = 9999;


					if (GetAsyncKeyState('F')) {

						auto entity = closestPlayerToAim(AimFov);
						aimbotTarget = entity;
						if (entity) {
							aaa->start_fire();
						}
						else
							aimbotTarget = nullptr;
					}
				}
			}
		}
	}

	ImGui::Begin("Main Menu");
	ImGui::Checkbox("ESP", &ESP);
	ImGui::Checkbox("Aim", &Aim);
	ImGui::Checkbox("Show infect window", &ShowInfect);
	ImGui::Checkbox("TP to Blood", &TpToBlood);

	ImGui::InputInt("Aim FOV", &AimFov);
	ImGui::InputFloat("Aim Speed", &AimSpeed);
	ImGui::Checkbox("Show names: ", &ShowAllNames);
	if (ImGui::CollapsingHeader("DEBUG")) {
		if (ImGui::Button("Spectate")) {
			Console->ExecuteString("de_spectate");
		}
		if (ImGui::Button("Lua Debugger")) {
			Console->ExecuteString("lua_debugger_show");
		}

		ImGui::Checkbox("Aimlock : ", &aimlock);

		ImGui::InputText("Command", (char*)&commandBuffer, sizeof(commandBuffer));
		if (ImGui::Button("Execute")) {
			//auto var = console->GetCVar("de_tracker");

			auto splitted_command = explode(commandBuffer, ' ');
			auto cvar_name = splitted_command.empty() ? commandBuffer : splitted_command.at(0);

			// HACK, this is setting the flags to VF_NULL on get_cvar
			auto cvar_ptr = Console->GetCVar(cvar_name.c_str());

			char* vf_cheat = (char*)((uintptr_t)GetModuleHandle(NULL) + 0x247E03);

			DWORD old;

			VirtualProtect(vf_cheat, 1, 0x40, &old);
			vf_cheat[0] = 0xEB;
			VirtualProtect(vf_cheat, 1, old, &old);



			Console->ExecuteString(string(commandBuffer).c_str());
		}
		ImGui::Checkbox("Log", &doLog);
	}


	localPlayer = pGameFramework->GetClientEntity();

	if (pEntitySystem->GetEntityNum() == 0)
	{
		detectedBags.clear();
		detectedPlayers.clear();
	}

	if (GetAsyncKeyStateN(VK_DELETE)) {
		typedef void(__fastcall* NetReviveAt)(IActor*, Vector3*, Vector4*, int, uint8);
		NetReviveAt func = (NetReviveAt)(gameAddr + 0x2B12D0);
		Vector4 vec = Vector4(0, 0, 0, 0);
		func(localPlayer->GetActor(), &localPlayer->Position, &vec, localPlayer->GetTeam(), 15);

	}

	if (GetAsyncKeyState(VK_HOME))
	{	//2574b96d5d0		
		//uintptr_t a1 = ((uintptr_t)GetModuleHandle(L"Game.dll") + 0xCECD98);
		//uintptr_t* a2 = (uintptr_t*)((uintptr_t)GetModuleHandle(L"Game.dll") + 0xCECD98);
		//func(a1, *a2, "de_tracker");


		Vector3 prevPos = localPlayer->Position;
		prevPos.z += 1;
		if (lastSetPos < GetTickCount()) {
			localPlayer->SetPosition(prevPos);
			lastSetPos = GetTickCount() + 30;
		}
	}

	if (GetAsyncKeyState(VK_INSERT))
	{	//2574b96d5d0		
		Vector3 prevPos = localPlayer->Position;
		prevPos.z -= 1;

		if (lastSetPos < GetTickCount()) {
			localPlayer->SetPosition(prevPos);
			lastSetPos = GetTickCount() + 30;
		}
	}

	//if (GetAsyncKeyState(VK_HOME))	
	{
		IEntityIt* it = *(IEntityIt**)pEntitySystem->GetEntityIterator();
		int i = 0;
		if (it)
		{
			for (; IEntity * entity = it->Next();) {
				if (!entity)
					continue;

				if (strcmp(entity->Name, "DrankBloodBag") == 0) {
					if (std::find(detectedBags.begin(), detectedBags.end(), entity) == detectedBags.end()) {
						auto Players = pEntitySystem->GetPlayers();

						float closest = FLT_MAX;
						IEntity* closestPlayer = nullptr;
						for (auto player : Players) {
							auto dist = player->Position.distanceTo(entity->Position);
							if (dist < closest) {
								closest = dist;
								closestPlayer = player;
							}
						}

						if (closestPlayer)
							if (std::find(detectedPlayers.begin(), detectedPlayers.end(), closestPlayer) == detectedPlayers.end())
								detectedPlayers.push_back(closestPlayer);
						detectedBags.push_back(entity);
					}
				}

				auto actor = entity->GetActor();
				if (!actor)
					continue;
				string charName = actor->GetCharacterName();
				if (charName.size() == 0)
					continue;
				////// Yeti
				////// Expiriment
				////// Spider
				////// Werewolf
				////// Vampire

				if (charName == string("Yeti") || charName == string("Experiment") || charName == string("Spider") || charName == string("Werewolf") || charName == string("Vampire"))
					if (std::find(detectedPlayers.begin(), detectedPlayers.end(), entity) == detectedPlayers.end())
						detectedPlayers.push_back(entity);
			}
		}
		ImGui::Text(("Entities count: " + to_string((int)pEntitySystem->GetEntityNum())).c_str());
	}

	if (ImGui::CollapsingHeader("Players")) {
		for (auto player : pEntitySystem->GetPlayers()) {
			IActor* actor = pGameFramework->GetActorSystem()->GetActor(player->GetId());

			if (!actor)
				continue;

			if (ImGui::Button(("" + string(player->Name) + ": " + to_string(player->GetId()) + " | " + to_string(actor->Health)).c_str())) {
				localPlayer = player;
				CopyToClipboard((char*)(to_hex(actor)).c_str());
			}
		}
	}



	ImGui::End();

	if (localPlayer && ShowInfect) {
		Render::BeginOverlayTab("infected");

		for (auto player : pEntitySystem->GetPlayers())
		{
			IActorSystem* sys = pGameFramework->GetActorSystem();
			IActor* actor = sys->GetActor(player->GetId());

			if (!actor || actor->IsDead())
				continue;

			string characterName;

			//if (!realName.count(actor)) {
			//	realName[actor] = actor->GetCharacterName();
			//}
			characterName = actor->GetCharacterName();

			if (player->IsInfected()) {
				auto text = (characterName + " | " + string(player->Name));
				if (DrankBlood(player))
					ImGui::TextColored(ImColor(255, 0, 0), text.c_str());
				else
					ImGui::Text(text.c_str());
			}

		}

		Render::EndOverlayTab();
	}


	//if (localPlayer) {
	//	auto angle = Ang3(quat_to_matrix33(*(Quat*)((uintptr_t)(localPlayer)+0x90)));
	//	auto x = angle.z * 180 / M_PI;
	//	auto y = 0;
	//	auto z = 0;
	//	Render::BeginOverlayTab("LP");
	//	ImGui::Columns(3);

	//	ImGui::Text(to_string((int)x).c_str());
	//	ImGui::NextColumn();
	//	ImGui::Text(to_string((int)y).c_str());
	//	ImGui::NextColumn();
	//	ImGui::Text(to_string((int)z).c_str());
	//	ImGui::NextColumn();

	//	Render::EndOverlayTab();

	//	Render::BeginOverlayTab("Angles");


	//	Render::EndOverlayTab();
	//}



	OverlayWindow();
}

IEntity* closestToAim(float distance, std::vector<IEntity*>& entities) {
	Vector3 center = Vector3(Render::RenderWidth / 2, Render::RenderHeight / 2, 0);
	float closest = FLT_MAX;
	IEntity* closestPlayer = nullptr;

	for (auto player : entities) {
		Vector3 in = player->Position;
		Vector3 out;
		if (!pRender->ProjectToScreen(in, &out))
			continue;

		float dist = center.distanceTo(out);
		if (dist > distance)
			continue;

		if (dist < closest) {
			closest = dist;
			closestPlayer = player;
		}
	}
	return closestPlayer;
}

bool smooth = false;

void AimAtPos(float x, float y)
{
	if (!(AimSpeed > 0))
		return;
	float CenterX = Render::RenderWidth / 2;
	float CenterY = Render::RenderHeight / 2;

	//int width = GetSystemMetrics(SM_CXSCREEN);
	//int height = GetSystemMetrics(SM_CYSCREEN);
	//int ScreenCenterY = height * 0.5f;
	//int ScreenCenterX = width * 0.5f;

	float TargetX = 0;
	float TargetY = 0;

	//X Axis
	if (x != 0)
	{
		if (x > CenterX)
		{
			TargetX = -(CenterX - x);
			TargetX /= AimSpeed;
			if (TargetX + CenterX > CenterX * 2) TargetX = 0;
		}

		if (x < CenterX)
		{
			TargetX = x - CenterX;
			TargetX /= AimSpeed;
			if (TargetX + CenterX < 0) TargetX = 0;
		}
	}

	//Y Axis

	if (y != 0)
	{
		if (y > CenterY)
		{
			TargetY = -(CenterY - y);
			TargetY /= AimSpeed;
			if (TargetY + CenterY > CenterY * 2) TargetY = 0;
		}

		if (y < CenterY)
		{
			TargetY = y - CenterY;
			TargetY /= AimSpeed;
			if (TargetY + CenterY < 0) TargetY = 0;
		}
	}

	if (!smooth)
	{
		mouse_event(MOUSEEVENTF_MOVE, (DWORD)(TargetX), (DWORD)(TargetY), NULL, NULL);
		return;
	}

	TargetX /= 10;
	TargetY /= 10;

	//Mouse even't will round to 0 a lot of the time, so we can add this, to make it more accurrate on slow speeds.
	if (fabs(TargetX) < 1)
	{
		if (TargetX > 0)
		{
			TargetX = 1;
		}
		if (TargetX < 0)
		{
			TargetX = -1;
		}
	}
	if (fabs(TargetY) < 1)
	{
		if (TargetY > 0)
		{
			TargetY = 1;
		}
		if (TargetY < 0)
		{
			TargetY = -1;
		}
	}
	mouse_event(MOUSEEVENTF_MOVE, TargetX, TargetY, NULL, NULL);
}

void DrawBone(IEntity* player, string boneStart, string boneEnd, ImColor color) {
	Vector3 in1 = player->getBonePosition(boneStart.c_str());
	Vector3 in2 = player->getBonePosition(boneEnd.c_str());
	Vector3 out1;
	Vector3 out2;

	if (!pRender->ProjectToScreen(in1, &out1) || !pRender->ProjectToScreen(in2, &out2))
		return;

	Render::Draw_Line(out1, out2, color, 1);
}

void DrawSkeleton(IEntity* player, ImColor color) {
	DrawBone(player, ("Bip01 Head"), ("Bip01 Neck"), color);
	DrawBone(player, ("Bip01 Head"), ("Bip01 Neck"), color);
	DrawBone(player, ("Bip01 Neck"), ("Bip01 R UpperArm"), color);
	DrawBone(player, ("Bip01 R UpperArm"), ("Bip01 R ForeArm"), color);
	DrawBone(player, ("Bip01 R ForeArm"), ("Bip01 R Hand"), color);
	DrawBone(player, ("Bip01 Neck"), ("Bip01 L UpperArm"), color);
	DrawBone(player, ("Bip01 L UpperArm"), ("Bip01 L ForeArm"), color);
	DrawBone(player, ("Bip01 L ForeArm"), ("Bip01 L Hand"), color);
	DrawBone(player, ("Bip01 Neck"), ("Bip01 Pelvis"), color);
	DrawBone(player, ("Bip01 Pelvis"), ("Bip01 R Thigh"), color);
	DrawBone(player, ("Bip01 R Thigh"), ("Bip01 R Calf"), color);
	DrawBone(player, ("Bip01 R Calf"), ("Bip01 R Foot"), color);
	DrawBone(player, ("Bip01 Pelvis"), ("Bip01 L Thigh"), color);
	DrawBone(player, ("Bip01 L Thigh"), ("Bip01 L Calf"), color);
	DrawBone(player, ("Bip01 L Calf"), ("Bip01 L Foot"), color);
}

void Main::OverlayWindow() {
	if (!localPlayer)
		return;

	Render::BeginOverlay();

	for (auto line : entitiesAround)
	{
		Render::Draw_Line(pRender->ProjectToScreen(line.v1), pRender->ProjectToScreen(line.v2), ImColor(255, 0, 0), 1);
	}

	if (ESP) {



		for (auto player : pEntitySystem->GetPlayers())
		{
			if (player == localPlayer)
				continue;

			IActorSystem* sys = pGameFramework->GetActorSystem();
			IActor* actor = sys->GetActor(player->GetId());

			if (!actor || actor->IsDead())
				continue;

			auto characterName = actor->GetCharacterName();

			Vector3 in = player->getBonePosition("Bip01 Head");
			Vector3 out;

			if (!pRender->ProjectToScreen(in, &out))
				continue;

			if (player->IsInfected()) {
				DrawSkeleton(player, ImColor(255, 0, 0));
				Render::Draw_Circle(out.x, out.y, 5, ImColor(255, 0, 0), 10);
			}
			else {
				DrawSkeleton(player, ImColor(255, 255, 255));
				Render::Draw_Circle(out.x, out.y, 5, ImColor(255, 255, 255), 10);
			}

			Render::Draw_Text(out.x - ImGui::CalcTextSize(characterName.c_str()).x / 2, out.y - 40, characterName);
		}

		if (ShowAllNames)
			for (auto player : pEntitySystem->GetAll()) {
				Vector3 in = player->Position;
				if (localPlayer && in.distanceTo(localPlayer->Position) > 30)
					continue;

				Vector3 out = pRender->ProjectToScreen(in);
				Render::Draw_Text(out.x, out.y, string(player->Name));
				Render::Draw_Text(out.x, out.y + 20, player->GetClass());
			}

		for (auto bag : pEntitySystem->GetAll("BloodBagConsumable")) {
			//BloodBadCosumable
			Vector3 in = bag->Position;
			Vector3 out;
			if (!pRender->ProjectToScreen(in, &out))
				continue;

			Render::Draw_Circle(out.x, out.y, 2, ImColor(255, 0, 0), 3);
		}

		for (auto entity : pEntitySystem->GetAll()) {
			if (string(entity->Name) == "RewardCrate-19" || string(entity->Name) == "RewardCrate-20")
			{
				Vector3 in = entity->Position;
				Vector3 out;
				if (!pRender->ProjectToScreen(in, &out))
					continue;

				Render::Draw_Circle(out.x, out.y, 10, ImColor(255, 0, 0), 5);
				break;
			}

			//BloodBadCosumable		
		}
		if (TpToBlood)
			if (GetAsyncKeyState('T') || GetAsyncKeyState('t')) {
				auto entities = pEntitySystem->GetAll("BloodBagConsumable");
				auto target = closestToAim(AimFov, entities);
				if (target) {
					localPlayer->SetPosition(target->Position);
				}
			}

	}


	/*for (auto player : pEntitySystem->GetFuses()) {
		Vector3 in = player->Position;
		in.z += 1;
		Vector3 out = pRender->ProjectToScreen(in);
		Render::Draw_Circle(out.x, out.y, 5, ImColor(0, 255, 0), 10);
	}*/


	if (Aim) {
		if (GetAsyncKeyState(VK_TAB)) {
			auto entities = pEntitySystem->GetPlayers();
			auto target = closestToAim(AimFov, entities);
			if (target) {
				localPlayer->SetPosition(target->Position);
			}
		}

		if ((GetAsyncKeyState(VK_RBUTTON)) && AimFov > 0) { // Aimbot
			auto target = closestPlayerToAim(AimFov);
			aimbotTarget = target;
			if (target) {
				Vector3 AimPos = target ? pRender->ProjectToScreen(target->getBonePosition("Bip01 Head")) : Vector3(0, 0, 0);

				if (aimlock)
					AimAtPos(AimPos.x, AimPos.y);
			}
		}
		else {
			aimbotTarget = nullptr;
		}
		if (ESP)
			Render::Draw_Circle(Render::RenderWidth / 2, Render::RenderHeight / 2, AimFov, ImColor(0, 0, 0), 1);
	}

	Render::EndOverlay();
}