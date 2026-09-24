#include "AcademyExt.h"

#include <Phobos.h>
#include <Syringe.h>
#include <Utilities/Patch.h>
#include <Utilities/Debug.h>
#include <Utilities/Macro.h>

HANDLE AcademyExtDLL::hInstance = nullptr;

char AcademyExtDLL::readBuffer[AcademyExtDLL::readLength];
wchar_t AcademyExtDLL::wideBuffer[AcademyExtDLL::readLength];

bool AcademyExtDLL::Authoritative = false;
bool AcademyExtDLL::DebugLog = false;

void AcademyExtDLL::ExeRun()
{
	Patch::ApplyStatic();
}

// Deliberately NOT called from ExeRun. Anything logged at ExeRun time is written
// before the log file exists and is silently lost -- which is exactly why the
// banner added last round never appeared, and why `grep AcademyExt debug.log`
// stayed ambiguous between "not injected" and "injected but silent". IntelExt
// hit and documented the same trap; this mirrors its fix by logging from the
// first rules parse instead.
void AcademyExtDLL::LogBannerOnce()
{
	static bool logged = false;

	if (logged)
		return;

	logged = true;

	Debug::Log("[AcademyExt] loaded. Set [General] AcademyExt.Debug=yes to log "
		"every veterancy decision.\n");

	// AcademyExt reimplements the academy pipeline rather than extending
	// Antares', and composes with it by only ever raising. Without Antares the
	// features still work, but a modder comparing behaviour should know which
	// half of the stack is missing rather than guess.
	if (!GetModuleHandleA("Antares.dll"))
	{
		Debug::Log("[AcademyExt] NOTE: Antares.dll is not loaded. AcademyExt's own "
			"academy/country/spy veterancy still applies, but Antares' academy "
			"and its SpyEffect promotions are absent. Ares is not a supported "
			"substitute.\n");
	}
}

bool __stdcall DllMain(HANDLE hInstance, DWORD dwReason, LPVOID)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		AcademyExtDLL::hInstance = hInstance;
		Phobos::hInstance = hInstance; // needed by Patch::ApplyStatic
	}
	return true;
}

SYRINGE_HANDSHAKE(pInfo)
{
	pInfo->Message = const_cast<char*>("AcademyExt");
	return S_OK;
}

// Main-loop entry, so static patches apply at the right time.
DEFINE_HOOK(0x7CD810, AcademyExt_ExeRun, 0x9)
{
	AcademyExtDLL::ExeRun();
	return 0;
}

// Flush the deferred debug log once the command line has been parsed.
DEFINE_HOOK(0x52F639, AcademyExt_CmdLineParse, 0x5)
{
	Debug::LogDeferredFinalize();
	return 0;
}
