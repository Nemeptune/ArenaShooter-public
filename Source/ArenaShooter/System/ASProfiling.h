#pragma once

#include "CoreMinimal.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "ProfilingDebugging/MiscTrace.h"

/*
 * One include for ArenaShooter's profiling markers.
 *   TRACE_CPUPROFILER_EVENT_SCOPE(Name)        named scope in Unreal Insights
 *   CSV_SCOPED_TIMING_STAT_EXCLUSIVE(AS_Name)  per-frame game-thread time in CSV captures
 *   CSV_CUSTOM_STAT(ArenaShooter, Name, ...)   per-frame counter in CSV captures
 *   CSV_EVENT(ArenaShooter, Text)              CSV event, also written as an Insights bookmark
 */
CSV_DECLARE_CATEGORY_EXTERN(ArenaShooter);

namespace ASProfiling
{
	/** Adds Delta to a CSV counter that keeps its value between frames, such as the number of live projectiles. */
	void AddToGauge(FName Stat, int32 Delta);
}