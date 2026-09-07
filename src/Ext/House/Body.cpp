#include "Body.h"

#include <algorithm>

#include <TechnoClass.h>
#include <TechnoTypeClass.h>
#include <BuildingClass.h>
#include <BuildingTypeClass.h>
#include <RulesClass.h>
#include <Unsorted.h>

#include <Utilities/Macro.h>
#include <Utilities/Debug.h>

#include <AcademyExt.h>
#include <Ext/BuildingType/Body.h>
#include <Ext/HouseType/Body.h>

HouseExt::ExtContainer HouseExt::ExtMap;

// ============================================================================
// Academy list
// ============================================================================

void HouseExt::ExtData::UpdateAcademy(BuildingClass* pAcademy, bool added)
{
	auto it = std::find(this->Academies.cbegin(), this->Academies.cend(), pAcademy);

	// Already in the desired state -- adding a known academy or removing an
	// unknown one is a no-op.
	if (added == (it != this->Academies.cend()))
		return;

	if (added)
		this->Academies.push_back(pAcademy);
	else
		this->Academies.erase(it);
}

void HouseExt::ExtData::RecordInfiltration(BuildingTypeClass* pBuildingType)
{
	auto const pExt = BuildingTypeExt::ExtMap.Find(pBuildingType);
	if (!pExt)
		return;

	for (int i = 0; i < SpyBranchCount; ++i)
	{
		auto const branch = static_cast<SpyBranch>(i);

		if (!pExt->HasSpyLevel(branch))
			continue;

		auto& list = this->InfiltratedSources[i];

		// Idempotent: infiltrating the same building type twice must not stack
		// with itself. Vanilla infiltration is a permanent per-house flag, and
		// a stacking level would otherwise grow without bound.
		if (std::find(list.cbegin(), list.cend(), pBuildingType) == list.cend())
			list.push_back(pBuildingType);
	}
}

// ============================================================================
// Resolution
// ============================================================================

void HouseExt::ExtData::AddSpyContributions(
	VeterancyResolver& resolver,
	TechnoTypeClass* pType,
	AcademyCategory category) const
{
	auto addBranch = [&](SpyBranch branch)
	{
		for (auto const& pBuildingType : this->InfiltratedSources[static_cast<int>(branch)])
		{
			auto const pExt = BuildingTypeExt::ExtMap.Find(pBuildingType);

			if (!pExt || !pExt->HasSpyLevel(branch) || !pExt->SpyAppliesTo(pType))
				continue;

			resolver.Add(pExt->GetSpyLevel(branch), pExt->SpyStacks);
		}
	};

	switch (category)
	{
	case AcademyCategory::Infantry:
		addBranch(SpyBranch::Infantry);
		break;

	case AcademyCategory::Aircraft:
		addBranch(SpyBranch::Aircraft);
		break;

	case AcademyCategory::Building:
		addBranch(SpyBranch::Building);
		break;

	case AcademyCategory::Vehicle:
		addBranch(SpyBranch::Vehicle);

		// Naval is a spy-only sub-branch layered on top of Vehicle: naval units
		// are UnitTypes, so they take the Vehicle academy bonus, but they can
		// carry an additional infiltration contribution of their own.
		if (pType->Naval)
			addBranch(SpyBranch::Naval);
		break;

	default:
		break;
	}
}

