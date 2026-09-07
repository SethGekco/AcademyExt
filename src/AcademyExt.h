#pragma once

#include <Windows.h>

class AcademyExtDLL
{
public:
	static HANDLE hInstance;

	static constexpr size_t readLength = 2048;
	static char readBuffer[readLength];
	static wchar_t wideBuffer[readLength];

	// [General] AcademyExt.Authoritative -- see Hooks.Rules.cpp.
	//
	// OFF (default): the resolved bonus is applied raise-only, which is
	// commutative with Antares and therefore independent of Syringe load order.
	// ON: the resolved value is written unconditionally whenever AcademyExt has
	// something to say, making it the final authority and enabling Academy.Cap
	// to actually lower a rank. Correct only while AcademyExt is injected AFTER
	// Antares -- see the warning logged when this is enabled.
	//
	// Plain static, not serialized: it is derived from [General] on every rules
	// pass, and rules are always read before any scenario or savegame loads.
	static bool Authoritative;

	static void ExeRun();
};
