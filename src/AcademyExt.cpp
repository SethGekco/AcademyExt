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

void AcademyExtDLL::ExeRun()
{
	Patch::ApplyStatic();
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
