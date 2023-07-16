#pragma once
#include "Utils.h"
#include <Windows.h>
#include "Vector.h"
#include "Render.h"


#ifdef _DEBUG
#define ILINE _inline
#else
#define ILINE __forceinline
#endif

typedef signed char      int8;
typedef signed short     int16;
typedef signed int		 int32;
typedef signed __int64	 int64;
typedef unsigned char	 uint8;
typedef unsigned short	 uint16;
typedef unsigned int	 uint32;
typedef unsigned __int64 uint64;
typedef char             s_char;
typedef const char* c_char;
typedef const wchar_t* c_char_w;

//#define z_ptr            nullptr
//#define ret              return 
//#define r_false          ret false
//#define r_true           ret true
//#define t64              this
//#define t_call           __thiscall
//#define f_call           __fastcall
#define dev_8(c)         c/8

typedef float f32;
typedef double f64;
#define zero_buffer ""
inline f32 f32_zero = 0.000f;
inline f32 m_pi_32 = 3.14159f;
inline f64 m_pi_64 = 3.14159265358979323846264;
ILINE f32 __fsel(const f32 _a, const f32 _b, const f32 _c) { return (_a < 0.0f) ? _c : _b; }
ILINE f64 __fsel(const f64 _a, const f64 _b, const f64 _c) { return (_a < 0.0f) ? _c : _b; }
ILINE f32 __fself(const f32 _a, const f32 _b, const f32 _c) { return (_a < 0.0f) ? _c : _b; }
ILINE f32 __fsels(const f32 _a, const f32 _b, const f32 _c) { return (_a < 0.0f) ? _c : _b; }
ILINE f32 __fres(const f32 _a) { return 1.f / _a; }

#define CanMove_offset 0x186290 // either can't move or is spectat: pseudo first func


#define pRender_offset  0x283B920 // 48 8B 01 FF 90 ? ? ? ? 48 8B 0B 4C 8D 45 17
#define pEntity_offset  0x283B8D0 // 48 8B 0D ? ? ? ? 48 8B 11 4C 8B 82 ? ? ? ? 48 8D 15 ? ? ? ? 
#define pGameFramework_offset  0x283B8C0// 48 8B 0D ? ? ? ? 48 8B 01 FF 90 ? ? ? ? 48 8B 54 24 ? 4C 8D 0D ? ? ? ? 
#define pAISystem 0x283B8F8 // sv_AISystem if ( qword_14283B8F8 ){ if (byte_14283BA49)


typedef __int64 EntityId;

class IEntity;
class IInventory;
class IEntityClass;
class IItem;
class IActor;
class IWeapon;
struct IEntityIt;
struct KillParams;

class IActorIterator
{
public:
	virtual void Function0();
	virtual void Function1();
	virtual IActor* Next();
	virtual void IncreaseIndex();
	virtual int Count();

};//Size=0x0004

class IActorSystem
{
public:
	IActor* GetActor(EntityId id)
	{
		typedef IActor* (__thiscall* OriginalFunc)(void* self, EntityId id);
		return CallVFunction<OriginalFunc>(this, 3)(this, id);
	}
	int GetActorCount()
	{
		typedef int(__thiscall* OriginalFunc)(void* self);
		return CallVFunction<OriginalFunc>(this, 6)(this);
	}
	void create_actor_interator(IActorIterator** ptr) {
		CallVFunction<void(__thiscall*)(void*, IActorIterator**)>(this, 7)(this, ptr);
	}
};


class CGameRulesMPDamageHandling {
public:
	static CGameRulesMPDamageHandling* GetInstance() {
		typedef CGameRulesMPDamageHandling* (__fastcall* getInstanceF)();
		getInstanceF func = (getInstanceF)((uintptr_t)GetModuleHandle(L"Game.dll") + (uintptr_t)0x26869); //E8 ? ? ? ? 48 8B C8 4C 8B F0 
		return func();
	}
};

class CFunctionHandler {
public:
	union {
		DEFINE_MEMBER_0(void** VTable);
	};

