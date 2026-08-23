// Academy hooks.
//
// ENCYCLOPEDIA-CHECKED (registry/hooks.csv, 2026-08): all nine addresses below
// list only Ares and Antares as consumers. Those two are mutually exclusive and
// never co-loaded, so their shared rows are inherited code rather than a real
// conflict -- AcademyExt is the sole third consumer. Phobos, Kratos and
// CnCNet-Spawner touch none of them.
//
// CHAIN SAFETY: every one of Antares' handlers at these addresses returns 0, so
// Syringe runs ours as well regardless of load order. Ours must do the same --
// returning a jump target here would cut Antares out of the chain and silently
// break its academy handling. Never return anything but 0 from this file.
//
// Both DLLs therefore compute an academy bonus for the same object. That is
// intentional and safe: our result is always >= Antares' plain max(), and both
// apply raise-only, so ours wins whatever the order. DESIGN.md section 4 has
// the proof and the two invariants it depends on.
//
// Registers below are taken from Antares' own handlers
// (src/Ext/House/Hooks.Academy.cpp), not guessed.

#include <InfantryClass.h>
#include <UnitClass.h>
#include <AircraftClass.h>
#include <BuildingClass.h>
#include <BuildingTypeClass.h>
#include <HouseClass.h>

#include <Utilities/Macro.h>

#include <AcademyEnums.h>
#include <Ext/BuildingType/Body.h>
#include <Ext/House/Body.h>

// ============================================================================
// Academy list maintenance
// ============================================================================
//
// These four keep HouseExt::Academies in step with what the house actually owns
// and has on the map. The gating mirrors Antares exactly, including the
// asymmetry: the ADD paths do not test IsOnMap (the building is being placed or
// has just changed hands, so it is on the map by construction) while the REMOVE
// paths do (a building that was never placed was never in the list).

static void UpdateAcademyFor(BuildingClass* pThis, bool added)
{
	if (!pThis || !pThis->Type)
		return;

	auto const pExt = BuildingTypeExt::ExtMap.Find(pThis->Type);

	if (!pExt || !pExt->IsAcademy())
		return;

	if (auto const pHouseExt = HouseExt::ExtMap.Find(pThis->Owner))
		pHouseExt->UpdateAcademy(pThis, added);
}

DEFINE_HOOK(0x446366, BuildingClass_Place_AcademyExt, 0x6)
{
	GET(BuildingClass*, pThis, EBP);

	UpdateAcademyFor(pThis, true);

	return 0;
}

DEFINE_HOOK(0x445905, BuildingClass_Remove_AcademyExt, 0x6)
{
	GET(BuildingClass*, pThis, ESI);

	if (pThis && pThis->IsOnMap)
		UpdateAcademyFor(pThis, false);

	return 0;
}

DEFINE_HOOK(0x448AB2, BuildingClass_ChangeOwnership_Remove_AcademyExt, 0x6)
{
	GET(BuildingClass*, pThis, ESI);

	// Paired with the add at 0x4491D5 further down the same function: this site
	// is expected to see the OLD owner and that one the NEW owner. We mirror
	// Antares' handling rather than having confirmed the ordering ourselves --
	// if it were wrong, Antares' academies would misbehave identically, so this
	// is at worst bug-compatible and at best correct.
	if (pThis && pThis->IsOnMap)
		UpdateAcademyFor(pThis, false);

	return 0;
}

DEFINE_HOOK(0x4491D5, BuildingClass_ChangeOwnership_Add_AcademyExt, 0x6)
{
	GET(BuildingClass*, pThis, ESI);

	UpdateAcademyFor(pThis, true);

	return 0;
}

// ============================================================================
// Applying the bonus
// ============================================================================
//
// One hook per object-creation site. Each resolves the owning house's
// contributions for the right category and raises the new object's veterancy.
//
// NOTE we deliberately do NOT replicate the infiltration SetVeteran() calls
// Antares makes in its versions of these handlers. Antares still runs and still
// performs them; duplicating that here would be redundant at best. The
// configurable SpyEffect.*.Level form is Phase 5 and flows through
// ApplyAcademy's own spy contributions instead.

static void ApplyAcademyTo(TechnoClass* pThis, AcademyCategory category)
{
	if (!pThis)
		return;

	if (auto const pHouseExt = HouseExt::ExtMap.Find(pThis->Owner))
		pHouseExt->ApplyAcademy(pThis, category);
}

DEFINE_HOOK(0x517D51, InfantryClass_Init_AcademyExt, 0x6)
{
	GET(InfantryClass*, pThis, ESI);

	ApplyAcademyTo(pThis, AcademyCategory::Infantry);

	return 0;
}

// Two sites: 0x735678 is the copy inlined into the constructor, 0x74689B is
// UnitClass::Init proper. Both must be covered or units created down one path
// silently miss their bonus.
DEFINE_HOOK_AGAIN(0x735678, UnitClass_Init_AcademyExt, 0x6)
DEFINE_HOOK(0x74689B, UnitClass_Init_AcademyExt, 0x6)
{
	GET(UnitClass*, pThis, ESI);

	if (!pThis || !pThis->Type)
		return 0;

	// Category dispatch mirrors Antares: a VehicleType is treated as whatever
	// it behaves like, not merely what class it belongs to.
	auto category = AcademyCategory::Vehicle;

	if (pThis->Type->ConsideredAircraft)
		category = AcademyCategory::Aircraft;
	else if (pThis->Type->Organic)
		category = AcademyCategory::Infantry;

	ApplyAcademyTo(pThis, category);

	return 0;
}

DEFINE_HOOK(0x413FD2, AircraftClass_Init_AcademyExt, 0x6)
{
	GET(AircraftClass*, pThis, ESI);

	ApplyAcademyTo(pThis, AcademyCategory::Aircraft);

	return 0;
}

DEFINE_HOOK(0x442D1B, BuildingClass_Init_AcademyExt, 0x6)
{
	GET(BuildingClass*, pThis, ESI);

	ApplyAcademyTo(pThis, AcademyCategory::Building);

	return 0;
}
