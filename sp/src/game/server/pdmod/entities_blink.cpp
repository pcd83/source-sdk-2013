#include "cbase.h"
#include "triggers.h"
#include "vphysics_interface.h"
#include "physics_saverestore.h"
#include "vphysics/constraints.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//*************************************************
// My Logical Entity
//*************************************************
class CMyLogicalEntity : public CLogicalEntity
{
public:
	DECLARE_CLASS(CMyLogicalEntity, CLogicalEntity);
	DECLARE_DATADESC();

	CMyLogicalEntity()
		: m_nCounter(0)
	{}

	void InputTick(inputdata_t & inputData);

private:
	int m_nThreshold;
	int m_nCounter;

	COutputEvent m_OnThreshold;
};

LINK_ENTITY_TO_CLASS(my_logical_entity, CMyLogicalEntity);

BEGIN_DATADESC(CMyLogicalEntity)
	DEFINE_FIELD(m_nCounter, FIELD_INTEGER),
	DEFINE_KEYFIELD(m_nThreshold, FIELD_INTEGER, "threshold"),
	DEFINE_INPUTFUNC(FIELD_VOID, "Tick", InputTick),
	DEFINE_OUTPUT(m_OnThreshold, "OnThreshold"),
END_DATADESC()

void CMyLogicalEntity::InputTick(inputdata_t & inputData)
{
	ConMsg("CMyLogicalEntity::InputTick\n");

	m_nCounter++;

	if (m_nCounter >= m_nThreshold)
	{
		m_OnThreshold.FireOutput(inputData.pActivator, this);

		m_nCounter = 0;
	}
}

//*************************************************
// My Model Entity
//*************************************************
#define ENTITY_MODEL "models/gibs/airboat_broken_engine.mdl"

class CMyModelEntity : public CBaseAnimating
{
public:
	DECLARE_CLASS(CMyModelEntity, CBaseAnimating);
	DECLARE_DATADESC();

	CMyModelEntity()
	{
		m_bActive = false;
	}

	void Spawn();
	void Precache();
	void MoveThink();

	void InputToggle(inputdata_t & inputData);

private:
	bool m_bActive;
	float m_flNextChangeTime;
};

LINK_ENTITY_TO_CLASS(my_model_entity, CMyModelEntity);

BEGIN_DATADESC(CMyModelEntity)
DEFINE_FIELD(m_bActive, FIELD_BOOLEAN),
DEFINE_FIELD(m_flNextChangeTime, FIELD_TIME),
DEFINE_INPUTFUNC(FIELD_VOID, "Toggle", InputToggle),
DEFINE_THINKFUNC(MoveThink),
END_DATADESC()

void CMyModelEntity::Precache()
{
	PrecacheModel(ENTITY_MODEL);

	BaseClass::Precache();
}

void CMyModelEntity::Spawn()
{
	Precache();

	SetModel(ENTITY_MODEL);
	SetSolid(SOLID_BBOX);
	UTIL_SetSize(this, -Vector(20, 20, 20), Vector(20, 20, 20));
}

void CMyModelEntity::MoveThink()
{
	if (m_flNextChangeTime < gpGlobals->curtime)
	{
		Vector vecNewVelocity = RandomVector(-64.f, 64.f);
		SetAbsVelocity(vecNewVelocity);

		m_flNextChangeTime = gpGlobals->curtime + random->RandomFloat(1.f, 3.f);
	}

	Vector velFacing = GetAbsVelocity();
	QAngle angFacing;
	VectorAngles(velFacing, angFacing);
	SetAbsAngles(angFacing);

	SetNextThink(gpGlobals->curtime + 0.05f);
}

void CMyModelEntity::InputToggle(inputdata_t & inputData)
{
	if (!m_bActive)
	{
		SetThink(&CMyModelEntity::MoveThink);

		SetNextThink(gpGlobals->curtime + 0.05f);

		SetMoveType(MOVETYPE_FLY);

		m_flNextChangeTime = gpGlobals->curtime;

		m_bActive = true;
	}
	else
	{
		SetThink(NULL);

		SetAbsVelocity(vec3_origin);
		SetMoveType(MOVETYPE_NONE);

		m_bActive = false;
	}
}

//*************************************************
// My Brush Entity
//*************************************************
class CMyBrushEntity : public CBaseTrigger
{
public:
	DECLARE_CLASS(CMyBrushEntity, CBaseTrigger);
	DECLARE_DATADESC();