	const char* GetFuncName()
	{
		return CallVFunction<const char* (__fastcall*)(CFunctionHandler*)>(this, 4)(this);
	}
};

class IFunctionHandler {
public:
	union {
		DEFINE_MEMBER_0(void** VTable);
	};

	const char* GetFuncName()
	{
		return CallVFunction<const char* (__fastcall*)(IFunctionHandler*)>(this, 4)(this);
	}
};

class IConsoleVar
{
public:
	char* str_desciption; //0x0010 
	union {
		DEFINE_MEMBER_0(void* VFTable);
		DEFINE_MEMBER_N(char* StrName, 0x8);
		DEFINE_MEMBER_N(char* StrDescription, 0x10);

	};

	int64& Flags()
	{
		return *reinterpret_cast<int64*>(uintptr_t(this) + 0x18);
	}
	/*int get_i_val()
	{
		return f_virtual<int(__fastcall*)(PVOID)>(this, virtual_console_var::f_get_i_val)(this);
	}

	float get_f_val()
	{
		return f_virtual<float(__fastcall*)(PVOID)>(this, virtual_console_var::f_get_f_val)(this);
	}

	const char* get_string()
	{
		return f_virtual<const char* (__fastcall*)(PVOID)>(this, virtual_console_var::f_get_string)(this);
	}

	void set(const float f)
	{
		return f_virtual<void(__fastcall*)(PVOID, const float)>(this, virtual_console_var::f_set_f)(this, f);
	}

	void set(const int i)
	{
		return f_virtual<void(__fastcall*)(PVOID, const int)>(this, virtual_console_var::f_set_i)(this, i);
	}

	void set(const char* s)
	{
		return f_virtual<void(__fastcall*)(PVOID, const char*)>(this, virtual_console_var::f_set_c)(this, s);
	}

	template<typename type>
	type get_val()
	{
		return **(type**)(this + 0x48);
	}

	template<typename type>
	void set_val(type value)
	{
		**(type**)(this + 0x48) = value;
	}

	void* value_ptr()
	{
		return *(void**)(this + 0x48);
	}

	e_c_var_type get_type()
	{
		return f_virtual<e_c_var_type(__fastcall*)(void*)>(this, virtual_console_var::f_get_type)(this);
	}*/
};

class IConsole {
public:
	void ExecuteString(const char* command, const bool bSilentMode = false, const bool bDeferExecution = false)
	{
		return CallVFunction<void(__fastcall*)(PVOID, const char*, const bool, const bool)>(this, 25)(this, command, bSilentMode, bDeferExecution);
	}


	IConsoleVar* GetCVar(const char* name) {
		auto var = CallVFunction<IConsoleVar * (__fastcall*)(PVOID, const char*)>(this, 15)(this, name);
		if (var)
			var->Flags() = 0;
		return var;
		//return CallVFunction<CGameRulesMPDamageHandling * (__thiscall*)(void*, const char*, bool)>(this, 8)(this, "MPDamageHandling", false);
	}
};

class IScriptSystem {
public:

};

class SystemEnvironment {

};

class IGameRules {
public:

};

class CGamesRulesModulesManager {
public:
	static CGamesRulesModulesManager* GetInstance() {
		typedef CGamesRulesModulesManager* (__fastcall* getInstanceF)(bool create);
		getInstanceF func = (getInstanceF)((uintptr_t)GetModuleHandle(L"Game.dll") + (uintptr_t)0x546B); //E8 ? ? ? ? 48 8B C8 4C 8B F0 
		return func(true);
	}

	template<typename T>
	T* GetRules(int i) {
		return CallVFunction<CGameRulesMPDamageHandling * (__thiscall*)(void*, int)>(this, 18)(this, i);
	}

	CGameRulesMPDamageHandling* GetDamageHandlingClass() {
		return CallVFunction<CGameRulesMPDamageHandling * (__thiscall*)(void*, const char*, bool)>(this, 7)(this, "MPDamageHandling", false);
	}
};

