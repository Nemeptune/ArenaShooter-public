#include "System/ASProfiling.h"

CSV_DEFINE_CATEGORY(ArenaShooter, true);

void ASProfiling::AddToGauge(FName Stat, int32 Delta)
{
#if CSV_PROFILER
	if (TCsvPersistentCustomStat<int32>* Gauge = FCsvProfiler::Get()->GetOrCreatePersistentCustomStatInt(Stat, CSV_CATEGORY_INDEX(ArenaShooter)))
	{
		Gauge->Add(Delta);
	}
#endif
}