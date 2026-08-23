#include "Body.h"

#include <Utilities/Macro.h>
#include <Utilities/Debug.h>

BuildingTypeExt::ExtContainer BuildingTypeExt::ExtMap;

// ============================================================================
// INI
// ============================================================================

void BuildingTypeExt::ExtData::LoadFromINIFile(CCINIClass* pINI)
{
	auto pID = this->OwnerObject()->ID;

	INI_EX exINI(pINI);

	// Antares' existing academy tags, read independently. Note Read() returns
	// void -- presence is tested with .isset(), never with the return value.
	this->AcademyInfantry.Read(exINI, pID, "Academy.InfantryVeterancy");
	this->AcademyAircraft.Read(exINI, pID, "Academy.AircraftVeterancy");
	this->AcademyVehicle.Read(exINI, pID, "Academy.VehicleVeterancy");
	this->AcademyBuilding.Read(exINI, pID, "Academy.BuildingVeterancy");
	this->AcademyWhitelist.Read(exINI, pID, "Academy.Types");
	this->AcademyBlacklist.Read(exINI, pID, "Academy.Ignore");

	// AcademyExt's one new academy tag.
	this->AcademyStacks.Read(exINI, pID, "Academy.Stacks");

	// Spy magnitudes (Phase 5 -- parsed now so the savegame format is stable
	// from the first release; nothing populates InfiltratedSources until the
	// observer hook at 0x4571E0 is wired).
	this->SpyInfantryLevel.Read(exINI, pID, "SpyEffect.InfantryVeterancy.Level");
	this->SpyVehicleLevel.Read(exINI, pID, "SpyEffect.VehicleVeterancy.Level");
	this->SpyNavalLevel.Read(exINI, pID, "SpyEffect.NavalVeterancy.Level");
	this->SpyAircraftLevel.Read(exINI, pID, "SpyEffect.AircraftVeterancy.Level");
	this->SpyBuildingLevel.Read(exINI, pID, "SpyEffect.BuildingVeterancy.Level");
	this->SpyStacks.Read(exINI, pID, "SpyEffect.Veterancy.Stacks");
	this->SpyWhitelist.Read(exINI, pID, "SpyEffect.Veterancy.Types");
	this->SpyBlacklist.Read(exINI, pID, "SpyEffect.Veterancy.Ignore");
}

// ============================================================================
// Queries
// ============================================================================

bool BuildingTypeExt::ExtData::IsAcademy() const
{
	return this->AcademyInfantry > 0.0
		|| this->AcademyAircraft > 0.0
		|| this->AcademyVehicle > 0.0
		|| this->AcademyBuilding > 0.0;
}

double BuildingTypeExt::ExtData::GetAcademyValue(AcademyCategory category) const
{
	switch (category)
	{
	case AcademyCategory::Infantry: return this->AcademyInfantry.Get();
	case AcademyCategory::Aircraft: return this->AcademyAircraft.Get();
	case AcademyCategory::Vehicle:  return this->AcademyVehicle.Get();
	case AcademyCategory::Building: return this->AcademyBuilding.Get();
	default:                        return 0.0;
	}
}

bool BuildingTypeExt::ExtData::HasSpyLevel(SpyBranch branch) const
{
	switch (branch)
	{
	case SpyBranch::Infantry: return this->SpyInfantryLevel.isset();
	case SpyBranch::Vehicle:  return this->SpyVehicleLevel.isset();
	case SpyBranch::Naval:    return this->SpyNavalLevel.isset();
	case SpyBranch::Aircraft: return this->SpyAircraftLevel.isset();
	case SpyBranch::Building: return this->SpyBuildingLevel.isset();
	default:                  return false;
	}
}

double BuildingTypeExt::ExtData::GetSpyLevel(SpyBranch branch) const
{
	switch (branch)
	{
	case SpyBranch::Infantry: return this->SpyInfantryLevel.Get(0.0);
	case SpyBranch::Vehicle:  return this->SpyVehicleLevel.Get(0.0);
	case SpyBranch::Naval:    return this->SpyNavalLevel.Get(0.0);
	case SpyBranch::Aircraft: return this->SpyAircraftLevel.Get(0.0);
	case SpyBranch::Building: return this->SpyBuildingLevel.Get(0.0);
	default:                  return 0.0;
	}
}