class IGameFramework {
public:
	IActor* get_client_actor() {
		return CallVFunction<IActor * (__thiscall*)(void*)>(this, 64)(this);
	}
	IEntity* GetClientEntity() {
		return CallVFunction<IEntity * (__thiscall*)(void*)>(this, 69)(this);
	}
	IActorSystem* GetActorSystem() {
		return CallVFunction<IActorSystem * (__thiscall*)(void*)>(this, 26)(this);
	}
	IGameRules* GetGameRules() {
		return CallVFunction<IGameRules * (__thiscall*)(void*)>(this, 33)(this);
	}
};

class ISkeletonPose {
public:
	QuatT& GetAbsJointByID(__int64 id) {
		typedef QuatT& (__thiscall* OriginalFunc)(void* self, __int64);
		return CallVFunction<OriginalFunc>(this, 24)(this, id);
	}
};
class IDefaultSkeleton {
public:
	__int64 GetJointIDByName(const char* name) {
		typedef __int64(__thiscall* get_join_)(void*, const char*);
		return CallVFunction<get_join_>(this, 9)(this, name);
	}
	QuatT& GetDefaultAbsJointByID(__int64 nJointIdx) {
		typedef QuatT& (__thiscall* get_abs_)(void*, __int64);
		return CallVFunction<get_abs_>(this, 11)(this, nJointIdx);
	}
};
class ICharacterInstance {
public:
	ISkeletonPose* GetISkeletonPose() {
		typedef ISkeletonPose* (__thiscall* OriginalFunc)(void* self);
		return CallVFunction<OriginalFunc>(this, 17)(this);
	}
	IDefaultSkeleton* GetIDefaultSkeleton() {
		typedef IDefaultSkeleton* (__thiscall* get_default_)(void*);
		return CallVFunction<get_default_>(this, 21)(this);
	}
};

class IRender {
public:

	bool ProjectToScreen(float ptx, float pty, float ptz, float* sx, float* sy, float* sz)
	{
		static DWORD64 game_base = (DWORD64)(GetModuleHandleA(0));
		typedef bool(__thiscall* oFunc)(void*, float, float, float, float*, float*, float*);
		bool ret = CallVFunction< oFunc >(this, 71)(this, ptx, pty, ptz, sx, sy, sz);
		*sx *= ((float)Render::RenderWidth / 100.0f);
		*sy *= ((float)Render::RenderHeight / 100.0f);
		*sz *= 1.0f;

		return (*sz < 1.0f);
	}

	Vector3 ProjectToScreen(Vector3 in) {
		Vector3 out;
		this->ProjectToScreen(in, &out);
		return out;
	}

	bool ProjectToScreen(Vector3 in, Vector3* out) {
		float x, y, z;
		if (!ProjectToScreen(in.x, in.y, in.z, &x, &y, &z))
			return false;
		out->x = x;
		out->y = y;
		out->z = z;
		return true;
	}

	int window_width() {
		return *(int*)(uintptr_t(this) + 0xAC30);
	}

	int window_height() {
		return *(int*)(uintptr_t(this) + 0xAC34);
	}

	int render_width() {
		return *(int*)(uintptr_t(this) + 0xA9F0);
	}

	int render_height() {
		return *(int*)(uintptr_t(this) + 0xA9F4);
	}

	bool is_full_screen() {
		return *(bool*)(uintptr_t(this) + 0xB5A8);
	}

	char* title_name() {
		return (char*)(uintptr_t(this) + 0xC8E1);
	}

	HANDLE window_handle() {
		return *(HANDLE*)(uintptr_t(this) + 0xC938);
	}
};

class IEntity
{
public:
	union {
		DEFINE_MEMBER_N(char* Name, 0x10);
		DEFINE_MEMBER_N(Vector3 Position, 0xDC);
		DEFINE_MEMBER_N(Quat Rotation, 0xD0);
		DEFINE_MEMBER_N(Matrix34 m_worldTM, 0x90);
	};

	int64 GetTeam();

