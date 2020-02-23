#include "Hooks.h"
#include "RPCs.h"
#include "Functions.h"
#include "Scripting.h"
#include "Utils.h"
#include "CVector.h"
#include "CVector2D.h"
#include "CTypes.h"
#include "main.h"
#include "Player.h"
#include "Hack.h"
#include <amx/amx.h>
#include <plugincommon.h>
#include <raknet/BitStream.h>
#include <raknet/NetworkTypes.h>
#include "Hack.h"

#include <stdio.h>
#include <string.h>
#include <set>

#include <map>
#if !defined PAD
    #define PAD(a, b) char a[b]
#endif

#include "Structs.h"

typedef cell AMX_NATIVE_CALL (*AMX_Function_t)(AMX *amx, cell *params);

AMX* AntiHackAMX;
extern std::set<AMX*> amxList;
bool debug = false;
namespace HackScripting
{
    static cell AMX_NATIVE_CALL SetPlayerActiveCheats(AMX* amx, cell* params)
    {
        CHECK_PARAMS(2, "SetPlayerActiveCheats");
        int playerid = (int)params[1];
        int cheats = (int)params[2];
        if (playerid < 0 || playerid > 999)return 0;
        Hack::PlayerActiveCheats[playerid] = cheats;
        return 1;
    }
    static cell AMX_NATIVE_CALL GetPlayerActiveCheats(AMX* amx, cell* params)
    {
        CHECK_PARAMS(1, "GetPlayerActiveCheats");
        int playerid = (int)params[1];
        if (playerid < 0 || playerid > 999)return 0;
        return (cell)Hack::PlayerActiveCheats[playerid];
    }
    static cell AMX_NATIVE_CALL GetPlayerCheat(AMX* amx, cell* params)
    {
        CHECK_PARAMS(1, "GetPlayerCheat");
        int playerid = (int)params[1];
        if (playerid < 0 || playerid > 999)return 0;
        return Hack::hacks[playerid];
    }
    static cell AMX_NATIVE_CALL WaitPlayerSync(AMX* amx, cell* params) 
    {
        CHECK_PARAMS(1, "WaitPlayerSync");
        int playerid = (int)params[1];
        if (playerid < 0 || playerid > 999)return 0;
        Hack::DisableSyncPly[playerid] = 50;
        return 1;
    }
    static cell AMX_NATIVE_CALL WaitVehicleSync(AMX* amx, cell* params)
    {
        CHECK_PARAMS(1, "WaitVehicleSync");
        int vehicleid = (int)params[1];
        if (vehicleid < 0 || vehicleid > 1999)return 0;
        Hack::DisableSyncVeh[vehicleid] = 50;
        return 1;
    }
    int const WeaponSlots[47] =
    {
        0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 10, 10,
        10, 10, 10, 10, 8, 8, 8, -1, -1, -1,
        2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 4, 6,
        6, 7, 7, 7, 7, 8, 12, 9, 9, 9, 11,
        11, 11
    };

