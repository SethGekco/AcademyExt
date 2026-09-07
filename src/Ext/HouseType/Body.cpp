#include "Body.h"

#include <Utilities/Macro.h>
#include <Utilities/Debug.h>

HouseTypeExt::ExtContainer HouseTypeExt::ExtMap;

// ============================================================================
// INI
// ============================================================================

void HouseTypeExt::ExtData::LoadFromINIFile(CCINIClass* pINI)
{
	auto pID = this->OwnerObject()->ID;

	INI_EX exINI(pINI);

	this->AcademyBonus.Read(exINI, pID, "AcademyBonus");
	this->AcademyBonusInfantry.Read(exINI, pID, "AcademyBonus.Infantry");
	this->AcademyBonusVehicle.Read(exINI, pID, "AcademyBonus.Vehicle");
	this->AcademyBonusAircraft.Read(exINI, pID, "AcademyBonus.Aircraft");
	this->AcademyBonusBuilding.Read(exINI, pID, "AcademyBonus.Building");
	this->AcademyBonusStacks.Read(exINI, pID, "AcademyBonus.Stacks");
	this->AcademyBonusCap.Read(exINI, pID, "AcademyBonus.Cap");
	this->AcademyBonusWhitelist.Read(exINI, pID, "AcademyBonus.Types");
	this->AcademyBonusBlacklist.Read(exINI, pID, "AcademyBonus.Ignore");

	// NOTE: these are deliberately NOT inherited from ParentCountry=. Antares
	// inherits only a hand-picked set of country tags (VeteranBuildings among
	// them) and silently inheriting a stacking bonus would be surprising --
	// a child country would gain a bonus its INI never mentions. Revisit if
	// real mods want it; see DESIGN.md section 9.
}

// ============================================================================
// Queries
// ============================================================================

bool HouseTypeExt::ExtData::HasBonus(AcademyCategory category) const
{
	switch (category)
	{
	case AcademyCategory::Infantry: if (this->AcademyBonusInfantry.isset()) return true; break;
	case AcademyCategory::Vehicle:  if (this->AcademyBonusVehicle.isset())  return true; break;
	case AcademyCategory::Aircraft: if (this->AcademyBonusAircraft.isset()) return true; break;
	case AcademyCategory::Building: if (this->AcademyBonusBuilding.isset()) return true; break;
	default: return false;
	}

	return this->AcademyBonus.isset();
}

double HouseTypeExt::ExtData::GetBonus(AcademyCategory category) const
{
	switch (category)
	{
	case AcademyCategory::Infantry:
		if (this->AcademyBonusInfantry.isset()) return this->AcademyBonusInfantry.Get(0.0);
		break;
	case AcademyCategory::Vehicle:
		if (this->AcademyBonusVehicle.isset()) return this->AcademyBonusVehicle.Get(0.0);
		break;
	case AcademyCategory::Aircraft:
		if (this->AcademyBonusAircraft.isset()) return this->AcademyBonusAircraft.Get(0.0);
		break;
	case AcademyCategory::Building:
		if (this->AcademyBonusBuilding.isset()) return this->AcademyBonusBuilding.Get(0.0);
		break;
	default:
		return 0.0;
	}

	return this->AcademyBonus.Get(0.0);
}

bool HouseTypeExt::ExtData::AppliesTo(TechnoTypeClass* pType) const
{
	bool const isWhitelisted = this->AcademyBonusWhitelist.empty()
		|| this->AcademyBonusWhitelist.Contains(pType);

	return isWhitelisted && !this->AcademyBonusBlacklist.Contains(pType);
}

// ============================================================================
// Serialization
// ============================================================================

template <typename T>
void HouseTypeExt::ExtData::Serialize(T& Stm)
{
	Stm
		.Process(this->AcademyBonus)
		.Process(this->AcademyBonusInfantry)
		.Process(this->AcademyBonusVehicle)
		.Process(this->AcademyBonusAircraft)
		.Process(this->AcademyBonusBuilding)
		.Process(this->AcademyBonusStacks)
		.Process(this->AcademyBonusCap)
		.Process(this->AcademyBonusWhitelist)
		.Process(this->AcademyBonusBlacklist)
		;
}

void HouseTypeExt::ExtData::LoadFromStream(PhobosStreamReader& Stm)
{
	Extension<HouseTypeClass>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void HouseTypeExt::ExtData::SaveToStream(PhobosStreamWriter& Stm)
{
	Extension<HouseTypeClass>::SaveToStream(Stm);
	this->Serialize(Stm);
}

// ============================================================================
// Container
// ============================================================================

HouseTypeExt::ExtContainer::ExtContainer()
	: Container("HouseTypeClass")
{ }

HouseTypeExt::ExtContainer::~ExtContainer() = default;

// Container lifecycle. Addresses from Phobos (develop, pinned 47475624).
// All handlers return 0 so Syringe chains rather than cutting.

DEFINE_HOOK(0x511635, HouseTypeClass_CTOR_1_AcademyExt, 0x5)
{
	GET(HouseTypeClass*, pItem, EAX);
	HouseTypeExt::ExtMap.TryAllocate(pItem);
	return 0;
}

DEFINE_HOOK(0x511643, HouseTypeClass_CTOR_2_AcademyExt, 0x5)
{
	GET(HouseTypeClass*, pItem, EAX);
	HouseTypeExt::ExtMap.TryAllocate(pItem);
	return 0;
}

DEFINE_HOOK(0x5127CF, HouseTypeClass_DTOR_AcademyExt, 0x6)
{
	GET(HouseTypeClass*, pItem, ESI);
	HouseTypeExt::ExtMap.Remove(pItem);
	return 0;
}

DEFINE_HOOK_AGAIN(0x512480, HouseTypeClass_SaveLoad_Prefix_AcademyExt, 0x5)
DEFINE_HOOK(0x512290, HouseTypeClass_SaveLoad_Prefix_AcademyExt, 0x5)
{
	GET_STACK(HouseTypeClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);
	HouseTypeExt::ExtMap.PrepareStream(pItem, pStm);
	return 0;
}

DEFINE_HOOK(0x51246D, HouseTypeClass_Load_Suffix_AcademyExt, 0x5)
{
	HouseTypeExt::ExtMap.LoadStatic();
	return 0;
}

DEFINE_HOOK(0x51255C, HouseTypeClass_Save_Suffix_AcademyExt, 0x5)
{
	HouseTypeExt::ExtMap.SaveStatic();
	return 0;
}

DEFINE_HOOK_AGAIN(0x51215A, HouseTypeClass_LoadFromINI_AcademyExt, 0x5)
DEFINE_HOOK(0x51214F, HouseTypeClass_LoadFromINI_AcademyExt, 0x5)
{
	GET(HouseTypeClass*, pItem, EBX);
	GET_BASE(CCINIClass*, pINI, 0x8);
	HouseTypeExt::ExtMap.LoadFromINI(pItem, pINI);
	return 0;
}
