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
#include <map>

#if !defined PAD
    #define PAD(a, b) char a[b]
#endif

#include "Structs.h"

namespace Hack
{
    BYTE SyncWeapons[1000][13] = {0};
    bool pDisSync[1000] = {0};
    BYTE DisableSyncVeh[2000] = { 0 };
    BYTE DisableSyncPly[1000] = { 0 };
    DWORD lastSyncVeh[1000] = { 0 };
    DWORD lastSyncAir[1000] = { 0 };
    DWORD lastSyncFly[1000] = { 0 };
    BYTE flywarning[1000] = { 0 };
    BYTE airwarning[1000] = { 0 };
    int hacks[1000] = { 0 };
    BYTE LastPacketID[1000] = { 0 };
    int PlayerActiveCheats[1000] = { 0 };
    BYTE atwarning[1000] = { 0 };
    BYTE PlayerWeapon;
    WORD LastVehLog[1000][3][2];

    int const WeaponSlots[47] =
    {
        0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 10, 10,
        10, 10, 10, 10, 8, 8, 8, -1, -1, -1,
        2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 4, 6,
        6, 7, 7, 7, 7, 8, 12, 9, 9, 9, 11,
        11, 11
    };

    static bool AntiHackCheck(Packet *p, BYTE packetid, WORD playerid)
    {
        if (!PlayerActiveCheats[playerid])return true;
        bool rtn = true;
        bool fake = false;
        switch (packetid)
        {
            case ID_PLAYER_SYNC:
            case ID_VEHICLE_SYNC:
            case ID_PASSENGER_SYNC:
            case ID_SPECTATOR_SYNC:
            {
                if (LastPacketID[playerid] != packetid)
                {				
                    if(LastPacketID[playerid] == ID_VEHICLE_SYNC || LastPacketID[playerid] == ID_PASSENGER_SYNC)
                        DisableSyncPly[playerid] = 25;
                    else
                        DisableSyncPly[playerid] = 10;
                    LastPacketID[playerid] = packetid;
                }
                break;
            }
        }
        int AfterUpdate = GetTickCount() - Player::lastUpdateTick[playerid];
        if ((AfterUpdate) > 1500)
        {
            DisableSyncPly[playerid] = 30;
        }
        switch (packetid)
        {
            case ID_PLAYER_SYNC:
            {
                if (p->bitSize != 552)
                {
                    return 0;
                }
                CSyncData *d = (CSyncData*)(&p->data[1]);
                CPlayer *player = pNetGame->pPlayerPool->pPlayer[playerid];
                if (!DisableSyncPly[playerid])
                {
                    PlayerWeapon = d->byteWeapon & ~(192);
                    if ((PlayerActiveCheats[playerid] & HACK_WEAPON) && PlayerWeapon > 0 && PlayerWeapon < 46 && PlayerWeapon != WEAPON_PARACHUTE && PlayerWeapon != WEAPON_BOMB && PlayerWeapon != SyncWeapons[playerid][WeaponSlots[PlayerWeapon]])
                    {
                        hacks[playerid] |= HACK_WEAPON;
                        SyncWeapons[playerid][WeaponSlots[PlayerWeapon]] = PlayerWeapon;
                    }
                    
                    if (pNetGame->pPlayerPool->pPlayer[playerid]->byteState == PLAYER_STATE_ONFOOT)
                    {
                        if ((PlayerActiveCheats[playerid] & HACK_TELEPORT))
                        {
                            CVector a = (d->vecPosition - player->vecPosition);
                            if (d->wSurfingInfo == 0)
                            {
                                float dist = a.Length2();
                                if (dist > 7.0)
                                {
                                    atwarning[playerid]++;
                                    if (atwarning[playerid] >= 7)
                                    {
                                        hacks[playerid] |= HACK_TELEPORT;
                                    }
                                }else
                                    if (atwarning[playerid] != 0)atwarning[playerid] = 0;
                                if (dist > 20.0 && d->vecVelocity.Length2() < 0.001)
                                {
                                    hacks[playerid] |= HACK_TELEPORT;
                                }
                            }					
                        }
                        if((PlayerActiveCheats[playerid] & HACK_AIRBREAK) && (d->wLRAnalog != 0 || d->wUDAnalog != 0))
                        {
                            CVector a = (d->vecPosition - player->vecPosition);
                            if (a.Length2() > 5.0 && d->vecVelocity.fZ == 0)
                            {
                                if(lastSyncAir[playerid] > GetTickCount())
                                    airwarning[playerid]++;
                                else if (airwarning[playerid] != 0)airwarning[playerid] = 0;
                                if (airwarning[playerid] == 3)
                                {
                                    hacks[playerid] |= HACK_AIRBREAK;
                                }			
                                lastSyncAir[playerid] = GetTickCount() + 220;
                            }
                        }
                        if ((PlayerActiveCheats[playerid] & HACK_FLY))
                        {
                            if ((1537 < d->wAnimIndex && d->wAnimIndex < 1545 || 156 < d->wAnimIndex && d->wAnimIndex < 162))
                            {
                                if (d->vecVelocity.Length() > 0.30)
                                {
                                    if (lastSyncFly[playerid] > GetTickCount())
                                        flywarning[playerid]++;
                                    else if (flywarning[playerid] != 0)flywarning[playerid] = 0;
                                    if (flywarning[playerid] == 3)
                                    {
                                        hacks[playerid] |= HACK_FLY;
                                    }
                                    lastSyncFly[playerid] = GetTickCount() + 220;
                                }
                            }
                            else if (PlayerWeapon != WEAPON_PARACHUTE && 957 < d->wAnimIndex && d->wAnimIndex < 960)
                            {
                                if (d->vecVelocity.Length2() > 0.55)
                                {
                                    if (lastSyncFly[playerid] > GetTickCount())
                                        flywarning[playerid]++;
                                    else if (flywarning[playerid] != 0)flywarning[playerid] = 0;
                                    if (flywarning[playerid] == 3)
                                    {
                                        hacks[playerid] |= HACK_FLY;
                                    }
                                    lastSyncFly[playerid] = GetTickCount() + 220;
                                }
                            }
                        }
                        if ((PlayerActiveCheats[playerid] & HACK_AFLY))
                        {
                            if (d->wSurfingInfo == 0 && d->byteSpecialAction != 2 && d->vecVelocity.Length2() > 0.55f && (PlayerWeapon != WEAPON_PARACHUTE || d->vecVelocity.fZ > 0.0f))
                            {
                                if (lastSyncFly[playerid] > GetTickCount())
                                    flywarning[playerid]++;
                                else if (flywarning[playerid] != 0)flywarning[playerid] = 0;
                                if (flywarning[playerid] == 45)
                                {
                                    hacks[playerid] |= HACK_AFLY;
                                }
                                lastSyncFly[playerid] = GetTickCount() + 220;
                            }						
                        }		
                        if (PlayerActiveCheats[playerid] & HACK_TROLL) {
                            if (d->vecVelocity.Length2() > 20.0) {
                                rtn = false;
                            }
                        }
                        if (d->byteSpecialAction == 3)
                        {
                            d->wKeys &= ~8;
                        }
                    }
                }else {
                    DisableSyncPly[playerid] --;
                }
                break;
            }
            case ID_VEHICLE_SYNC:
            {
                if (p->bitSize != 512)
                {
                    return 0;
                }
                CVehicleSyncData *d = (CVehicleSyncData*)(&p->data[1]);
                WORD vehicleid = d->wVehicleId;
                if ((GetTickCount() - Player::lastUpdateTick[playerid]) > 1500)
                {
                    DisableSyncVeh[vehicleid] = 5;
                }
                if (vehicleid > 0 && vehicleid <= 1999)
                {
                    if (pNetGame->pVehiclePool->pVehicle[vehicleid] != NULL)
                    {
                        CVehicle *veh = pNetGame->pVehiclePool->pVehicle[vehicleid];
                        if (PlayerActiveCheats[playerid] & HACK_TROLL) {
                            if (!DisableSyncVeh[vehicleid]) {
                                if (pNetGame->pPlayerPool->pPlayer[playerid]->wVehicleId != 0)
                                {
                                    if (pNetGame->pPlayerPool->pPlayer[playerid]->wVehicleId != vehicleid)
                                    {
                                        WORD lastdriver = pNetGame->pVehiclePool->pVehicle[vehicleid]->wLastDriverID;
                                        if (lastdriver != INVALID_PLAYER_ID && lastdriver >= 0 && lastdriver < 1000 && pNetGame->pPlayerPool->bIsPlayerConnected[lastdriver] && pNetGame->pPlayerPool->pPlayer[lastdriver]->byteState == 2 && pNetGame->pPlayerPool->pPlayer[lastdriver]->wVehicleId == vehicleid)
                                        {
                                            hacks[playerid] |= HACK_TROLL;
                                            rtn = false;
                                            fake = true;
                                        }
                                        else
                                        {
                                            d->wKeys &= ~8;
                                        }
                                    }
                                }
                                else
                                {
                                    if (PlayerActiveCheats[playerid] & HACK_ATELEPORT) {
                                        CVector a = (d->vecPosition - pNetGame->pPlayerPool->pPlayer[playerid]->vecPosition);
                                        CVector b = (d->vecPosition - veh->vecPosition);
                                        if (!DisableSyncPly[playerid]) {
                                            if (a.Length() > 14.0 || b.Length() > 28.0)
                                            {
                                                hacks[playerid] |= HACK_ATELEPORT;
                                                logprintf("ID: %d Jump Vehicle %.2f %.2f %d", playerid, a.Length(), b.Length(), AfterUpdate);
                                                fake = true;
                                                if (d->fHealth <= 250.0 && veh->fHealth > 250.0) {
                                                    d->fHealth = veh->fHealth;
                                                }
                                            }
                                        }
                                    }
                                    d->wKeys &= ~8;
                                }
                            }
                        }
                    
                        if(!fake)
                        {
                            if ((PlayerActiveCheats[playerid] & HACK_WEAPON))
                            {
                                if (!DisableSyncPly[playerid]) {
                                    PlayerWeapon = d->bytePlayerWeapon & ~(192);
                                    if (PlayerWeapon > 0 && PlayerWeapon < 46 && PlayerWeapon != WEAPON_PARACHUTE && PlayerWeapon != WEAPON_BOMB && PlayerWeapon != SyncWeapons[playerid][WeaponSlots[PlayerWeapon]])
                                    {
                                        hacks[playerid] |= HACK_WEAPON;
                                        SyncWeapons[playerid][WeaponSlots[PlayerWeapon]] = PlayerWeapon;
                                    }
                                }
                            }
                            if ((PlayerActiveCheats[playerid] & HACK_ATROLL) && pNetGame->pPlayerPool->pPlayer[playerid]->wVehicleId != vehicleid)
                            {
                                if (!DisableSyncVeh[vehicleid]) {
                                    if (lastSyncVeh[playerid] > GetTickCount())
                                    {
                                        if (!atwarning[playerid] && LastVehLog[playerid][0][1] != vehicleid)
                                        {
                                            atwarning[playerid]++;
                                            if (atwarning[playerid] < 3)
                                            {
                                                LastVehLog[playerid][atwarning[playerid]][0] = pNetGame->pPlayerPool->pPlayer[playerid]->wVehicleId;
                                                LastVehLog[playerid][atwarning[playerid]][1] = vehicleid;
                                            }
                                            if (atwarning[playerid] > 3)
                                            {
                                                hacks[playerid] |= HACK_ATROLL;
                                                rtn = false;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        if (atwarning[playerid] != 0)atwarning[playerid] = 0;
                                        LastVehLog[playerid][atwarning[playerid]][0] = pNetGame->pPlayerPool->pPlayer[playerid]->wVehicleId;
                                        LastVehLog[playerid][atwarning[playerid]][1] = vehicleid;
                                    }
                                    lastSyncVeh[playerid] = GetTickCount() + 220;
                                }
                            }
                            bool isbreak = false;
                            if (DisableSyncPly[playerid] > 0)
                            {
                                DisableSyncPly[playerid]--;
                                isbreak = true;
                            }
                            if (DisableSyncVeh[vehicleid] > 0)
                            {
                                DisableSyncVeh[vehicleid]--;
                                isbreak = true;
                            }
                            if (isbreak)break;
                            if ((PlayerActiveCheats[playerid] & HACK_TELEPORT))
                            {
                                CVector a = (d->vecPosition - veh->vecPosition);
                                if (a.Length2() > 25.0 && d->vecVelocity.Length2() < 0.001 || a.Length2() > 50.0)
                                {
                                    hacks[playerid] |= HACK_TELEPORT;
                                    //logprintf("ID: %d Veh Teleport: %f,%f,%f(%f,%f) L: %d", playerid, a.fX, a.fY, a.fZ, a.Length(), a.Length2(), AfterUpdate);
                                    //rtn = false;
                                }							
                            }
                            if ((PlayerActiveCheats[playerid] & HACK_SPEED) && d->wKeys & 4 && d->vecVelocity.Length2() > veh->vecVelocity.Length2())
                            {
                                CVector a = (d->vecVelocity - veh->vecVelocity);
                                CVector b = (d->vecVelocity + veh->vecVelocity);
                                if (a.Length2() < b.Length2())
                                {
                                    if (AfterUpdate > 50)
                                        a /= ((float)AfterUpdate / 50);
                                    if ((a.Length2() > 0.10 || d->vecVelocity.fastLength2() > 5.3f) && veh->vecVelocity.fZ > -0.5 && veh->vecVelocity.fZ < 0.5 && d->vecVelocity.Length2() > veh->vecVelocity.Length2())
                                    {
                                        if (d->fHealth > 250.0f && pNetGame->pVehiclePool->pVehicle[vehicleid]->fHealth > 250.0f && pNetGame->pVehiclePool->pVehicle[vehicleid]->bDead == false && d->fHealth == veh->fHealth)
                                        {
                                            hacks[playerid] |= HACK_SPEED;
                                        }
                                    }
                                }
                            }
                            if ((PlayerActiveCheats[playerid] & HACK_AIRBREAK) && d->vecVelocity.Length() < 0.001 && veh->vecVelocity.Length() < 0.001 && pNetGame->pPlayerPool->pPlayer[playerid]->byteState == 2 && pNetGame->pPlayerPool->pPlayer[playerid]->wVehicleId == vehicleid)
                            {
                                CVector a = (d->vecPosition - veh->vecPosition);
                                if ((d->wLRAnalog != 0 || d->wUDAnalog != 0 || d->wKeys != 0) && a.Length() > 0.4)
                                {
                                    hacks[playerid] |= HACK_AIRBREAK;
                                }								
                            }
                            if ((PlayerActiveCheats[playerid] & HACK_VHEALTH) && d->fHealth > veh->fHealth && d->fHealth == 1000.0f)
                            {
                                if (pNetGame->pPlayerPool->pPlayer[playerid]->byteState == 2 && pNetGame->pPlayerPool->pPlayer[playerid]->wVehicleId == vehicleid)
                                {
                                    hacks[playerid] |= HACK_VHEALTH;
                                    rtn = false;
                                }
                            }
                        }
                    }
                }
                break;
            }
            case ID_PASSENGER_SYNC:
            {
                if (p->bitSize != 200)
                {
                    return 0;
                }
                if (DisableSyncPly[playerid] > 0) {
                    DisableSyncPly[playerid]--;
                    break;
                }
                if ((PlayerActiveCheats[playerid] & HACK_WEAPON))
                {
                    CPassengerSyncData *d = (CPassengerSyncData*)(&p->data[1]);
                    PlayerWeapon = d->bytePlayerWeapon & ~(192);
                    if (PlayerWeapon > 0 && PlayerWeapon < 46 && PlayerWeapon != WEAPON_PARACHUTE && PlayerWeapon != WEAPON_BOMB && PlayerWeapon != SyncWeapons[playerid][WeaponSlots[PlayerWeapon]])
                    {
                        hacks[playerid] |= HACK_WEAPON;
                        SyncWeapons[playerid][WeaponSlots[PlayerWeapon]] = PlayerWeapon;
                    }
                }	
                break;
            }
            case ID_UNOCCUPIED_SYNC:
            {
                if (p->bitSize != 544)
                {
                    return 0;
                }
                CUnoccupiedSyncData *d = (CUnoccupiedSyncData*)(&p->data[1]);
                if (d->wVehicleID > 0 && d->wVehicleID <= 1999)
                {
                    WORD vehicleid = d->wVehicleID;
                    if (pNetGame->pVehiclePool->pVehicle[vehicleid] != NULL)
                    {
                        CVehicle *veh = pNetGame->pVehiclePool->pVehicle[vehicleid];
                        if (pNetGame->pPlayerPool->pPlayer[playerid]->wVehicleId != vehicleid)
                        {
                            WORD lastdriver = pNetGame->pVehiclePool->pVehicle[vehicleid]->wLastDriverID;
                            if (lastdriver != INVALID_PLAYER_ID && lastdriver >= 0 && lastdriver < 1000 && pNetGame->pPlayerPool->bIsPlayerConnected[lastdriver] && pNetGame->pPlayerPool->pPlayer[lastdriver]->byteState == 2 && pNetGame->pPlayerPool->pPlayer[lastdriver]->wVehicleId == vehicleid)
                            {
                                rtn = false;
                            }
                            else
                            {
                                if (d->bytePassengerSlot != 0)
                                {
                                    rtn = false;
                                }
                                else
                                {
                                    CVector a = (d->vecPosition - veh->vecPosition);
                                    if (d->fHealth > 1000.0 || d->fHealth < 0.0 || abs(d->vecDirection.fX) > 1.0 || abs(d->vecDirection.fY) > 1.0 || abs(d->vecDirection.fZ) > 1.0 || abs(d->vecRool.fX) > 1.0 || abs(d->vecRool.fY) > 1.0 || abs(d->vecRool.fZ) > 1.0 || abs(d->vecTurnVelocity.fX) > 1.0 || abs(d->vecTurnVelocity.fY) > 1.0 || abs(d->vecTurnVelocity.fZ) > 1.0 ||
                                        abs(d->vecVelocity.fX) > 1.0 || abs(d->vecVelocity.fY) > 1.0 || abs(d->vecVelocity.fZ) > 1.0 || abs(d->vecPosition.fX) > 2500.0 || abs(d->vecPosition.fY) > 2500.0 || abs(d->vecPosition.fZ) > 1500.0 || a.Length() > 28.0)
                                    {
                                        rtn = false;
                                    }
                                }
                            }
                        }
                        else rtn = false;
                    }
                    else rtn = false;
                }
                else rtn = false;
                break;
            }
            case ID_AIM_SYNC:
            {
                if (p->bitSize != 256)
                {
                    return 0;
                }
                /*CAimSyncData *d = (CAimSyncData*)(&p->data[1]);
                for (int i = 2; i > 0; i--)
                {
                    LastFourAimTime[playerid][i] = LastFourAimTime[playerid][i - 1];
                    LastFourAim[playerid][i] = LastFourAim[playerid][i - 1];
                }
                LastFourAimTime[playerid][0] = GetTickCount();
                LastFourAim[playerid][0] = (d->vecFront - pNetGame->pPlayerPool->pPlayer[playerid]->aimSyncData.vecFront);*/
                break;
            }
        }
        return rtn;
    }
}