    AMX_NATIVE original_GivePlayerWeapon;
    cell AMX_NATIVE_CALL hook_GivePlayerWeapon(AMX *amx, cell *params) {
        CHECK_PARAMS(3, "hook_GivePlayerWeapon");
        int playerid = (int)params[1];
        BYTE weapon = (BYTE)params[2];
        if (weapon < 0 || weapon > 46)return 0;
        if (playerid < 0 || playerid > 999)return 0;
        Hack::DisableSyncPly[playerid] = 20;
        Hack::SyncWeapons[playerid][WeaponSlots[weapon]] = weapon;
        return original_GivePlayerWeapon(amx, params);
    }
    AMX_NATIVE original_ResetPlayerWeapons;
    cell AMX_NATIVE_CALL hook_ResetPlayerWeapons(AMX *amx, cell *params) {
        CHECK_PARAMS(1, "hook_ResetPlayerWeapons");
        int playerid = (int)params[1];
        if (playerid < 0 || playerid > 999)return 0;
        Hack::DisableSyncPly[playerid] = 20;
        for (int i = 0; i < 13; i++)
        Hack::SyncWeapons[playerid][i] = 0;
        return original_ResetPlayerWeapons(amx, params);
    }
    AMX_NATIVE original_SetVehicleHealth;
    cell AMX_NATIVE_CALL hook_SetVehicleHealth(AMX *amx, cell *params) {
        CHECK_PARAMS(2, "hook_SetVehicleHealth");
        int vehicleid = (int)params[1];
        if (vehicleid < 1 || vehicleid > 1999)return 0;
        Hack::DisableSyncVeh[vehicleid] = 20;
        return original_SetVehicleHealth(amx, params);
    }
    AMX_NATIVE original_SetVehiclePos;
    cell AMX_NATIVE_CALL hook_SetVehiclePos(AMX *amx, cell *params) {
        CHECK_PARAMS(4, "hook_SetVehiclePos");
        int vehicleid = (int)params[1];
        if (vehicleid < 1 || vehicleid > 1999)return 0;
        Hack::DisableSyncVeh[vehicleid] = 20;
        return original_SetVehiclePos(amx, params);
    }
    AMX_NATIVE original_SetVehicleVelocity;
    cell AMX_NATIVE_CALL hook_SetVehicleVelocity(AMX *amx, cell *params) {
        CHECK_PARAMS(4, "hook_SetVehicleVelocity");
        int vehicleid = (int)params[1];
        if (vehicleid < 1 || vehicleid > 1999)return 0;
        Hack::DisableSyncVeh[vehicleid] = 20;
        return original_SetVehicleVelocity(amx, params);
    }
    AMX_NATIVE original_RepairVehicle;
    cell AMX_NATIVE_CALL hook_RepairVehicle(AMX *amx, cell *params) {
        CHECK_PARAMS(1, "hook_RepairVehicle");
        int vehicleid = (int)params[1];
        if (vehicleid < 0 || vehicleid > 1999)return 0;
        Hack::DisableSyncVeh[vehicleid] = 20;
        return original_RepairVehicle(amx, params);
    }
    AMX_NATIVE original_PutPlayerInVehicle;
    cell AMX_NATIVE_CALL hook_PutPlayerInVehicle(AMX *amx, cell *params) {
        CHECK_PARAMS(3, "hook_PutPlayerInVehicle");
        int playerid = (int)params[1];
        int vehicleid = (int)params[2];
        if (playerid < 0 || playerid > 999)return 0;
        if (vehicleid < 0 || vehicleid > 1999)return 0;
        Hack::DisableSyncVeh[vehicleid] = 20;
        Hack::DisableSyncPly[playerid] = 30;
        Hack::atwarning[playerid] = 0;
        return original_PutPlayerInVehicle(amx, params);
    }
    AMX_NATIVE original_SetPlayerPos;
    cell AMX_NATIVE_CALL hook_SetPlayerPos(AMX *amx, cell *params) {
        CHECK_PARAMS(4, "hook_SetPlayerPos");
        int playerid = (int)params[1];
        if (playerid < 0 || playerid > 999)return 0;
        Hack::DisableSyncPly[playerid] = 20;
        return original_SetPlayerPos(amx, params);
    }
    AMX_NATIVE original_SetPlayerPosFindZ;
    cell AMX_NATIVE_CALL hook_SetPlayerPosFindZ(AMX *amx, cell *params) {
        CHECK_PARAMS(4, "hook_SetPlayerPosFindZ");
        int playerid = (int)params[1];
        if (playerid < 0 || playerid > 999)return 0;
        Hack::DisableSyncPly[playerid] = 20;
        return original_SetPlayerPosFindZ(amx, params);
    }
    AMX_NATIVE original_SetPlayerVelocity;
    cell AMX_NATIVE_CALL hook_SetPlayerVelocity(AMX *amx, cell *params) {
        CHECK_PARAMS(4, "hook_SetPlayerVelocity");
        int playerid = (int)params[1];
        if (playerid < 0 || playerid > 999)return 0;
        Hack::DisableSyncPly[playerid] = 20;
        return original_SetPlayerVelocity(amx, params);
    }
    AMX_NATIVE original_SetVehicleAngularVelocity;
    cell AMX_NATIVE_CALL hook_SetVehicleAngularVelocity(AMX *amx, cell *params) {
        CHECK_PARAMS(4, "hook_SetVehicleAngularVelocity");
        int vehicleid = (int)params[1];
        if (vehicleid < 0 || vehicleid > 1999)return 0;
        Hack::DisableSyncVeh[vehicleid] = 28;
        return original_SetVehicleAngularVelocity(amx, params);
    }
    cell AMX_NATIVE_CALL SKY_GetPlayerArmour(AMX *amx, cell *params)
    {
        CHECK_PARAMS(2, "SKY_GetPlayerArmour");
        int amxIndex = 0;
        cell retval;
        cell* addr;
        if (!amx_FindPublic(AntiHackAMX, "SKY_GetPlayerArmour", &amxIndex))
        {
            amx_Push(AntiHackAMX, params[1]);
            amx_Exec(AntiHackAMX, &retval, amxIndex);
            amx_GetAddr(amx, params[2], &addr);
            *addr = retval;
        }
        return 1;
    }
    /*CVector CalculateDifferantial(CVector Vec[])
    {
        CVector dif;
        dif = (Vec[0] * 3.0f - Vec[1] * 4.0f + Vec[2]);
        return dif;
    }
    cell AMX_NATIVE_CALL SKY_GetPlayerDifAim(AMX *amx, cell *params)
    {
        CHECK_PARAMS(4, "SKY_GetPlayerDifAim");
        int playerid = params[1];
        if (playerid < 0 || playerid > 999)return 0;
        CVector dif = CalculateDifferantial(LastFourAim[playerid]);
        cell* addr;
        amx_GetAddr(amx, params[2], &addr);
        *addr = amx_ftoc(dif.fX);
        amx_GetAddr(amx, params[3], &addr);
        *addr = amx_ftoc(dif.fY);
        amx_GetAddr(amx, params[4], &addr);
        *addr = amx_ftoc(dif.fZ);
        return 1;
    }*/
    cell AMX_NATIVE_CALL SKY_GetPlayerHealth(AMX *amx, cell *params)
    {
        CHECK_PARAMS(2, "SKY_GetPlayerHealth");
        int amxIndex = 0;
        cell retval;
        cell* addr;
        if (!amx_FindPublic(AntiHackAMX, "SKY_GetPlayerHealth", &amxIndex))
        {
            amx_Push(AntiHackAMX, params[1]);
            amx_Exec(AntiHackAMX, &retval, amxIndex);
            amx_GetAddr(amx, params[2], &addr);
            *addr = retval;
        }
        return 1;
    }
    cell AMX_NATIVE_CALL CallHooks(AMX *amx, cell *params, char* name, int params_count)
    {
        CHECK_PARAMS(params_count, name);
        int amxIndex = 0;
        cell retval = 0;
        int i;
        if (!amx_FindPublic(AntiHackAMX, name, &amxIndex))
        {
            for (i = params_count; i > 0; i--)
            {
                amx_Push(AntiHackAMX, params[i]);
            }
            amx_Exec(AntiHackAMX, &retval, amxIndex);
        }
        return retval;
    }
    cell AMX_NATIVE_CALL SKY_IsPlayerSpawned(AMX *amx, cell *params) { return CallHooks(amx, params, "SKY_IsPlayerSpawned", 1); }
    cell AMX_NATIVE_CALL SKY_SpawnPlayer(AMX *amx, cell *params){return CallHooks(amx, params, "SKY_SpawnPlayer", 1);}
    cell AMX_NATIVE_CALL SKY_SetPlayerHealth(AMX *amx, cell *params){return CallHooks(amx, params, "SKY_SetPlayerHealth", 2);}
    cell AMX_NATIVE_CALL SKY_SetPlayerArmour(AMX *amx, cell *params){return CallHooks(amx, params, "SKY_SetPlayerArmour", 2);}
    cell AMX_NATIVE_CALL SKY_GetPlayerTeam(AMX *amx, cell *params){return CallHooks(amx, params, "SKY_GetPlayerTeam", 1);}
    cell AMX_NATIVE_CALL SKY_SetPlayerTeam(AMX *amx, cell *params){return CallHooks(amx, params, "SKY_SetPlayerTeam", 2);}
    cell AMX_NATIVE_CALL SKY_SendDeathMessage(AMX *amx, cell *params){return CallHooks(amx, params, "SKY_SendDeathMessage", 3);}
    cell AMX_NATIVE_CALL SKY_AddPlayerClass(AMX *amx, cell *params){return CallHooks(amx, params, "SKY_AddPlayerClass", 11);}
    cell AMX_NATIVE_CALL SKY_AddPlayerClassEx(AMX *amx, cell *params){return CallHooks(amx, params, "SKY_AddPlayerClassEx", 12);}
    cell AMX_NATIVE_CALL SKY_SetSpawnInfo(AMX *amx, cell *params){return CallHooks(amx, params, "SKY_SetSpawnInfo", 13);}
    cell AMX_NATIVE_CALL SKY_TogglePlayerSpectating(AMX *amx, cell *params){return CallHooks(amx, params, "SKY_TogglePlayerSpectating", 2);}
    cell AMX_NATIVE_CALL SKY_TogglePlayerControllable(AMX *amx, cell *params){return CallHooks(amx, params, "SKY_TogglePlayerControllable", 2);}
    cell AMX_NATIVE_CALL SKY_SetPlayerVirtualWorld(AMX *amx, cell *params){return CallHooks(amx, params, "SKY_SetPlayerVirtualWorld", 2);}
    cell AMX_NATIVE_CALL SKY_GetPlayerVirtualWorld(AMX *amx, cell *params){return CallHooks(amx, params, "SKY_GetPlayerVirtualWorld", 1);}
    cell AMX_NATIVE_CALL SKY_PlayerSpectatePlayer(AMX *amx, cell *params){return CallHooks(amx, params, "SKY_PlayerSpectatePlayer", 3);}
    cell AMX_NATIVE_CALL SKY_GetPlayerState(AMX *amx, cell *params){return CallHooks(amx, params, "SKY_GetPlayerState", 1);}
    cell AMX_NATIVE_CALL SKY_DestroyVehicle(AMX *amx, cell *params) { return CallHooks(amx, params, "SKY_DestroyVehicle", 1); }
    cell AMX_NATIVE_CALL SKY_SetPlayerPosFindZ(AMX *amx, cell *params) { return CallHooks(amx, params, "SKY_SetPlayerPosFindZ", 4); }
    cell AMX_NATIVE_CALL SKY_AddStaticVehicleEx(AMX *amx, cell *params) { return CallHooks(amx, params, "SKY_AddStaticVehicleEx", 9); }
    cell AMX_NATIVE_CALL SKY_AddStaticVehicle(AMX *amx, cell *params) { return CallHooks(amx, params, "SKY_AddStaticVehicle", 7); }
    cell AMX_NATIVE_CALL SKY_CreateVehicle(AMX *amx, cell *params) { return CallHooks(amx, params, "SKY_CreateVehicle", 9); }
    cell AMX_NATIVE_CALL SKY_SetPlayerPos(AMX *amx, cell *params) { return CallHooks(amx, params, "SKY_SetPlayerPos", 4); }
    cell AMX_NATIVE_CALL SKY_SetPlayerVelocity(AMX *amx, cell *params) { return CallHooks(amx, params, "SKY_SetPlayerVelocity", 4); }