void HouseExt::ExtData::ApplyAcademy(
	TechnoClass* const pTechno, AcademyCategory const category) const
{
	// Mirrors Antares exactly. Without this, preplaced map objects would pick
	// up bonuses Antares would never grant, and in-game "conversions" such as
	// deploy would re-apply the bonus on every transform.
	if (Unsorted::ScenarioInit)
		return;

	auto const pType = pTechno->GetTechnoType();

	if (!pType || !pType->Trainable)
		return;

	VeterancyResolver resolver;

	// Ceilings are collected alongside contributions. The MOST RESTRICTIVE
	// (lowest) cap among everything that applies wins -- a cap is a promise
	// about a maximum, so two caps must not be able to raise each other.
	bool hasCap = false;
	double cap = 0.0;

	auto const considerCap = [&hasCap, &cap](Nullable<double> const& candidate)
	{
		if (!candidate.isset())
			return;

		double const value = candidate.Get(0.0);
		cap = hasCap ? std::min(cap, value) : value;
		hasCap = true;
	};

	// 1. Academy buildings this house owns.
	for (auto const& pBuilding : this->Academies)
	{
		if (!pBuilding)
			continue;

		auto const pExt = BuildingTypeExt::ExtMap.Find(pBuilding->Type);

		if (!pExt || !pExt->AcademyAppliesTo(pType))
			continue;

		double const value = pExt->GetAcademyValue(category);

		if (value != 0.0)
			resolver.Add(value, pExt->AcademyStacks);

		considerCap(pExt->AcademyCap);
	}

	// 2. Passive country bonus.
	if (auto const pOwner = this->OwnerObject())
	{
		if (auto const pCountryExt = HouseTypeExt::ExtMap.Find(pOwner->Type))
		{
			if (pCountryExt->AppliesTo(pType))
			{
				if (pCountryExt->HasBonus(category))
					resolver.Add(pCountryExt->GetBonus(category), pCountryExt->AcademyBonusStacks);

				considerCap(pCountryExt->AcademyBonusCap);
			}
		}
	}

	// 3. Spy / infiltration, recorded by the observer at 0x4571E0.
	this->AddSpyContributions(resolver, pType, category);

	double const veterancyCap = RulesClass::Instance->VeteranCap;
	auto& current = pTechno->Veterancy.Veterancy;

	// ---- default: raise-only ------------------------------------------------
	// Commutative with Antares, so the outcome does not depend on Syringe load
	// order. A cap cannot bite here -- lowering is impossible by construction --
	// which is exactly why authoritative mode exists.
	if (!AcademyExtDLL::Authoritative)
	{
		if (hasCap)
		{
			// Silent no-ops are the worst failure mode in this codebase, so say
			// it once rather than let someone hunt a cap that never applies.
			static bool warned = false;
			if (!warned)
			{
				warned = true;
				Debug::Log("[AcademyExt] WARNING: an Academy.Cap / AcademyBonus.Cap is "
					"configured, but [General] AcademyExt.Authoritative is not set. "
					"Caps lower veterancy and raise-only mode cannot lower anything, "
					"so every cap is being ignored.\n");
			}
		}

		if (resolver.Empty())
			return;

		double const result = resolver.Resolve(veterancyCap);

		if (result > current)
			current = static_cast<float>(result);

		return;
	}

	// ---- authoritative: we are the final writer -----------------------------
	// Only when we actually have something to say. Writing unconditionally with
	// no contribution and no cap would stamp 0.0 over every rank the object got
	// from anywhere else -- spy effects, VeteranBuildings, country VeteranX.
	if (resolver.Empty() && !hasCap)
		return;

	// Start from whatever the object would otherwise have ended up with. We run
	// after Antares in the Syringe chain, so `current` already carries its
	// academy, VeteranBuildings and infiltration promotions -- taking the max
	// keeps all of those intact, and the cap below is what deliberately lowers.
	double result = std::max(resolver.Resolve(veterancyCap), static_cast<double>(current));

	if (hasCap)
		result = std::min(result, cap);

	current = static_cast<float>(std::clamp(result, 0.0, veterancyCap));
}

// ============================================================================
// Pointer invalidation
// ============================================================================

void HouseExt::ExtData::InvalidatePointer(void* ptr, bool bRemoved)
{
	// Academies holds raw BuildingClass*. The place/remove/ownership hooks keep
	// it accurate in the normal cases, but a building destroyed by any path
	// those hooks miss would leave a dangling pointer that ApplyAcademy would
	// dereference on the next unit built.
	auto& list = this->Academies;

	list.erase(
		std::remove(list.begin(), list.end(), ptr),
		list.end());

	// InfiltratedSources holds BuildingTypeClass*, which live for the whole
	// scenario, so they are not invalidated here.
}

// ============================================================================
// Serialization
// ============================================================================

template <typename T>
void HouseExt::ExtData::Serialize(T& Stm)
{
	Stm.Process(this->Academies);

	for (int i = 0; i < SpyBranchCount; ++i)
		Stm.Process(this->InfiltratedSources[i]);
}

void HouseExt::ExtData::LoadFromStream(PhobosStreamReader& Stm)
{
	Extension<HouseClass>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void HouseExt::ExtData::SaveToStream(PhobosStreamWriter& Stm)
{
	Extension<HouseClass>::SaveToStream(Stm);
	this->Serialize(Stm);
}

// ============================================================================
// Container
// ============================================================================

HouseExt::ExtContainer::ExtContainer()
	: Container("HouseClass")
{ }

HouseExt::ExtContainer::~ExtContainer() = default;

// Container lifecycle. Addresses from Phobos (develop, pinned 47475624).
// All handlers return 0 so Syringe chains rather than cutting.

DEFINE_HOOK(0x4F6532, HouseClass_CTOR_AcademyExt, 0x5)
{
	GET(HouseClass*, pItem, EAX);
	HouseExt::ExtMap.TryAllocate(pItem);
	return 0;
}

DEFINE_HOOK(0x4F7371, HouseClass_DTOR_AcademyExt, 0x6)
{
	GET(HouseClass*, pItem, ESI);
	HouseExt::ExtMap.Remove(pItem);
	return 0;
}

DEFINE_HOOK_AGAIN(0x504080, HouseClass_SaveLoad_Prefix_AcademyExt, 0x5)
DEFINE_HOOK(0x503040, HouseClass_SaveLoad_Prefix_AcademyExt, 0x5)
{
	GET_STACK(HouseClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);
	HouseExt::ExtMap.PrepareStream(pItem, pStm);
	return 0;
}

DEFINE_HOOK(0x504069, HouseClass_Load_Suffix_AcademyExt, 0x7)
{
	HouseExt::ExtMap.LoadStatic();
	return 0;
}

DEFINE_HOOK(0x5046DE, HouseClass_Save_Suffix_AcademyExt, 0x7)
{
	HouseExt::ExtMap.SaveStatic();
	return 0;
}

// Global pointer-invalidation broadcast. Established multi-consumer chain point
// (Phobos, SquadExt and others all hook it and return 0).
DEFINE_HOOK(0x7258D0, AcademyExt_AnnounceInvalidPointer, 0x6)
{
	GET(void* const, pInvalid, ECX);
	GET(bool const, removed, EDX);

	HouseExt::ExtMap.PointerGotInvalid(pInvalid, removed);

	return 0;
}