	bool IsInfected() {
		return this->GetTeam() == 2;
	}

	ICharacterInstance* get_character(DWORD64 id) { // 48 89 5C 24 ? 56 48 81 EC ? ? ? ? 48 8B DA E8 ? ? ? ? 48 8B F0  //CScriptObjectWeapon::GetBonePos
		typedef ICharacterInstance* (__thiscall* OriginalFunc)(void* self, DWORD64);
		return CallVFunction<OriginalFunc>(this, 122)(this, id); // 48 89 5C 24 ? 56 48 81 EC ? ? ? ? 48 8B DA E8 ? ? ? ? 48 8B F0
	}

	IActor* GetActor();

	std::string GetClass() {
		uintptr_t entityInfo = *(uintptr_t*)((uintptr_t)this + 0x18);
		auto entityPtr = *(char**)(entityInfo + 0x10);
		return std::string(entityPtr);
	}
	//Matrix34& get_world_tm() {
	//	typedef Matrix34& (__thiscall* OriginalFunc)(void* self);
	//	return CallVFunction<OriginalFunc>(this, 29)(this);
	//}
	//void GetWorldBounds(AABB* aabb) {
	//	CallVFunction<void(__thiscall*)(void*, AABB*)>(this, 31)(this, aabb);
	//}
	//void get_local_bounds(AABB* aabb) {
	//	CallVFunction<void(__thiscall*)(void*, AABB*)>(this, 32)(this, aabb);
	//}
	void* get_entity_render() {
		return CallVFunction<void* (__thiscall*)(void*)>(this, 40)(this);
	}

	const char* get_name() {
		return CallVFunction<const char* (__thiscall*)(void*)>(this, 14)(this);
	}

	void set_name(const char* name) {
		CallVFunction<void(__thiscall*)(void*, const char*)>(this, 13)(this, name);
	}

	void set_invisible(bool b_invisible) {
		CallVFunction<void(__thiscall*)(void*, bool)>(this, 56)(this, b_invisible);
	}

	bool is_invisible() {
		return CallVFunction<bool(__thiscall*)(void*)>(this, 57)(this);
	}

	EntityId GetId() {
		//0xFFFFFFFFFFFFFFFF
		auto func = CallVFunction<EntityId(__thiscall*)(void*)>(this, 1);
		if ((unsigned long long)func == 0xFFFFFFFFFFFFFFFF || func == 0 || (unsigned long long)func > 0x0000FFFFFFFFFFFF)
			return 0;
		return func(this);
	}
	IEntityClass* get_class() {
		return CallVFunction<IEntityClass * (__thiscall*)(void*)>(this, 3)(this);
	}

	void SetPosition(Vector3 vPos) {
		int check = 0;
		CallVFunction<IEntityClass* (__thiscall*)(void*, Vector3, int* unkwn)>(this, 37)(this, vPos, &check);
	}

	Ang3 get_angle() {
		return Ang3(quat_to_matrix33(this->Rotation));
	}

	Vector3 getBonePosition(__int64 id) {
		ICharacterInstance* pCharacterInstance = this->get_character(0);
		if (pCharacterInstance != 0) {
			ISkeletonPose* pSkeletonPose = pCharacterInstance->GetISkeletonPose();
			if (pSkeletonPose != 0) {
				QuatT bitch = pSkeletonPose->GetAbsJointByID(id);
				Matrix34 World = this->m_worldTM;
				Matrix34 SkeletonAbs = Matrix34(bitch);
				return (World * SkeletonAbs).GetTranslation();
			}
		}
		return Vector3(0, 0, 0);
	}

	Vector3 getBonePosition(const char* name) {
		ICharacterInstance* Character = this->get_character(0);
		if (!Character)
			return Vector3(0, 0, 0);
		IDefaultSkeleton* DefaultSkeleton = Character->GetIDefaultSkeleton();
		if (!DefaultSkeleton)
			return Vector3(0, 0, 0);
		__int64 id = DefaultSkeleton->GetJointIDByName(name);

		return getBonePosition(id);
	}

