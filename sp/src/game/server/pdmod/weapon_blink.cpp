//#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
//#include "tier0/memdbgon.h"


//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "npcevent.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#include "game.h"
#include "in_buttons.h"
#include "ai_memory.h"
#include "soundent.h"
#include "rumble_shared.h"
#include "gamestats.h"

#include "physics_saverestore.h"

#include "entities_blink.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar sk_plr_dmg_smg1_grenade;

#define TELEPORT_END_ENTITY_DISTANCE	10
#define TELEPORT_SPRING_CONSTANT		10000
#define TELEPORT_SPRING_DAMPING			20

//-----------------------------------------------------------------------------
// CWeaponMP5
//-----------------------------------------------------------------------------
class CWeaponMP5 : public CHLSelectFireMachineGun
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS(CWeaponMP5, CHLSelectFireMachineGun);

	CWeaponMP5();

	DECLARE_SERVERCLASS();

	//---------------------------------------------------
	// pdmod overrides
	//---------------------------------------------------
public:
	virtual void PrimaryAttack();

	//virtual void Activate()
	virtual void Spawn()
	{
		m_pTeleporter = (IBlinkTeleporter*)CBaseEntity::Create("blink_teleporter", GetAbsOrigin(), GetAbsAngles());

		DEBUG_SpawnMyModelEntity();
		//BaseClass::Activate();
		BaseClass::Spawn();
	}

private:
	void TryTeleport();
	void DEBUG_SpawnMyModelEntity();

	CBaseEntity * m_pEndEntity;
	IPhysicsSpring * m_pPhysSpring;
	CBaseEntity * m_pSpring;

	IBlinkTeleporter * m_pTeleporter;

public:

	void			Precache();
	void			AddViewKick();
	void			SecondaryAttack();

	int				GetMinBurst() { return 2; }
	int				GetMaxBurst() { return 5; }

	virtual void	Equip(CBaseCombatCharacter *pOwner);
	bool			Reload();

	//float			GetFireRate() { return 0.075f; } // RPS, 60sec/800 rounds = 0.075
	float			GetFireRate() { return 1; } // RPS
	int				CapabilitiesGet() { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	int				WeaponRangeAttack2Condition(float flDot, float flDist);
	Activity		GetPrimaryAttackActivity();

	// Values which allow our "spread" to change from button input from player
	virtual const Vector &GetBulletSpread()
	{
		// Define "spread" parameters based on the "owner" and what they are doing
		static Vector plrDuckCone = VECTOR_CONE_2DEGREES;
		static Vector plrStandCone = VECTOR_CONE_3DEGREES;
		static Vector plrMoveCone = VECTOR_CONE_4DEGREES;
		static Vector npcCone = VECTOR_CONE_5DEGREES;
		static Vector plrRunCone = VECTOR_CONE_6DEGREES;
		static Vector plrJumpCone = VECTOR_CONE_9DEGREES;

		if (GetOwner() && GetOwner()->IsNPC())
			return npcCone;

		//static Vector cone;

		// We must know the player "owns" the weapon before different cones may be used
		CBasePlayer *pPlayer = ToBasePlayer(GetOwnerEntity());
		if (pPlayer->m_nButtons & IN_DUCK)
			return plrDuckCone;
		if (pPlayer->m_nButtons & IN_FORWARD)
			return plrMoveCone;
		if (pPlayer->m_nButtons & IN_BACK)
			return plrMoveCone;
		if (pPlayer->m_nButtons & IN_MOVERIGHT)
			return plrMoveCone;
		if (pPlayer->m_nButtons & IN_MOVELEFT)
			return plrMoveCone;
		if (pPlayer->m_nButtons & IN_JUMP)
			return plrJumpCone;
		if (pPlayer->m_nButtons & IN_SPEED)
			return plrRunCone;
		if (pPlayer->m_nButtons & IN_RUN)
			return plrRunCone;
		else
			return plrStandCone;
	}

	const WeaponProficiencyInfo_t *GetProficiencyValues();

	void FireNPCPrimaryAttack(CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir);
	void Operator_ForceNPCFire(CBaseCombatCharacter *pOperator, bool bSecondary);
	void Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator);

	DECLARE_ACTTABLE();
};

IMPLEMENT_SERVERCLASS_ST(CWeaponMP5, DT_WeaponMP5)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(weapon_mp5, CWeaponMP5);
PRECACHE_WEAPON_REGISTER(weapon_mp5);

BEGIN_DATADESC(CWeaponMP5)
	DEFINE_PHYSPTR(m_pPhysSpring),