	void Spawn();
	void BrushTouch(CBaseEntity * pOther);
};

LINK_ENTITY_TO_CLASS(my_brush_entity, CMyBrushEntity);

BEGIN_DATADESC(CMyBrushEntity)
	DEFINE_ENTITYFUNC(BrushTouch),
END_DATADESC()

void CMyBrushEntity::Spawn()
{
	BaseClass::Spawn();

	SetTouch(&CMyBrushEntity::BrushTouch); // we want to capture touches from other entities

	SetSolid(SOLID_VPHYSICS); // we should collide with physics

	SetModel(STRING(GetModelName())); // use our brush model

	SetMoveType(MOVETYPE_PUSH); // push things out of our awy

	VPhysicsInitShadow(false, false); // Create physics hull info
}

void CMyBrushEntity::BrushTouch(CBaseEntity* pOther)
{
	DevMsg("CMyBrushEntity::BrushTouch\n");

	const trace_t &tr = GetTouchTrace();

	Vector vecPushDir = tr.plane.normal;
	vecPushDir.Negate();
	vecPushDir.z = 0;

	// Uncomment this line to print plane information to the console in developer mode
	//DevMsg ( "%s (%s) touch plane's normal: [%f %f]\n", GetClassname(), GetDebugName(),tr.plane.normal.x, tr.plane.normal.y );

	// Move slowly in that direction
	LinearMove(GetAbsOrigin() + (vecPushDir * 64.0f), 32.0f);
}

//*************************************************
// Spring Entity
//*************************************************

//*************************************************
//
// NOTE(pd): See CBarnacleTongueTip!
//
//*************************************************

/*
#define BLINK_SPRING_ENTITY_MODEL_NAME	"models/props_junk/rock001a.mdl"

class CBlinkSpringEntity : public CBaseAnimating
{
public:
	CBlinkSpringEntity();
	DECLARE_CLASS(CBlinkSpringEntity, CBaseAnimating);
	DECLARE_DATADESC();

	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void UpdateOnRemove();
	virtual void VPhysicsUpdate(IPhysicsObject *pPhysics);

	void UpdateSpringPosition(Vector position);
	void SpringThink();

private:
	IPhysicsSpring * m_pPhysSpring;
};

LINK_ENTITY_TO_CLASS(blink_spring, CBlinkSpringEntity);

BEGIN_DATADESC(CBlinkSpringEntity)
DEFINE_PHYSPTR(m_pPhysSpring),
//DEFINE_ENTITYFUNC(BrushTouch),
DEFINE_THINKFUNC(SpringThink),
END_DATADESC()

CBlinkSpringEntity::CBlinkSpringEntity()
: m_pPhysSpring(NULL)
{}

void CBlinkSpringEntity::Spawn()
{
	Precache();
	SetModel(BLINK_SPRING_ENTITY_MODEL_NAME);
	AddEffects(EF_NODRAW);

	// We don't want this to be solid, because we don't want it to collide with the barnacle.
	SetSolid(SOLID_VPHYSICS);
	AddSolidFlags(FSOLID_NOT_SOLID);
	BaseClass::Spawn();

	m_pPhysSpring = NULL;

	SetThink(&CBlinkSpringEntity::SpringThink);
	SetNextThink(gpGlobals->curtime + 0.5f);
}

void CBlinkSpringEntity::Precache()
{
	PrecacheModel(BLINK_SPRING_ENTITY_MODEL_NAME);
	BaseClass::Precache();
}

void CBlinkSpringEntity::UpdateOnRemove()
{
	if (m_pPhysSpring)
	{
		physenv->DestroySpring(m_pPhysSpring);
		m_pPhysSpring = NULL;
	}
	BaseClass::UpdateOnRemove();
}

void CBlinkSpringEntity::VPhysicsUpdate(IPhysicsObject *pPhysics)
{
	BaseClass::VPhysicsUpdate(pPhysics);

}

void CBlinkSpringEntity::UpdateSpringPosition(Vector position)
{
	m_pPhysSpring->SetSpringLength(1000);
}

void CBlinkSpringEntity::SpringThink()
{
	SetNextThink(gpGlobals->curtime + 0.1f);

	if (m_pPhysSpring)
	{
		Vector vec;
		UpdateSpringPosition(vec);
	}
}
*/

class CBlinkTeleporter;

//*************************************************
// Teleport target
//*************************************************
#define BLINK_TELEPORT_TARGET_MODEL_NAME "models/props_junk/rock001a.mdl"