	/*bool IsAlive(IGameFramework* pGameFramework) {
		IActorSystem* sys = pGameFramework->GetActorSystem();
		IActor* actor = sys->GetActor(this->get_id());

		if (!actor || actor->Health <= 0)
			return false;
		return true;
	}*/


	void set_position(Vector3 pos);
	//char pad_0x0000[0x10]; //0x0000
	//char* Name; //0x00010
	//char pad_0x0010[0x30]; //0x0010
	//Vector3 m_position; //0x0040 
	////Quat m_rotation; //0x004C 
	//Vector3 m_scale; //0x005C 
	; //0x0068 

}; //Size=0x004C

class IEntityIt
{
public:
	// <interfuscator:shuffle>
	virtual ~IEntityIt() {}

	virtual void AddRef() = 0;

	//! Deletes this iterator and frees any memory it might have allocated.
	virtual void Release() = 0;

	//! Checks whether current iterator position is the end position.
	//! \return true if iterator at end position.
	virtual bool IsEnd() = 0;

	//! Retrieves next entity.
	//! \return Pointer to the entity that the iterator points to before it goes to the next.
	virtual IEntity* Next() = 0;

	//! Retrieves current entity.
	//! \return Entity that the iterator points to.
	virtual IEntity* This() = 0;

	//! Positions the iterator at the beginning of the entity list.
	virtual void MoveFirst() = 0;
	// </interfuscator:shuffle>
};

class IEntitySystem {
public:
	//IEntityClassRegistry* GetClassRegistry() {
	//	return CallVFunction< IEntityClassRegistry * (__thiscall*)(void*)>(this, 8)(this);
	//}

	IEntityIt* GetEntityNum() {
		return CallVFunction<IEntityIt * (__thiscall*)(void*)>(this, 16)(this);
	}

	IEntityIt* GetEntityIterator() {
		uintptr_t outAddr = NULL;
		return CallVFunction<IEntityIt * (__thiscall*)(void*, void*)>(this, 17)(this, &outAddr);
	}

	std::vector<IEntity*> GetPlayers() {
		std::vector<IEntity*> rez;
		IEntityIt* it = *(IEntityIt**)this->GetEntityIterator();
		int i = 0;
		if (it)
		{
			for (; IEntity * entity = it->Next();) {
				if (!entity)
					continue;

				std::string entityClass = entity->GetClass();

				if (entityClass == "Player")
					rez.push_back(entity);
			}
		}

		return rez;
	}

	std::vector<IEntity*> GetAll(std::string className = "") {
		std::vector<IEntity*> rez;
		IEntityIt* it = *(IEntityIt**)this->GetEntityIterator();
		int i = 0;
		if (it)
		{
			for (; IEntity * entity = it->Next();) {
				if (!entity)
					continue;

				if (className != "") {
					auto entityClass = entity->GetClass();
					if (entityClass != className)
						continue;
				}
				rez.push_back(entity);
			}
		}

		return rez;
	}
};

class CPlayerInput {

};

class IActor {
public:
	union {
		DEFINE_MEMBER_N(float Health, 0x88);
		DEFINE_MEMBER_N(IEntity* entity, 0x0010);
		DEFINE_MEMBER_N(int64 team_id, 0x4D8);
		DEFINE_MEMBER_N(CPlayerInput* playerInput, 0x1228);
	};

	char _0x0004[4];
	IEntity* LinkedEntity; //0x0008 
	DWORD GameObject; //0x000C 
	DWORD EntityID; //0x0010 
	IInventory* get_inventory() {
		return CallVFunction< IInventory * (__thiscall*)(void*)>(this, 68)(this);
	}
	IItem* GetCurrentItem(bool includeVehicle = false) {
		if (!this)
			return nullptr;
		return CallVFunction <IItem * (__thiscall*)(void*, bool)>(this, 71)(this, includeVehicle);
	}
	bool is_player() {
		return CallVFunction<bool(__thiscall*)(void*)>(this, 81)(this);
	}
	void toggle_thirdperson() {
		return CallVFunction<void(__thiscall*)(void*)>(this, 75)(this);
	}
	bool is_thirdperson() {
		return CallVFunction<bool(__thiscall*)(void*)>(this, 74)(this);
	}

