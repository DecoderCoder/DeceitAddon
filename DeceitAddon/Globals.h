#pragma once
#include "Structs.h"

inline IConsole* Console = nullptr;
inline IScriptSystem* pScriptSystem = nullptr;
inline SystemEnvironment* System = nullptr;
inline IGameFramework* pGameFramework = nullptr;
inline IEntitySystem* pEntitySystem = nullptr;
inline IPhysicalWorld* pPhysicalWorld = nullptr;
inline IRender* pRender = nullptr;
inline IGameRules* pGameRules = nullptr;

inline CGamesRulesModulesManager* pGameRulesModuleManager = nullptr;
inline CGameRulesMPDamageHandling* pGameRulesMPDamageHandling = nullptr;

inline DWORD old;