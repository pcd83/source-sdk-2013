void TraceFromPlayerAimInfinitely(trace_t & trace);

class IBlinkTeleporter
{
public:
	virtual ~IBlinkTeleporter() {}
	virtual bool GetAbsTargetPosition(Vector * absPos) = 0;
	virtual bool IsAbsTargetPositionValid() const = 0;
};