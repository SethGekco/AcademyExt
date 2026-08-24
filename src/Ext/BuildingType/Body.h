#pragma once

#include <Utilities/Container.h>
#include <Utilities/TemplateDef.h>

#include <BuildingTypeClass.h>
#include <TechnoTypeClass.h>

#include <AcademyEnums.h>

// Per-BuildingType academy and spy-veterancy settings.
//
// Container<T> runs in unordered_map mode (Canary defined, no ExtPointerOffset)
// so we claim no pointer slot inside BuildingTypeClass and never collide with
// Antares' or Phobos' extension storage. This matters more here than usual:
// AcademyExt is designed to be co-loaded with Antares, which keeps its own
// BuildingType extension on the same objects.
class BuildingTypeExt
{
public:
	using base_type = BuildingTypeClass;

	// Distinct from Phobos (0x11111111 / 0xAFFEAFFE), SquadExt (0x50DAC77 /
	// 0x50DEC77) and PrerequisiteExt (0xB2B2B2B2).
	static constexpr DWORD Canary = 0x0ACADE01;

	class ExtData final : public Extension<BuildingTypeClass>
	{
	public:
		// -- academy --
		// These are Antares' OWN tag names, read independently by us. Reusing
		// the namespace is deliberate: existing mods keep working untouched and
		// opt into stacking with one extra flag. See DESIGN.md section 4 for why
		// running both readers is safe.
		Valueable<double> AcademyInfantry;
		Valueable<double> AcademyAircraft;
		Valueable<double> AcademyVehicle;
		Valueable<double> AcademyBuilding;
		ValueableVector<TechnoTypeClass*> AcademyWhitelist; // Academy.Types
		ValueableVector<TechnoTypeClass*> AcademyBlacklist; // Academy.Ignore

		// NEW. Default no == current Antares behaviour, so an untouched mod
		// behaves identically to before.
		Valueable<bool> AcademyStacks;

		// -- spy / infiltration magnitudes --
		// Nullable so "unset" is distinguishable from "set to 0.0"; only a set
		// value registers a contribution at all.
		//
		// These are NEW tags, not Antares'. Antares' SpyEffect.*Veterancy=
		// booleans call SetVeteran() (a hard 1.0) and cannot be lowered from
		// outside that DLL, so a partial bonus is only reachable via a tag
		// Antares does not read. Setting both yields 1.0, not the partial value.
		Nullable<double> SpyInfantryLevel;
		Nullable<double> SpyVehicleLevel;
		Nullable<double> SpyNavalLevel;
		Nullable<double> SpyAircraftLevel;
		Nullable<double> SpyBuildingLevel;
		Valueable<bool> SpyStacks;
		ValueableVector<TechnoTypeClass*> SpyWhitelist;
		ValueableVector<TechnoTypeClass*> SpyBlacklist;

		ExtData(BuildingTypeClass* OwnerObject) : Extension<BuildingTypeClass>(OwnerObject)
			, AcademyInfantry { 0.0 }
			, AcademyAircraft { 0.0 }
			, AcademyVehicle { 0.0 }
			, AcademyBuilding { 0.0 }
			, AcademyWhitelist {}
			, AcademyBlacklist {}
			, AcademyStacks { false }
			, SpyInfantryLevel {}
			, SpyVehicleLevel {}
			, SpyNavalLevel {}
			, SpyAircraftLevel {}
			, SpyBuildingLevel {}
			, SpyStacks { true }
			, SpyWhitelist {}
			, SpyBlacklist {}
		{ }

		virtual ~ExtData() = default;

		virtual void LoadFromINIFile(CCINIClass* pINI) override;
		virtual void Initialize() override { }
		virtual void InvalidatePointer(void* ptr, bool bRemoved) override { }

		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;

		// Mirrors Antares: a building is an academy iff any category is > 0.
		bool IsAcademy() const;

		// Academy bonus for one category. Naval has no academy counterpart and
		// is not accepted here -- it is a spy-only branch.
		double GetAcademyValue(AcademyCategory category) const;

		// Spy level for one branch, or nullptr-equivalent (unset) as false.
		bool HasSpyLevel(SpyBranch branch) const;
		double GetSpyLevel(SpyBranch branch) const;

		// True if any branch declares a level at all. Lets the infiltration
		// observer reject the overwhelmingly common case in one call.
		bool HasAnySpyLevel() const;

		// Whitelist/blacklist gate. Empty whitelist means "all types";
		// the blacklist always wins. Mirrors Antares.
		bool AcademyAppliesTo(TechnoTypeClass* pType) const;
		bool SpyAppliesTo(TechnoTypeClass* pType) const;

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<BuildingTypeExt>
	{
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;
};
