// Infiltration observer — records stolen veterancy sources.
//
// ENCYCLOPEDIA-CHECKED (registry/hooks.csv + encyclopedia/Spy-Infiltration.md):
// 0x4571E0 is BuildingClass::Infiltrate. Antares and Ares both wrap the WHOLE
// function here and return the jump target 0x4575A2 when their own dispatch
// consumed the event.
//
// That looks like it should make a second hook here dead code. It does not.
// Syringe invokes EVERY registered handler for an address; the first non-zero
// return decides only where control ultimately transfers. This is verified at
// runtime, not assumed -- a third-party DLL's `return 0` observer at this exact
// address logs infiltrations in live games despite Antares being registered
// first and returning 0x4575A2. See
// encyclopedia/Spy-Infiltration.md § "VERIFIED - co-hooking 0x4571E0".
//
// The condition on that guarantee is absolute:
//
//   *** THIS HANDLER MUST ALWAYS RETURN 0. ***
//
// It is an observer. Returning a jump target would contend with Antares for
// control of the site, and then load order -- which we do not control -- would
// decide whose spy effects run at all.
//
// Stolen size is 0x5, matching Antares/Ares exactly. Declaring a different size
// at a shared address is its own hazard (a wider range can overlap a
// neighbouring hook's patch and corrupt its JMP displacement).

#include <BuildingClass.h>
#include <BuildingTypeClass.h>
#include <HouseClass.h>

#include <Utilities/Debug.h>
#include <Utilities/Macro.h>

#include <AcademyExt.h>
#include <Ext/BuildingType/Body.h>
#include <Ext/House/Body.h>

DEFINE_HOOK(0x4571E0, BuildingClass_Infiltrate_AcademyExt, 0x5)
{
	// LIVENESS PROBE -- deliberately the FIRST statement, before every bail.
	//
	// A same-address co-hook is legal (Syringe installs it without complaint)
	// but legal is not the same as LIVE, and whether an earlier handler's
	// non-zero return kills the rest of the chain is currently UNRESOLVED: this
	// repository has one in-game observation each way (a later handler at THIS
	// address did run; a later handler at 0x449CC1 never emitted a line at all).
	//
	// Without a probe here, "spy veterancy did nothing" is ambiguous between a
	// dead handler and a building with no SpyEffect.*Veterancy.Level set --
	// and that exact ambiguity has already cost a full debug round elsewhere.
	// Never conclude this handler runs from source alone; confirm the line.
	if (AcademyExtDLL::DebugLog)
		Debug::Log("[AcademyExt] infiltration hook at 0x4571E0 fired.\n");

	GET(BuildingClass*, pVictim, ECX);
	GET_STACK(HouseClass*, pEnterer, 0x4);

	if (!pVictim || !pVictim->Type || !pEnterer)
		return 0;

	auto const pTypeExt = BuildingTypeExt::ExtMap.Find(pVictim->Type);

	// Nothing to record unless the infiltrated building actually declares a
	// SpyEffect.*Veterancy.Level for at least one branch. Checked here as well
	// as inside RecordInfiltration so the common case costs one lookup and no
	// log line.
	if (!pTypeExt || !pTypeExt->HasAnySpyLevel())
		return 0;

	// The bonus belongs to the INFILTRATOR's house, keyed by the VICTIM's type
	// (mirrors Antares, which sets its flags on the enterer's house extension
	// from the entered building's type).
	if (auto const pHouseExt = HouseExt::ExtMap.Find(pEnterer))
	{
		pHouseExt->RecordInfiltration(pVictim->Type);

		Debug::Log("[AcademyExt] %s infiltrated %s: stolen veterancy recorded.\n",
			pEnterer->get_ID(), pVictim->Type->ID);
	}

	return 0; // never contend for control of this site
}
