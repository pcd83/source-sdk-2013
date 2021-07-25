#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

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
	m_nCounter++;

	if (m_nCounter >= m_nThreshold)
	{
		m_OnThreshold.FireOutput(inputData.pActivator, this);

		m_nCounter = 0;
	}
}