class CBlinkTeleportEndpoint : public CBaseAnimating
{
	DECLARE_CLASS(CBlinkTeleportEndpoint, CBaseAnimating);

public:
	DECLARE_DATADESC();

	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void UpdateOnRemove();
	virtual void VPhysicsUpdate(IPhysicsObject *pPhysics);

	virtual int	UpdateTransmitState(void);

	bool						CreateSpring(CBaseAnimating *pTongueRoot);
	static CBlinkTeleportEndpoint	*CreateTeleportTargetEnd(CBlinkTeleporter *pSpringEntity, CBaseAnimating *pTongueStart, const Vector &vecOrigin, const QAngle &vecAngles);
	static CBlinkTeleportEndpoint	*CreateTeleportTargetBeginning(const Vector &vecOrigin, const QAngle &vecAngles);

	IPhysicsSpring			*m_pSpring;

private:
	CHandle<CBlinkTeleporter>	m_hTeleporter;
};

LINK_ENTITY_TO_CLASS(blink_teleport_endpoint, CBlinkTeleportEndpoint);

BEGIN_DATADESC(CBlinkTeleportEndpoint)

//DEFINE_FIELD(m_hBarnacle, FIELD_EHANDLE),
DEFINE_PHYSPTR(m_pSpring),

END_DATADESC()

//****************************************************************************************
class CBlinkTeleporter : public CBaseAnimating
{
	DECLARE_CLASS(CBlinkTeleporter, CBaseAnimating);

public:
	DECLARE_DATADESC();

	virtual void Activate();
	virtual void Spawn();

	void TeleporterThink();

private:
	void InitRootPosition();
	void CreateStartAndEndEntities();
	void CreateConstraint();

	CHandle<CBlinkTeleportEndpoint>	m_hStartEntity;
	CHandle<CBlinkTeleportEndpoint>	m_hEndEntity;
	IPhysicsConstraint			*m_pConstraint;

	Vector m_vecRoot, m_vecTip;
};

LINK_ENTITY_TO_CLASS(blink_teleporter, CBlinkTeleporter);