END_DATADESC()

acttable_t CWeaponMP5::m_acttable[] =
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SMG1, true },
	{ ACT_RELOAD, ACT_RELOAD_SMG1, true },
	{ ACT_IDLE, ACT_IDLE_SMG1, true },
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_SMG1, true },

	{ ACT_WALK, ACT_WALK_RIFLE, true },
	{ ACT_WALK_AIM, ACT_WALK_AIM_RIFLE, true },

	// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED, ACT_IDLE_SMG1_RELAXED, false }, // Never aims
	{ ACT_IDLE_STIMULATED, ACT_IDLE_SMG1_STIMULATED, false },
	{ ACT_IDLE_AGITATED, ACT_IDLE_ANGRY_SMG1, false }, // Always aims

	{ ACT_WALK_RELAXED, ACT_WALK_RIFLE_RELAXED, false }, // Never aims
	{ ACT_WALK_STIMULATED, ACT_WALK_RIFLE_STIMULATED, false },
	{ ACT_WALK_AGITATED, ACT_WALK_AIM_RIFLE, false }, // Always aims

	{ ACT_RUN_RELAXED, ACT_RUN_RIFLE_RELAXED, false }, // Never aims
	{ ACT_RUN_STIMULATED, ACT_RUN_RIFLE_STIMULATED, false },
	{ ACT_RUN_AGITATED, ACT_RUN_AIM_RIFLE, false }, // Always aims

	// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED, ACT_IDLE_SMG1_RELAXED, false }, // Never aims
	{ ACT_IDLE_AIM_STIMULATED, ACT_IDLE_AIM_RIFLE_STIMULATED, false },
	{ ACT_IDLE_AIM_AGITATED, ACT_IDLE_ANGRY_SMG1, false }, // Always aims

	{ ACT_WALK_AIM_RELAXED, ACT_WALK_RIFLE_RELAXED, false }, // Never aims
	{ ACT_WALK_AIM_STIMULATED, ACT_WALK_AIM_RIFLE_STIMULATED, false },
	{ ACT_WALK_AIM_AGITATED, ACT_WALK_AIM_RIFLE, false }, // Always aims

	{ ACT_RUN_AIM_RELAXED, ACT_RUN_RIFLE_RELAXED, false }, // Never aims
	{ ACT_RUN_AIM_STIMULATED, ACT_RUN_AIM_RIFLE_STIMULATED, false },
	{ ACT_RUN_AIM_AGITATED, ACT_RUN_AIM_RIFLE, false }, // Always aims
	// End readiness activities

	{ ACT_WALK_AIM, ACT_WALK_AIM_RIFLE, true },
	{ ACT_WALK_CROUCH, ACT_WALK_CROUCH_RIFLE, true },
	{ ACT_WALK_CROUCH_AIM, ACT_WALK_CROUCH_AIM_RIFLE, true },
	{ ACT_RUN, ACT_RUN_RIFLE, true },
	{ ACT_RUN_AIM, ACT_RUN_AIM_RIFLE, true },
	{ ACT_RUN_CROUCH, ACT_RUN_CROUCH_RIFLE, true },
	{ ACT_RUN_CROUCH_AIM, ACT_RUN_CROUCH_AIM_RIFLE, true },
	{ ACT_GESTURE_RANGE_ATTACK1, ACT_GESTURE_RANGE_ATTACK_SMG1, true },
	{ ACT_RANGE_ATTACK1_LOW, ACT_RANGE_ATTACK_SMG1_LOW, true },
	{ ACT_COVER_LOW, ACT_COVER_SMG1_LOW, false },
	{ ACT_RANGE_AIM_LOW, ACT_RANGE_AIM_SMG1_LOW, false },
	{ ACT_RELOAD_LOW, ACT_RELOAD_SMG1_LOW, false },
	{ ACT_GESTURE_RELOAD, ACT_GESTURE_RELOAD_SMG1, true },
};

