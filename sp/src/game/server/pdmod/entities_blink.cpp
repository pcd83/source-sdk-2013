#include "cbase.h"
#include "triggers.h"

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
// My Model Entity
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