// Empty whitelist means "all types"; the blacklist always wins. Mirrors
// Antares' HouseExt::ApplyAcademy filter exactly.
static bool PassesFilter(
	ValueableVector<TechnoTypeClass*> const& whitelist,
	ValueableVector<TechnoTypeClass*> const& blacklist,
	TechnoTypeClass* pType)
{
	bool const isWhitelisted = whitelist.empty() || whitelist.Contains(pType);
	return isWhitelisted && !blacklist.Contains(pType);
}

bool BuildingTypeExt::ExtData::AcademyAppliesTo(TechnoTypeClass* pType) const
{
	return PassesFilter(this->AcademyWhitelist, this->AcademyBlacklist, pType);
}

bool BuildingTypeExt::ExtData::SpyAppliesTo(TechnoTypeClass* pType) const
{
	return PassesFilter(this->SpyWhitelist, this->SpyBlacklist, pType);
}

// ============================================================================
// Serialization
// ============================================================================

template <typename T>
void BuildingTypeExt::ExtData::Serialize(T& Stm)
{
	Stm
		.Process(this->AcademyInfantry)
		.Process(this->AcademyAircraft)
		.Process(this->AcademyVehicle)
		.Process(this->AcademyBuilding)
		.Process(this->AcademyWhitelist)
		.Process(this->AcademyBlacklist)
		.Process(this->AcademyStacks)
		.Process(this->SpyInfantryLevel)
		.Process(this->SpyVehicleLevel)
		.Process(this->SpyNavalLevel)
		.Process(this->SpyAircraftLevel)
		.Process(this->SpyBuildingLevel)
		.Process(this->SpyStacks)
		.Process(this->SpyWhitelist)
		.Process(this->SpyBlacklist)
		;
}

void BuildingTypeExt::ExtData::LoadFromStream(PhobosStreamReader& Stm)
{
	Extension<BuildingTypeClass>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void BuildingTypeExt::ExtData::SaveToStream(PhobosStreamWriter& Stm)
{
	Extension<BuildingTypeClass>::SaveToStream(Stm);
	this->Serialize(Stm);
}

// ============================================================================
// Container
// ============================================================================

BuildingTypeExt::ExtContainer::ExtContainer()
	: Container("BuildingTypeClass")
{ }

BuildingTypeExt::ExtContainer::~ExtContainer() = default;

// Container lifecycle. Addresses taken from Phobos (develop, pinned commit
// 47475624). All handlers return 0 so Syringe chains us with every other
// consumer instead of cutting the chain.

DEFINE_HOOK(0x45E50C, BuildingTypeClass_CTOR_AcademyExt, 0x6)
{
	GET(BuildingTypeClass*, pItem, EAX);
	BuildingTypeExt::ExtMap.TryAllocate(pItem);
	return 0;
}

DEFINE_HOOK(0x45E707, BuildingTypeClass_DTOR_AcademyExt, 0x6)
{
	GET(BuildingTypeClass*, pItem, ESI);
	BuildingTypeExt::ExtMap.Remove(pItem);
	return 0;
}

DEFINE_HOOK_AGAIN(0x465300, BuildingTypeClass_SaveLoad_Prefix_AcademyExt, 0x5)
DEFINE_HOOK(0x465010, BuildingTypeClass_SaveLoad_Prefix_AcademyExt, 0x5)
{
	GET_STACK(BuildingTypeClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);
	BuildingTypeExt::ExtMap.PrepareStream(pItem, pStm);
	return 0;
}

DEFINE_HOOK(0x4652ED, BuildingTypeClass_Load_Suffix_AcademyExt, 0x7)
{
	BuildingTypeExt::ExtMap.LoadStatic();
	return 0;
}

DEFINE_HOOK(0x46536A, BuildingTypeClass_Save_Suffix_AcademyExt, 0x7)
{
	BuildingTypeExt::ExtMap.SaveStatic();
	return 0;
}

DEFINE_HOOK(0x464A49, BuildingTypeClass_LoadFromINI_AcademyExt, 0xA)
{
	GET(BuildingTypeClass*, pItem, EBP);
	GET_STACK(CCINIClass*, pINI, 0x364);
	BuildingTypeExt::ExtMap.LoadFromINI(pItem, pINI);
	return 0;
}