    cell upParams[5];
    bool upState = false;
    cell AMX_NATIVE_CALL SKY_OnPlayerDamage(AMX *amx, cell *params)
    {
        CHECK_PARAMS(5, "SKY_OnPlayerDamage");
        int amxIndex = 0;
        cell retval;
        cell* addr[5];
        for (int i = 0; i < 5; i++)
        {
            amx_GetAddr(amx, params[i + 1], &addr[i]);
        }
        for (std::set<AMX*>::iterator a = amxList.begin(); a != amxList.end(); ++a)
        {
            if (*a == AntiHackAMX)continue;
            amxIndex = 0;
            if (!amx_FindPublic(*a, "OnPlayerDamage", &amxIndex))
            {
                for (int i = 4; i >= 0; i--)amx_Push(*a, *addr[i]);
                amx_Exec(*a, &retval, amxIndex);
                if (upState)
                {
                    upState = false;
                    for (int i = 0; i < 5; i++)
                    {
                        *addr[i] = upParams[i];
                    }
                    break;
                }
            }
        }
        return retval;
    }
    cell AMX_NATIVE_CALL SKY_UpdateOnPlayerDamage(AMX *amx, cell *params)
    {
        CHECK_PARAMS(5, "SKY_UpdateOnPlayerDamage");
        for (int i = 1; i < 6; i++)upParams[i-1] = params[i];
        upState = true;
        return 1;
    }
    cell AMX_NATIVE_CALL SKY_InitializeAntiHack(AMX *amx, cell *params)
    {
        CHECK_PARAMS(1, "SKY_InitializeAntiHack");
        AntiHackAMX = amx;
        debug = !!params[1];
        return 1;
    }
    /*
    cell AMX_NATIVE_CALL SKY_OnPlayerDeath(AMX *amx, cell *params)
    {
        int amxIndex = 0;
        for (std::set<AMX*>::iterator a = amxList.begin(); a != amxList.end(); ++a)
        {
            if (*a == AntiHackAMX)continue;
            amxIndex = 0;
            if (!amx_FindPublic(*a, "SKY_OnPlayerDeath", &amxIndex))
            {
                amx_Push(AntiHackAMX, params[3]);
                amx_Push(AntiHackAMX, params[2]);
                amx_Push(AntiHackAMX, params[1]);
                amx_Exec(*a, NULL, amxIndex);
            }
        }
        return 1;
    }*/
    cell AMX_NATIVE_CALL SKY_IsPlayerBot(AMX *amx, cell *params)
    {
        int playerid = params[1];
        if (playerid < 0 || playerid > 999)return 0;
        if (!pNetGame->pPlayerPool->bIsPlayerConnected[playerid])return 0;
        int c,i, len = strlen(pNetGame->pPlayerPool->szSerial[playerid]);
        c = 0;
        for (i = 0; i < len; i++) {
            if (pNetGame->pPlayerPool->szSerial[playerid][i] >= '0' && pNetGame->pPlayerPool->szSerial[playerid][i] <= '9')c++;
        }
        return (c >= 30 || len <=30);
    }
    AMX_NATIVE_INFO Natives[] = {
        { "SetPlayerActiveCheats",		SetPlayerActiveCheats },
        { "GetPlayerCheat",				GetPlayerCheat },
        { "WaitPlayerSync",				WaitPlayerSync },
        { "WaitVehicleSync",			WaitVehicleSync },
        { "GetPlayerActiveCheats",		GetPlayerActiveCheats },

        { "SKY_GivePlayerWeapon",		hook_GivePlayerWeapon },
        { "SKY_ResetPlayerWeapons",		hook_ResetPlayerWeapons },
        { "SyncSetVehicleHealth",		hook_SetVehicleHealth },
        { "SyncRepairVehicle",  		hook_RepairVehicle },
        { "SKY_SetVehiclePos",			hook_SetVehiclePos },
        { "SKY_SetPlayerPosFindZ",		hook_SetPlayerPosFindZ },
        { "SKY_SetVehicleVelocity",		hook_SetVehicleVelocity },
        { "SKY_AddStaticVehicleEx",		SKY_AddStaticVehicleEx },
        { "SKY_AddStaticVehicle",		SKY_AddStaticVehicle },
        { "SKY_CreateVehicle",			SKY_CreateVehicle },
        { "SKY_PutPlayerInVehicle",		hook_PutPlayerInVehicle },
        { "SKY_SetPlayerPos",			SKY_SetPlayerPos },
        { "SKY_SetPlayerVelocity",		SKY_SetPlayerVelocity },
        { "SKY_SetVehicleAngularVelocity",hook_SetVehicleAngularVelocity },
        { "SKY_IsPlayerSpawned",		SKY_IsPlayerSpawned},
        { "SKY_SpawnPlayer",			SKY_SpawnPlayer },
        { "SKY_GetPlayerState",			SKY_GetPlayerState },
        { "SKY_GetPlayerHealth",		SKY_GetPlayerHealth },
        { "SKY_SetPlayerHealth",		SKY_SetPlayerHealth },
        { "SKY_GetPlayerArmour",		SKY_GetPlayerArmour },
        { "SKY_SetPlayerArmour",		SKY_SetPlayerArmour },
        { "SKY_GetPlayerTeam",			SKY_GetPlayerTeam },
        { "SKY_SetPlayerTeam",			SKY_SetPlayerTeam },
        { "SKY_SendDeathMessage",		SKY_SendDeathMessage },
        { "SKY_AddPlayerClass",			SKY_AddPlayerClass },
        { "SKY_AddPlayerClassEx",		SKY_AddPlayerClassEx },
        { "SKY_SetSpawnInfo",			SKY_SetSpawnInfo },
        { "SKY_TogglePlayerSpectating",	SKY_TogglePlayerSpectating },
        { "SKY_TogglePlayerControllable",SKY_TogglePlayerControllable },
        { "SKY_SetPlayerVirtualWorld",	SKY_SetPlayerVirtualWorld },
        { "SKY_GetPlayerVirtualWorld",	SKY_GetPlayerVirtualWorld },
        { "SKY_PlayerSpectatePlayer",	SKY_PlayerSpectatePlayer },
        { "SKY_IsPlayerSpawned",		SKY_IsPlayerSpawned },
        { "InitializeAntiHack",			SKY_InitializeAntiHack },
        { "SKY_OnPlayerDamage",			SKY_OnPlayerDamage },
        { "UpdateOnPlayerDamage",		SKY_UpdateOnPlayerDamage },
        //{ "SKY_OnPlayerDeath",			SKY_OnPlayerDeath },
        { "SKY_DestroyVehicle",			SKY_DestroyVehicle },
        //{ "GetPlayerDifAim",			SKY_GetPlayerDifAim },
        { "IsPlayerBot",				SKY_IsPlayerBot },
        { 0,							0 }
    };

