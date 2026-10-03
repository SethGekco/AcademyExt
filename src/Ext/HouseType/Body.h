#pragma once

#include <Utilities/Container.h>
#include <Utilities/TemplateDef.h>

#include <HouseTypeClass.h>
#include <TechnoTypeClass.h>

#include <AcademyEnums.h>

// Per-country passive veterancy bonus.
//
// This generalises the stock VeteranX= / EliteX= country lists: instead of
// enumerating individual types per country, a country grants a blanket level
// that can optionally be scoped with .Types / .Ignore, and that composes with
// every other veterancy source rather than sitting in its own codepath.
class HouseTypeExt
{
public:
	using base_type = HouseTypeClass;

	static constexpr DWORD Canary = 0x0ACADE02;

	class ExtData final : public Extension<HouseTypeClass>
	{
	public:
		// Shorthand: AcademyBonus= sets every category at once.
		Nullable<double> AcademyBonus;

		// Per-category overrides. Nullable so an unset override falls through
		// to the shorthand rather than reading as an explicit 0.0.
		Nullable<double> AcademyBonusInfantry;
		Nullable<double> AcademyBonusVehicle;
		Nullable<double> AcademyBonusAircraft;
		Nullable<double> AcademyBonusBuilding;

		// Defaults to YES, unlike academy buildings: a passive country bonus is
		// meant to combine with what the player builds, not compete with it.
		Valueable<bool> AcademyBonusStacks;

		// Ceiling this country imposes on the FINAL veterancy, from any source.
		// Same authoritative-mode caveat as BuildingTypeExt::AcademyCap, and the
		// same category precedence: a per-category tag wins, the bare form is the
		// all-categories fallback.
		Nullable<double> AcademyBonusCap;
		Nullable<double> AcademyBonusCapInfantry;
		Nullable<double> AcademyBonusCapVehicle;
		Nullable<double> AcademyBonusCapAircraft;
		Nullable<double> AcademyBonusCapBuilding;

		ValueableVector<TechnoTypeClass*> AcademyBonusWhitelist;
		ValueableVector<TechnoTypeClass*> AcademyBonusBlacklist;

		ExtData(HouseTypeClass* OwnerObject) : Extension<HouseTypeClass>(OwnerObject)
			, AcademyBonus {}
			, AcademyBonusInfantry {}
			, AcademyBonusVehicle {}
			, AcademyBonusAircraft {}
			, AcademyBonusBuilding {}
			, AcademyBonusStacks { true }
			, AcademyBonusCap {}
			, AcademyBonusCapInfantry {}
			, AcademyBonusCapVehicle {}
			, AcademyBonusCapAircraft {}
			, AcademyBonusCapBuilding {}
			, AcademyBonusWhitelist {}
			, AcademyBonusBlacklist {}
		{ }

		virtual ~ExtData() = default;

		virtual void LoadFromINIFile(CCINIClass* pINI) override;
		virtual void Initialize() override { }
		virtual void InvalidatePointer(void* ptr, bool bRemoved) override { }

		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;

		// Resolution order: per-category override, then the shorthand, then 0.
		double GetBonus(AcademyCategory category) const;
		bool HasBonus(AcademyCategory category) const;

		// Same precedence, for the ceiling.
		bool HasCap(AcademyCategory category) const;
		double GetCap(AcademyCategory category) const;
		bool HasAnyCap() const;

		bool AppliesTo(TechnoTypeClass* pType) const;

	private:
		// Resolves the per-category / all-categories precedence once, so HasCap
		// and GetCap cannot disagree about which tag applies.
		Nullable<double> const* CapFor(AcademyCategory category) const;

		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<HouseTypeExt>
	{
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;
};