IMPLEMENT_ACTTABLE(CWeaponMP5);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWeaponMP5::CWeaponMP5()
{
	m_fMinRange1 = 0; // No minimum range
	m_fMaxRange1 = 1400;

	m_pEndEntity = NULL;
	m_pSpring = NULL;
	m_pTeleporter = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMP5::Precache()
{
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Give this weapon longer range when wielded by an ally NPC.
//-----------------------------------------------------------------------------
void CWeaponMP5::Equip(CBaseCombatCharacter *pOwner)
{
	if (pOwner->Classify() == CLASS_PLAYER_ALLY)
		m_fMaxRange1 = 3000;
	else
		m_fMaxRange1 = 1400;

	BaseClass::Equip(pOwner);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMP5::FireNPCPrimaryAttack(CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir)
{
	// FIXME: Use the returned number of bullets to account for >10hz firerate
	WeaponSoundRealtime(SINGLE_NPC);

	CSoundEnt::InsertSound(SOUND_COMBAT | SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy());
	pOperator->FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2, entindex(), 0);

	pOperator->DoMuzzleFlash();
	m_iClip1--;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMP5::Operator_ForceNPCFire(CBaseCombatCharacter *pOperator, bool bSecondary)
{
	// Ensure we have enough rounds in the magazine
	m_iClip1++;

	Vector vecShootOrigin, vecShootDir;
	QAngle angShootDir;
	GetAttachment(LookupAttachment("muzzle"), vecShootOrigin, angShootDir);
	AngleVectors(angShootDir, &vecShootDir);
	FireNPCPrimaryAttack(pOperator, vecShootOrigin, vecShootDir);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMP5::Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator)
{
	switch (pEvent->event)
	{
	case EVENT_WEAPON_SMG1:
	{
		Vector vecShootOrigin, vecShootDir;
		QAngle angDiscard;

		// Support old style attachment point firing
		if ((pEvent->options == NULL) || (pEvent->options[0] == '\0') || (!pOperator->GetAttachment(pEvent->options, vecShootOrigin, angDiscard)))
			vecShootOrigin = pOperator->Weapon_ShootPosition();

		CAI_BaseNPC *npc = pOperator->MyNPCPointer();
		ASSERT(npc != NULL);
		vecShootDir = npc->GetActualShootTrajectory(vecShootOrigin);

		FireNPCPrimaryAttack(pOperator, vecShootOrigin, vecShootDir);
	}
	break;

	default:
		BaseClass::Operator_HandleAnimEvent(pEvent, pOperator);
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CWeaponMP5::GetPrimaryAttackActivity()
{
	if (m_nShotsFired < 2)
		return ACT_VM_PRIMARYATTACK;

	if (m_nShotsFired < 3)
		return ACT_VM_RECOIL1;

	if (m_nShotsFired < 4)
		return ACT_VM_RECOIL2;

	return ACT_VM_RECOIL3;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponMP5::Reload()
{
	bool fRet = DefaultReload(GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD);
	if (fRet)
		WeaponSound(RELOAD);

	return fRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMP5::AddViewKick()
{
#define EASY_DAMPEN			2.3f
#define MAX_VERTICAL_KICK	17.0f // Degrees
#define SLIDE_LIMIT			0.11f // Seconds

	// Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if (pPlayer == NULL)
		return;

	DoMachineGunKick(pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT);
}

//-----------------------------------------------------------------------------
// Purpose: Shoot bullets but also teleport player
// NOTE(pd): pdmod override
//-----------------------------------------------------------------------------
void CWeaponMP5::PrimaryAttack()
{
	BaseClass::PrimaryAttack();
	TryTeleport();
	//DEBUG_SpawnMyModelEntity();
}

void CWeaponMP5::TryTeleport()
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
#if 1
	trace_t trace;
	TraceFromPlayerAimInfinitely(trace);
#elif 0

	if (m_pTeleporter->IsAbsTargetPositionValid() == false) { return; }

	Vector teleportTargetPosition;
	if (!m_pTeleporter->GetAbsTargetPosition(&teleportTargetPosition)) { return; }
	pPlayer->Teleport(&teleportTargetPosition, NULL, NULL);
	
#elif 0
	IPhysicsObject *pPhysObject = pPlayer->VPhysicsGetObject();

	//QAngle absEyeAngles = pPlayer->GetAbsAngles();
	QAngle absEyeAngles = pPlayer->EyeAngles();

	Vector forward, right, up;
	AngleVectors(absEyeAngles, &forward, &right, &up);

	Vector mins = pPlayer->GetPlayerMins();
	Vector maxs = pPlayer->GetPlayerMaxs();
	//maxs.z = mins.z + 1;

	//const bool isTeleport = true;
	Vector startPosition, endPosition;
	QAngle angles;

	pPhysObject->GetPosition(&startPosition, &angles);

	endPosition = startPosition;

	endPosition = startPosition + forward * 10000;

	Ray_t ray;
	ray.Init(startPosition, endPosition, mins, maxs);
	trace_t trace;

#if 1
	//UTIL_TraceRay(ray, MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_NONE, &trace);
	UTIL_TraceRay(ray, MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_DEBRIS, &trace);
#else
	CTraceFilterWorldOnly filter;
	UTIL_TraceHull(startPosition, endPosition, mins, maxs, MASK_SOLID_BRUSHONLY, &filter, &trace);
#endif
#endif

	if (trace.DidHit())
	{

		Vector up, down;
		AngleVectors(pPlayer->GetAbsAngles(), NULL, NULL, &up);

		Vector teleportPosition = trace.endpos + up * 10; // +100 * forward;

#if 0
		ConMsg("Pistol Teleport\n");
		ConMsg(" -- Start Position: %f, %f, %f\n", startPosition.x, startPosition.y, startPosition.z);
		ConMsg(" -- End Position: %f, %f, %f\n", endPosition.x, endPosition.y, endPosition.z);
		ConMsg(" -- Trace Result End Position: %f, %f, %f\n", trace.endpos.x, trace.endpos.y, trace.endpos.z);
#endif
		pPlayer->Teleport(&teleportPosition, NULL, NULL);

		//ConMsg(" -- End\n");
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMP5::SecondaryAttack()
{
#if 0
	Vector teleporterTargetAbsPosition = GetAbsOrigin();
	if (m_pTeleporter->GetAbsTargetPosition(&teleporterTargetAbsPosition))
	{
		CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
		pPlayer->Teleport(&teleporterTargetAbsPosition, NULL, NULL);
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flDot -
//          flDist -
// Output : int
//-----------------------------------------------------------------------------
int CWeaponMP5::WeaponRangeAttack2Condition(float flDot, float flDist)
{
	return COND_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const WeaponProficiencyInfo_t *CWeaponMP5::GetProficiencyValues()
{
	static WeaponProficiencyInfo_t proficiencyTable[] =
	{
		{ 7.0, 0.75 },
		{ 5.00, 0.75 },
		{ 10.0 / 3.0, 0.75 },
		{ 5.0 / 3.0, 0.75 },
		{ 1.00, 1.0 },
	};

	COMPILE_TIME_ASSERT(ARRAYSIZE(proficiencyTable) == WEAPON_PROFICIENCY_PERFECT + 1);

	return proficiencyTable;
}

void CWeaponMP5::DEBUG_SpawnMyModelEntity()
{

#if 0
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	//QAngle absEyeAngles = pPlayer->GetAbsAngles();
	QAngle absEyeAngles = pPlayer->EyeAngles();

	Vector forward, right, up;
	AngleVectors(absEyeAngles, &forward, &right, &up);

	Vector endPosition = GetAbsOrigin() + forward * TELEPORT_END_ENTITY_DISTANCE;

	m_pEndEntity = Create("my_model_entity", endPosition, absEyeAngles);

	// IPhysicsEnvironment 
	// virtual IPhysicsSpring	*CreateSpring( IPhysicsObject *pObjectStart, IPhysicsObject *pObjectEnd, springparams_t *pParams ) = 0;
	// See CBarnacleTongueTip::CreateSpring
	// IPhysicsEnvironment  is just a "physenv" global

	springparams_t springParams;
	springParams.constant = TELEPORT_SPRING_CONSTANT;
	springParams.damping = TELEPORT_SPRING_DAMPING;
	springParams.naturalLength = TELEPORT_END_ENTITY_DISTANCE;
	springParams.relativeDamping = 10;
	springParams.startPosition = GetAbsOrigin();
	springParams.endPosition = endPosition;
	springParams.useLocalPositions = false;

	if (m_pSpring)
	{
		//physenv->DestroySpring(m_pSpring);
		UTIL_Remove(m_pSpring);
		m_pSpring = NULL;
	}

	
	IPhysicsObject *pPlayerPhysObject = pPlayer->VPhysicsGetObject();

	m_pPhysSpring = physenv->CreateSpring(pPlayerPhysObject, m_pEndEntity->VPhysicsGetObject(), &springParams);
	
	//IPhysicsObject * startObj = spring->GetStartObject();
	//startObj = NULL;
	//m_pSpring = Create("phys_spring", GetAbsOrigin(), absEyeAngles);

	/*
	IPhysicsObject *pPhys = m_pEndEntity->VPhysicsInitShadow(false, false);
	if (pPhys)
	{
		pPhys->SetMass(500);
	}
	*/
	//m_pSpring = physenv->CreateSpring(m_pEndEntity->VPhysicsGetObject(), VPhysicsGetObject(), &springParams);
	//m_pSpring = physenv->CreateSpring(VPhysicsGetObject(), m_pEndEntity->VPhysicsGetObject(), &springParams);

#endif
}