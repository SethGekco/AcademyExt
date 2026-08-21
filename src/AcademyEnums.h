#pragma once

// Academy categories.
//
// Mirrors Antares' `considerAs` dispatch exactly -- diverging here would make
// AcademyExt disagree with Antares about which bonus applies to a given object:
//   Organic=yes vehicles           -> Infantry
//   ConsideredAircraft=yes vehicles -> Aircraft
//   everything else                 -> its own abstract type
enum class AcademyCategory : int
{
	Infantry = 0,
	Vehicle,
	Aircraft,
	Building,

	Count
};

// Spy / infiltration branches.
//
// Naval is a spy-ONLY sub-branch with no academy counterpart: naval units are
// UnitTypes with Naval=yes, so they resolve as AcademyCategory::Vehicle for
// academy purposes while still being able to carry a distinct infiltration
// contribution. This mirrors Antares, whose SpyEffect.NavalVeterancy covers
// exactly the vehicles the stock WarFactoryInfiltrated flag does not.
enum class SpyBranch : int
{
	Infantry = 0,
	Vehicle,
	Naval,
	Aircraft,
	Building,

	Count
};

static constexpr int AcademyCategoryCount = static_cast<int>(AcademyCategory::Count);
static constexpr int SpyBranchCount = static_cast<int>(SpyBranch::Count);
