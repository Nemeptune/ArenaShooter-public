#pragma once
#include "CoreMinimal.h"

enum class EASShotVerdict : uint8
{
	Valid,
	TooManyBullets,
	NotARay,
	OutsideSpread
};

ARENASHOOTER_API const TCHAR* LexToString(EASShotVerdict Verdict);

namespace ASShotValidation
{
	/** Covers the rounding of a ray's ends on the wire, which bends short rays most. */
	constexpr float PelletAngleToleranceDeg = 1.f;

	/**
	 * Checks the bullets of one client shot against the weapon that fired it: no more than it fires per shot, each
	 * a real direction, and none further from the first than twice the spread half-angle allows. Directions are
	 * unit length, or zero where the client sent no usable ray.
	 */
	ARENASHOOTER_API EASShotVerdict CheckBullets(TArrayView<const FVector> Directions, int32 BulletsPerShot, float SpreadHalfAngleDeg);
}