    int InitScripting(AMX *amx)
    {
        AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);
        AMX_FUNCSTUBNT *natives = reinterpret_cast<AMX_FUNCSTUBNT*>(amx->base + hdr->natives);
        int index = 0;
        if (amx_FindNative(amx, "GivePlayerWeapon", &index) == AMX_ERR_NONE) {
            original_GivePlayerWeapon = reinterpret_cast<AMX_NATIVE>(natives[index].address);
            natives[index].address = reinterpret_cast<ucell>(hook_GivePlayerWeapon);
            if (debug)logprintf(" GivePlayerWeapon found and hooked successfully!");
        }
        else if (debug)logprintf(" GivePlayerWeapon is not used in this script...");
        index = 0;
        if (amx_FindNative(amx, "ResetPlayerWeapons", &index) == AMX_ERR_NONE) {
            original_ResetPlayerWeapons = reinterpret_cast<AMX_NATIVE>(natives[index].address);
            natives[index].address = reinterpret_cast<ucell>(hook_ResetPlayerWeapons);
            if (debug)logprintf(" ResetPlayerWeapons found and hooked successfully!");
        }
        else if (debug)logprintf(" ResetPlayerWeapons is not used in this script...");
        index = 0;
        if (amx_FindNative(amx, "SetVehicleHealth", &index) == AMX_ERR_NONE) {
            original_SetVehicleHealth = reinterpret_cast<AMX_NATIVE>(natives[index].address);
            natives[index].address = reinterpret_cast<ucell>(hook_SetVehicleHealth);
            if (debug)logprintf(" SetVehicleHealth found and hooked successfully!");
        }
        else if (debug)logprintf(" SetVehicleHealth is not used in this script...");
        index = 0;
        if (amx_FindNative(amx, "RepairVehicle", &index) == AMX_ERR_NONE) {
            original_RepairVehicle = reinterpret_cast<AMX_NATIVE>(natives[index].address);
            natives[index].address = reinterpret_cast<ucell>(hook_RepairVehicle);
            if (debug)logprintf(" RepairVehicle found and hooked successfully!");
        }
        else if (debug)logprintf(" RepairVehicle is not used in this script...");
        index = 0;
        if (amx_FindNative(amx, "SetVehiclePos", &index) == AMX_ERR_NONE) {
            original_SetVehiclePos = reinterpret_cast<AMX_NATIVE>(natives[index].address);
            natives[index].address = reinterpret_cast<ucell>(hook_SetVehiclePos);
            if (debug)logprintf(" SetVehiclePos found and hooked successfully!");
        }
        else if (debug)logprintf(" SetVehiclePos is not used in this script...");
        index = 0;
        if (amx_FindNative(amx, "SetVehicleVelocity", &index) == AMX_ERR_NONE) {
            original_SetVehicleVelocity = reinterpret_cast<AMX_NATIVE>(natives[index].address);
            natives[index].address = reinterpret_cast<ucell>(hook_SetVehicleVelocity);
            if (debug)logprintf(" SetVehicleVelocity found and hooked successfully!");
        }
        else if (debug)logprintf(" SetVehicleVelocity is not used in this script...");
        index = 0;
        if (amx_FindNative(amx, "PutPlayerInVehicle", &index) == AMX_ERR_NONE) {
            original_PutPlayerInVehicle = reinterpret_cast<AMX_NATIVE>(natives[index].address);
            natives[index].address = reinterpret_cast<ucell>(hook_PutPlayerInVehicle);
            if (debug)logprintf(" PutPlayerInVehicle found and hooked successfully!");
        }
        else if (debug)logprintf(" PutPlayerInVehicle is not used in this script...");
        index = 0;
        if (amx_FindNative(amx, "SetPlayerPos", &index) == AMX_ERR_NONE) {
            original_SetPlayerPos = reinterpret_cast<AMX_NATIVE>(natives[index].address);
            natives[index].address = reinterpret_cast<ucell>(hook_SetPlayerPos);
            if (debug)logprintf(" SetPlayerPos found and hooked successfully!");
        }
        else if (debug)logprintf(" SetPlayerPos is not used in this script...");
        index = 0;
        if (amx_FindNative(amx, "SetPlayerVelocity", &index) == AMX_ERR_NONE) {
            original_SetPlayerVelocity = reinterpret_cast<AMX_NATIVE>(natives[index].address);
            natives[index].address = reinterpret_cast<ucell>(hook_SetPlayerVelocity);
            if (debug)logprintf(" SetPlayerVelocity found and hooked successfully!");
        }
        else if (debug)logprintf(" SetPlayerVelocity is not used in this script...");
        index = 0;
        if (amx_FindNative(amx, "SetVehicleAngularVelocity", &index) == AMX_ERR_NONE) {
            original_SetVehicleAngularVelocity = reinterpret_cast<AMX_NATIVE>(natives[index].address);
            natives[index].address = reinterpret_cast<ucell>(hook_SetVehicleAngularVelocity);
            if (debug)logprintf(" SetVehicleAngularVelocity found and hooked successfully!");
        }
        else if (debug)logprintf(" SetVehicleAngularVelocity is not used in this script...");
        index = 0;
        if (amx_FindNative(amx, "SetPlayerPosFindZ", &index) == AMX_ERR_NONE) {
            original_SetPlayerPosFindZ = reinterpret_cast<AMX_NATIVE>(natives[index].address);
            natives[index].address = reinterpret_cast<ucell>(hook_SetPlayerPosFindZ);
            if (debug)logprintf(" SetPlayerPosFindZ found and hooked successfully!");
        }
        else if (debug)logprintf(" SetPlayerPosFindZ is not used in this script...");
        return amx_Register(amx, Natives, -1);
    }
}