	bool IsFallen()
	{
		return CallVFunction<int(__thiscall*)(void*)>(this, 46)(this);
	}

	bool IsDead()
	{
		return CallVFunction<bool(__thiscall*)(void*)>(this, 48)(this);
	}

	void simle_net_kill() {
		return CallVFunction<void(__thiscall*)(void*)>(this, 180)(this);
	}
	void NetKill(KillParams* param) {
		CallVFunction<void(__thiscall*)(void*, KillParams*)>(this, 124)(this, param);
	}
	float get_health() {
		return CallVFunction<float(__thiscall*)(void*)>(this, 35)(this);
	}

	std::string GetCharacterName() {
		if (!((char*)((uintptr_t)this + 0xBF8)))
			return std::string("");
		std::string str = std::string(*(char**)((uintptr_t)this + 0xBF8));

		if (str.size() > 0) {
			auto exploded = explode(str, '/');

			if (exploded.size() > 2) {
				return exploded[2];
			}
			else {
				return std::string("");
			}
		}
		else
			return std::string("");
	}
};

class IPhysicalWorld {
public:
	union {

	};
};

enum type_zero { _zero };
enum type_min { v_min };
enum type_max { v_max };
enum type_identity { _identity };

typedef void* win_hwnd;
typedef void* win_hinstance;
typedef void* win_hdc;
typedef void* win_hglrc;

struct s_spread_mod_params
{
	f32 min_mod;
	f32 max_mod;
	f32 attack_mod;
	f32 decay_mod;
	f32 end_decay_mod;
	f32 speed_m_mod;
	f32 speed_holdBreathActive_m_mod;
	f32 rotation_m_mod;
	f32 spread_crouch_m_mod;
	f32 spread_jump_m_mod;
	f32 spread_slide_m_mod;
	f32 spread_holdBreathActive_m_mod;
};

struct s_recoil_mod_params
{
	f32 max_recoil_mod;
	f32 attack_mod;
	f32 first_attack_mod;
	f32 decay_mod;
	f32 end_decay_mod;
	Vector2 max_mod;
	f32 impulse_mod;
	f32 recoil_crouch_m_mod;
	f32 recoil_jump_m_mod;
	f32 recoil_holdBreathActive_m_mod;
};

struct s_weapon_ammo
{
	IEntity* p_ammo_class;
	int32 count;
};

enum virtual_fire_mode : int32
{
	f_set_spread_param = 496 / 8,
	f_set_recoil_param = 512 / 8,
	f_start_fire_fm = 216 / 8,
	f_stop_fire_fm = 224 / 8,
	f_is_fire = 232 / 8,
	f_net_shoot = 296 / 8,
};

enum virtual_item : int32
{
	f_get_weapon = 720 / 8, // %s: Weapon not available (EntId %i)
};

class IItem
{
public:
	std::string GetName() {
		char* nameP = (char*)(*(uintptr_t*)(*(uintptr_t*)(this + 0x10) + 0x10));
		return std::string(nameP);
	}

	IWeapon* GetWeapon()
	{
		return CallVFunction<IWeapon * (__fastcall*)(void*)>(this, virtual_item::f_get_weapon)(this);
	}

	IEntity* get_weapon_entity()
	{
		return *(IEntity**)(this + 0x10);
	}
};

struct IFireMode
{
	f32 __fastcall get_fire_rate()
	{
		return (f32) * (int16*)(*(uint64*)(*(uint64*)(this + 0x10) + 8) + 0x90);
	}

	void set_fire_rate(int16 r)
	{
		auto addr1 = *(uint64*)((uintptr_t)this + 0x10);
		if (!addr1)
			return;
		auto addr2 = *(uint64*)(addr1 + 8);
		auto addr3 = (int32*)((uintptr_t)((int16*)(addr2)) + 0x92);
		*addr3 = r;
		addr3 = (int32*)((uintptr_t)((int16*)(addr2)) + 0x96);
		*addr3 = r;
	}

