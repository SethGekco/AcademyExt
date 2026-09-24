// Global [General] settings.
//
// ENCYCLOPEDIA-CHECKED: 0x679CAF is RulesClass::LoadAfterTypeData, hooked by
// Antares, Ares AND Phobos (all stolen 0x5, all return 0). It is a proven
// multi-consumer chain point -- another third-party DLL in this same deployment
// already co-hooks it to parse its own globals. ESI = CCINIClass* for the pass
// currently being read, so map and game-mode INIs can override the value.
//
// Ours returns 0 like everyone else's.

#include <CCINIClass.h>
#include <RulesClass.h>

#include <Utilities/Debug.h>
#include <Utilities/Macro.h>

#include <AcademyExt.h>

DEFINE_HOOK(0x679CAF, RulesClass_LoadAfterTypeData_AcademyExt, 0x5)
{
	GET(CCINIClass*, pINI, ESI);

	if (!pINI)
		return 0;

	// First point in startup where the log file exists. See LogBannerOnce.
	AcademyExtDLL::LogBannerOnce();

	bool const wasAuthoritative = AcademyExtDLL::Authoritative;

	AcademyExtDLL::Authoritative =
		pINI->ReadBool("General", "AcademyExt.Authoritative", AcademyExtDLL::Authoritative);

	AcademyExtDLL::DebugLog =
		pINI->ReadBool("General", "AcademyExt.Debug", AcademyExtDLL::DebugLog);

	// Log the transition only, not every pass -- Read_File runs several times
	// (rulesmd, game mode, map) and this would otherwise spam the log.
	if (AcademyExtDLL::Authoritative && !wasAuthoritative)
	{
		Debug::Log("[AcademyExt] Authoritative mode ENABLED. AcademyExt now writes "
			"veterancy unconditionally instead of raise-only, so Academy.Cap can "
			"lower a rank.\n");
		Debug::Log("[AcademyExt] REQUIREMENT: AcademyExt.dll must be injected AFTER "
			"Antares.dll in the Syringe -i= list. Syringe runs handlers in -i= "
			"order; if Antares runs after us it will raise the value back and any "
			"cap will silently do nothing.\n");
	}
	else if (!AcademyExtDLL::Authoritative && wasAuthoritative)
	{
		Debug::Log("[AcademyExt] Authoritative mode disabled by a later INI pass; "
			"back to raise-only.\n");
	}

	return 0;
}
