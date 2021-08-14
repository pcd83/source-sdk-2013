class IBlinkTeleporter
{
public:
	virtual ~IBlinkTeleporter() {}
	virtual bool GetAbsTargetPosition(Vector * absPos) = 0;
};