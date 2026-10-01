#include "ASShotValidation.h"

const TCHAR* LexToString(EASShotVerdict Verdict)
{
	switch (Verdict)
	{
	case EASShotVerdict::Valid:          return TEXT("a valid shot");
	case EASShotVerdict::TooManyBullets: return TEXT("more bullets than the weapon fires");
	case EASShotVerdict::NotARay:        return TEXT("a bullet that isn't a ray");
	case EASShotVerdict::OutsideSpread:  return TEXT("bullets wider apart than the weapon's spread");
	}
	return TEXT("an unknown verdict");
}

EASShotVerdict ASShotValidation::CheckBullets(TArrayView<const FVector> Directions, int32 BulletsPerShot, float SpreadHalfAngleDeg)
{
	if (Directions.Num() > BulletsPerShot)
	{
		return EASShotVerdict::TooManyBullets;
	}

	// Every bullet leaves the same eyes at the same moment, so each one is held to the first one's direction.
	const float MaxPelletAngleDeg = FMath::Min(2.f * SpreadHalfAngleDeg + PelletAngleToleranceDeg, 180.f);
	const FVector::FReal MinPelletDot = FMath::Cos(FMath::DegreesToRadians(MaxPelletAngleDeg));
	for (const FVector& Direction : Directions)
	{
		if (Direction.IsZero())
		{
			return EASShotVerdict::NotARay;
		}
		if ((Direction | Directions[0]) < MinPelletDot)
		{
			return EASShotVerdict::OutsideSpread;
		}
	}
	return EASShotVerdict::Valid;
}