	void set_spread_param(s_spread_mod_params* s_smp, f32 mod_multiplier)
	{
		CallVFunction<void(__fastcall*)(void*, s_spread_mod_params*, f32)>(this, virtual_fire_mode::f_set_spread_param)(this, s_smp, mod_multiplier);
	}

	void set_recoil_param(s_recoil_mod_params* s_rmp, f32 mod_multiplier)
	{
		CallVFunction<void(__fastcall*)(void*, s_recoil_mod_params*, f32)>(this, virtual_fire_mode::f_set_recoil_param)(this, s_rmp, mod_multiplier);
	}

	void start_fire()
	{
		CallVFunction<void(__fastcall*)(void*)>(this, virtual_fire_mode::f_start_fire_fm)(this);
	}

	void stop_fire()
	{
		CallVFunction<void(__fastcall*)(void*)>(this, virtual_fire_mode::f_stop_fire_fm)(this);
	}

	bool is_fire()
	{
		return CallVFunction<bool(__fastcall*)(void*)>(this, virtual_fire_mode::f_is_fire)(this);
	}

	void net_shoot(Vector3* bullshit, int bullshit2)
	{
		CallVFunction<void(__fastcall*)(void*, Vector3*, int)>(this, virtual_fire_mode::f_net_shoot)(this, bullshit, bullshit2);
	}

	char pad_0x0000[0x90]; //0x0000
	f32 N000000FA; //0x0090 
	char pad_0x0094[0x14]; //0x0094
	f32 N000000FD; //0x00A8 
	char pad_0x00AC[0x74]; //0x00AC
	f32 fl_min_spread; //0x0120 
	f32 fl_max_spread; //0x0124 
};

class IMeleeParam {
public:
	union {
		DEFINE_MEMBER_N(float Range, 0x08);
		DEFINE_MEMBER_N(float Delay, 0x28);
		DEFINE_MEMBER_N(float Duration, 0x30);
	};
};

class IMelee {
public:
	union {
		DEFINE_MEMBER_N(IMeleeParam* MeleeParam, 0x18);
	};
};

enum virtual_weapon : int32
{
	f_get_ammo_count = 200 / 8,
	f_set_ammo_count = 208 / 8,
	f_start_fire = 96 / 8, //%s: Weapon not available (EntId %i)
	f_stop_fire = 104 / 8,
};

class IWeapon
{
public:
	union {
		DEFINE_MEMBER_N(IMelee* Melee, 0x40);
	};

	int32 get_ammo_count(IEntity* type)
	{
		return CallVFunction<int32(__fastcall*)(void*, IEntity*)>(this, virtual_weapon::f_get_ammo_count)(this, type);
	}

	void set_ammo_count(IEntity* type, int32 count)
	{
		return CallVFunction<void(__fastcall*)(void*, IEntity*, int32)>(this, virtual_weapon::f_start_fire)(this, type, count);
	}

	void start_fire_to_vector()
	{
		return CallVFunction<void(__fastcall*)(void*)>(this, virtual_weapon::f_start_fire)(this);
	}

	void start_fire()
	{
		return CallVFunction<void(__fastcall*)(void*)>(this, virtual_weapon::f_start_fire)(this);
	}

	void stop_fire()
	{
		return CallVFunction<void(__fastcall*)(void*)>(this, virtual_weapon::f_stop_fire)(this);
	}

	char pad_0x0000[0x38]; //0x0000
	IFireMode* m_fm; //0x0038 
	char pad_0x0040[0xA0]; //0x0040
	s_weapon_ammo* m_ammo; //0x00E0 
	s_weapon_ammo* m_bonusammo; //0x00E8 
	unsigned char m_fire_alternation; //0x00F0 
	unsigned char m_restart_zoom; //0x00F1 
};