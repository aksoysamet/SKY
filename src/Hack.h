#define HACK_WEAPON		(1)
#define HACK_AIRBREAK	(2)
#define HACK_TROLL		(4)
#define HACK_VHEALTH	(8)
#define HACK_SPEED		(16)
#define HACK_FLY		(32)
#define HACK_ATROLL		(64)
#define HACK_TELEPORT	(128)
#define HACK_ATELEPORT	(256)
#define HACK_AFLY		(512)

namespace Hack
{
    extern BYTE SyncWeapons[1000][13];
    extern bool pDisSync[1000];
    extern BYTE DisableSyncVeh[2000];
    extern BYTE DisableSyncPly[1000];
    extern DWORD lastSyncVeh[1000];
    extern DWORD lastSyncAir[1000];
    extern DWORD lastSyncFly[1000];
    extern BYTE flywarning[1000];
    extern BYTE airwarning[1000];
    extern int hacks[1000];
    extern BYTE LastPacketID[1000];
    extern int PlayerActiveCheats[1000];
    extern BYTE atwarning[1000];
    extern BYTE PlayerWeapon;
    extern WORD LastVehLog[1000][3][2];

    extern int const WeaponSlots[47];

    bool AntiHackCheck(Packet *p, BYTE packetid, WORD playerid);
}