BEGIN_DATADESC(CBlinkTeleporter)
	DEFINE_FIELD(m_hStartEntity, FIELD_EHANDLE),
	DEFINE_FIELD(m_hEndEntity, FIELD_EHANDLE),
	DEFINE_PHYSPTR(m_pConstraint),
	DEFINE_FIELD(m_vecRoot, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(m_vecTip, FIELD_POSITION_VECTOR),
	DEFINE_THINKFUNC(TeleporterThink)
END_DATADESC()

void CBlinkTeleporter::Activate()
{
	if (m_hStartEntity)
		return;

	InitRootPosition();

	m_hStartEntity = CBlinkTeleportEndpoint::CreateTeleportTargetBeginning(m_vecRoot, QAngle(90, 0, 0));
	m_hEndEntity = CBlinkTeleportEndpoint::CreateTeleportTargetEnd(this, m_hStartEntity, m_vecTip, QAngle(0, 0, 0));

	CreateStartAndEndEntities();
	CreateConstraint();
}

void CBlinkTeleporter::InitRootPosition()
{
	CBasePlayer * player = UTIL_GetLocalPlayer();
	if (!player)  { return; }

	const float height = 25;
	const float distanceFromPlayer = 100;

	Vector origin = player->GetAbsOrigin();
	//m_vecRoot = origin - Vector(0, 0, flTongueAdj);
	m_vecRoot = origin + Vector(0, distanceFromPlayer, height);
	//m_vecTip = origin + Vector(0, 0, 30); // DEBUG(pd): Just put the ending endpoint above the teleporter entity
	m_vecTip = m_vecRoot + Vector(0,0,height); // DEBUG(pd): Just put the ending endpoint above the teleporter entity
	CollisionProp()->MarkSurroundingBoundsDirty();
}

void CBlinkTeleporter::CreateStartAndEndEntities()
{
	if (m_hStartEntity)
		return;

	m_hStartEntity = CBlinkTeleportEndpoint::CreateTeleportTargetBeginning(m_vecRoot, QAngle(90, 0, 0));
	m_hEndEntity = CBlinkTeleportEndpoint::CreateTeleportTargetEnd(this, m_hStartEntity, m_vecTip, QAngle(0, 0, 0));
	//m_nSpitAttachment = LookupAttachment("StrikeHeadAttach");
	Assert(m_hStartEntity && m_hEndEntity);
}

void CBlinkTeleporter::CreateConstraint()
{
	if (m_pConstraint)
	{
		physenv->DestroyConstraint(m_pConstraint);
		m_pConstraint = NULL;
	}

	// Create the new constraint for the standing/ducking player physics object.
	IPhysicsObject *pPlayerPhys = m_hStartEntity->VPhysicsGetObject();
	IPhysicsObject *pTonguePhys = m_hEndEntity->VPhysicsGetObject();

	constraint_fixedparams_t fixed;
	fixed.Defaults();
	fixed.InitWithCurrentObjectState(pTonguePhys, pPlayerPhys);
	fixed.constraint.Defaults();

	m_pConstraint = physenv->CreateFixedConstraint(pTonguePhys, pPlayerPhys, NULL, fixed);
}

void CBlinkTeleporter::TeleporterThink()
{
	DevMsg("TeleporterThink\n");
	SetNextThink(gpGlobals->curtime + 1);
}

void CBlinkTeleporter::Spawn()
{
	BaseClass::Spawn();
	Precache();

	SetThink(&CBlinkTeleporter::TeleporterThink);
	SetNextThink(gpGlobals->curtime + 0.5f);
}

//*******************************************************************************************



void CBlinkTeleportEndpoint::Spawn()
{
	Precache();
	SetModel(BLINK_TELEPORT_TARGET_MODEL_NAME);
	//AddEffects(EF_NODRAW);

	// We don't want this to be solid, because we don't want it to collide with the barnacle.
	SetSolid(SOLID_VPHYSICS);
	//AddSolidFlags(FSOLID_NOT_SOLID);
	BaseClass::Spawn();

	m_pSpring = NULL;
}

void CBlinkTeleportEndpoint::Precache()
{
	PrecacheModel(BLINK_TELEPORT_TARGET_MODEL_NAME);
	BaseClass::Precache();
}

void CBlinkTeleportEndpoint::UpdateOnRemove()
{
	BaseClass::UpdateOnRemove();
}

void CBlinkTeleportEndpoint::VPhysicsUpdate(IPhysicsObject *pPhysics)
{
	BaseClass::VPhysicsUpdate(pPhysics);
}

int	CBlinkTeleportEndpoint::UpdateTransmitState()
{
	return SetTransmitState(FL_EDICT_PVSCHECK);
}

bool CBlinkTeleportEndpoint::CreateSpring(CBaseAnimating *pTongueRoot)
{
	return true;
}

CBlinkTeleportEndpoint * CBlinkTeleportEndpoint::CreateTeleportTargetEnd(CBlinkTeleporter *pTeleporter, CBaseAnimating *pTeleportStart, const Vector &vecOrigin, const QAngle &vecAngles)
{
	CBlinkTeleportEndpoint *pTeleportTarget = (CBlinkTeleportEndpoint *)CBaseEntity::Create("blink_teleport_endpoint", vecOrigin, vecAngles);
	if (!pTeleportTarget)
		return NULL;

	pTeleportTarget->VPhysicsInitNormal(pTeleportTarget->GetSolid(), pTeleportTarget->GetSolidFlags(), false);
	if (!pTeleportTarget->CreateSpring(pTeleportStart))
		return NULL;

	// Set the backpointer to the barnacle
	pTeleportTarget->m_hTeleporter = pTeleporter;

	// Don't collide with the world
	IPhysicsObject *pTeleportTargetPhys = pTeleportTarget->VPhysicsGetObject();

	// turn off all floating / fluid simulation
	pTeleportTargetPhys->SetCallbackFlags(pTeleportTargetPhys->GetCallbackFlags() & (~CALLBACK_DO_FLUID_SIMULATION));

	DevLog("CreateTeleportTargetEnd worked\n");

	return pTeleportTarget;
}

CBlinkTeleportEndpoint * CBlinkTeleportEndpoint::CreateTeleportTargetBeginning(const Vector &vecOrigin, const QAngle &vecAngles)
{
	CBlinkTeleportEndpoint *pTT = (CBlinkTeleportEndpoint *)CBaseEntity::Create("blink_teleport_endpoint", vecOrigin, vecAngles);
	if (!pTT)
		return NULL;

	pTT->AddSolidFlags(FSOLID_NOT_SOLID);

	// Disable movement on the root, we'll move this thing manually.
	pTT->VPhysicsInitShadow(false, false);
	pTT->SetMoveType(MOVETYPE_NONE);

	DevLog("CreateTeleportTargetBeginning worked\n");

	return pTT;
}

