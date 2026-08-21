#pragma once

#include <vector>

#include <Utilities/Container.h>
#include <Utilities/TemplateDef.h>

#include <HouseClass.h>

#include <AcademyEnums.h>
#include <Veterancy/Resolver.h>

class TechnoClass;
class TechnoTypeClass;
class BuildingClass;
class BuildingTypeClass;

// Per-house veterancy state: which academies this house owns, and which
// building types it has infiltrated.
//
// AcademyExt keeps its OWN copy of both. Antares' equivalent state lives in
// HouseExt::ExtData::Academies inside Antares' ext map, which has no exported
// API and is unreachable from a separate DLL -- so this is a reimplementation,
// not an extension. DESIGN.md section 4 covers why running both is safe.
class HouseExt
{
public:
	using base_type = HouseClass;

	static constexpr DWORD Canary = 0x0ACADE03;

	class ExtData final : public Extension<HouseClass>
	{
	public:
		// Academy buildings currently owned and on the map.
		std::vector<BuildingClass*> Academies;

		// Building TYPES this house has infiltrated, per spy branch. Types, not
		// instances: vanilla infiltration is a permanent per-house fact that
		// outlives the building, and storing types keeps re-infiltration
		// idempotent instead of silently stacking with itself.
		std::vector<BuildingTypeClass*> InfiltratedSources[SpyBranchCount];

		ExtData(HouseClass* OwnerObject) : Extension<HouseClass>(OwnerObject)
			, Academies {}
			, InfiltratedSources {}
		{ }

		virtual ~ExtData() = default;

		virtual void LoadFromINIFile(CCINIClass* pINI) override { }
		virtual void Initialize() override { }
		virtual void InvalidatePointer(void* ptr, bool bRemoved) override;

		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;

		// Add/remove an academy building. Idempotent in both directions.
		void UpdateAcademy(BuildingClass* pAcademy, bool added);

		// Record that this house infiltrated a building of this type. Registers
		// the type under every spy branch the type actually defines a level for.
		void RecordInfiltration(BuildingTypeClass* pBuildingType);

		// Resolve every contribution for this category and apply the result to
		// a freshly created object. Raise-only.
		void ApplyAcademy(TechnoClass* pTechno, AcademyCategory category) const;

	private:
		void AddSpyContributions(
			VeterancyResolver& resolver,
			TechnoTypeClass* pType,
			AcademyCategory category) const;

		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<HouseExt>
	{
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;
};
