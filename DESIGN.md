# AcademyExt — Design

A standalone Syringe DLL that makes veterancy bonuses **composable**. It replaces
the fixed "highest academy wins" rule with one configurable resolution model that
covers academy buildings, spy/infiltration effects, and a new passive country
bonus.

- **Depends on:** Antares (co-loaded). **Never edits Antares.**
- **Framework lineage:** YRpp / Syringe, Ares-lineage. Do not use Ares — see
  `antares-replaces-ares`.
- **Status:** feature-complete, CI green, deployed. **Stacking verified in
  game** (0.5-per-building academy: 1 → no chevron, 2 → veteran, 4 → elite,
  which Antares' `max` could not produce). Remaining features implemented but
  not yet exercised in game.

---

## 1. The problem

Four unrelated codepaths in Antares all mean "this object spawns pre-promoted",
and none of them compose:

| Source | Where | Magnitude | Stacks? |
|---|---|---|---|
| Academy buildings | `HouseExt::ApplyAcademy` | `double` (0.0–VeteranCap) | no — `std::max` |
| Spy on factory (legacy) | `SpyEffect.UnitVeterancy` → vanilla `*Infiltrated` flags | hardcoded veteran (1.0) | n/a |
| Spy per-branch | `SpyEffect.{Infantry,Vehicle,Naval,Aircraft,Building}Veterancy` | hardcoded veteran (1.0) | n/a |
| Country veteran lists | `VeteranBuildings=`, vanilla `VeteranX=` | hardcoded veteran (1.0) | n/a |

Only the first has a tunable magnitude, and it is explicitly non-stacking.
Everything else is a boolean that means exactly 1.0.

**Note:** Antares already solved the *branch coverage* problem that the Ares docs
describe — the five per-branch `SpyEffect.*Veterancy` flags do not exist in Ares.
What is still missing is **magnitude** and **stacking**, which is what this
project adds.

---

## 2. The unifying model

Every promotion source becomes a **contribution**: `(value: double, stacks: bool)`,
resolved per category.

**Categories** — `Infantry`, `Vehicle`, `Aircraft`, `Building`, matching Antares'
existing `considerAs` dispatch (`Organic=yes` vehicles count as Infantry,
`ConsideredAircraft=yes` as Aircraft). `Naval` is a **spy-only sub-branch**: naval
units are `UnitType`s with `Naval=yes`, so they resolve as `Vehicle` for academy
purposes but can carry a distinct spy contribution.

**Resolution**, per category, for one freshly created object:

```
best_single = max(0.0, max{ v : contributions where stacks == false })
stack_sum   =            sum{ v : contributions where stacks == true  }
result      = max(best_single, stack_sum)
result      = clamp(result, 0.0, RulesClass::VeteranCap)

if (type->Trainable && result > techno->Veterancy.Veterancy)
    techno->Veterancy.Veterancy = result
```

Walking the motivating example — three academies with values A (non-stacking),
B and C (stacking): candidates are `{A}` and `{B+C}`, so the result is
`max(A, B+C)`. Exactly the requested behavior.

The `max(0.0, ...)` lower clamp is **load-bearing** — see §4.

---

## 3. INI surface

Decision: **reuse Antares' `Academy.*` namespace** (backward compatible; existing
mods work unchanged and gain stacking only when they opt in).

### 3a. Academy buildings — one new tag

```ini
[SomeBuilding]
Academy.InfantryVeterancy=1.0    ; existing Antares tags, unchanged
Academy.AircraftVeterancy=
Academy.VehicleVeterancy=
Academy.BuildingVeterancy=
Academy.Types=                   ; existing whitelist
Academy.Ignore=                  ; existing blacklist
Academy.Stacks=no                ; NEW. default no == current Antares behavior
```

`Academy.Stacks` is per-building, not per-category. Per-category stacking
(`Academy.Stacks.Infantry=`) is a possible later extension; deliberately omitted
until someone needs it.

### 3b. Country passive bonus — new

```ini
[Americans]
AcademyBonus=1.0                 ; shorthand: sets all four categories
AcademyBonus.Infantry=           ; per-category override
AcademyBonus.Vehicle=
AcademyBonus.Aircraft=
AcademyBonus.Building=
AcademyBonus.Stacks=yes          ; default YES — a passive bonus is meant to combine
AcademyBonus.Types=              ; optional whitelist, same semantics as Academy.Types
AcademyBonus.Ignore=             ; optional blacklist
```

Numeric scale only: `1.0` = veteran, `2.0` = elite, decimals = partial progress
toward the next rank. No keyword aliases — matches Antares' existing convention.

This subsumes `VeteranX=`/`EliteX=`: instead of enumerating types per country, a
country grants a blanket level, optionally scoped with `.Types`/`.Ignore`.

### 3c. Spy / infiltration — new `.Level` tags

Antares' `SpyEffect.*Veterancy=` booleans call `SetVeteran()` (a hard 1.0) and
**cannot be lowered from outside the DLL**. So magnitudes live on new tags, and
the old booleans must be left **unset**.

```ini
[SomeFactory]
SpyEffect.InfantryVeterancy.Level=1.0
SpyEffect.VehicleVeterancy.Level=
SpyEffect.NavalVeterancy.Level=
SpyEffect.AircraftVeterancy.Level=
SpyEffect.BuildingVeterancy.Level=
SpyEffect.Veterancy.Stacks=yes   ; default yes
SpyEffect.Veterancy.Types=
SpyEffect.Veterancy.Ignore=
```

> **Documentation must state:** setting both `SpyEffect.InfantryVeterancy=yes`
> *and* `SpyEffect.InfantryVeterancy.Level=0.5` yields **1.0**, not 0.5 — Antares
> wins the floor. Use one or the other, never both.

---

## 4. Coexistence with Antares

Antares' academy state (`HouseExt::ExtData::Academies`) is private to Antares'
ext map with no exported API, so **AcademyExt must reimplement the pipeline**, not
extend it. Both DLLs then run at the same addresses.

**This is provably safe for the academy half.** Antares computes
`A = max(all academy values)`. AcademyExt computes
`M = max(best_single, stack_sum)`. Every academy lands in exactly one bucket:

- if the max-valued academy is non-stacking, `best_single ≥ A`, so `M ≥ A`;
- if it is stacking, `stack_sum ≥ A` (all values non-negative), so `M ≥ A`.

Therefore `M ≥ A` always. Both handlers are **raise-only**
(`if (bonus > value)`), so running both in either order yields `max(A, M) = M`.
Antares' handler becomes a harmless no-op underneath.

**Two invariants this proof depends on — do not break them:**

1. **Non-negative clamp.** A negative `Academy.*Veterancy` is possible in INI.
   Antares implicitly clamps at 0 by seeding `veterancyBonus = 0.0`. AcademyExt's
   `stack_sum` could go negative without the explicit `max(0.0, ...)`, which
   would break `M ≥ A`.
2. **Raise-only.** In the default mode AcademyExt never *lowers* an existing
   veterancy value.

Syringe hook **order between DLLs is not controllable**. The default mode
deliberately relies on commutativity (both raise-only) rather than on winning
the race.

### 4a. Authoritative mode — reduction *is* reachable

An earlier revision of this document claimed a cap/override/demote academy was
"architecturally impossible". **That was wrong**, and it conflated a property of
the default mode with a property of the architecture. Three facts make reduction
reachable:

1. **Antares' academy is entirely INI-gated.** `BuildingTypeExt::Academy` is
   computed once at parse time (`AcademyInfantry > 0.0 || ...`) and
   `IsAcademy()` is the *only* gate on list membership across all four of its
   bookkeeping hooks. With those tags unset its `Academies` list is permanently
   empty, `ApplyAcademy` resolves `0.0`, and its `if (bonus > value)` never
   fires. It can be neutralised by configuration alone — no patching, and no
   need for a runtime "disable Antares" switch.
2. **Syringe runs handlers in `-i=` order**, and all academy handlers return
   `0`. Antares is injected early and AcademyExt late, so ours already runs
   **last** at every apply site. Whatever we write is the final word — the only
   thing stopping us was our own clamp.
3. **Nothing re-raises it afterwards.** Antares' per-frame
   `TechnoClass_Update_Veterancy` (`0x6FA054`) calls `HandlePromotion`, which
   reacts to rank *changes* and never writes veterancy itself.

**`[General] AcademyExt.Authoritative=yes`** therefore switches the final write
from raise-only to unconditional, which is what lets `Academy.Cap=` /
`AcademyBonus.Cap=` actually lower a rank.

**The trade, stated plainly:** authoritative mode is **load-order dependent**.
It is correct only while AcademyExt is injected *after* Antares. If that were
reversed, Antares would run last and raise the value back — caps would silently
stop working while everything else kept functioning. That is why it is opt-in,
defaults off, and logs the requirement loudly when enabled. The default mode
remains provably order-independent.

**Why lowering goes through a cap rather than a contribution.** The resolution
rule is `max(best, sum)`, which can never yield a smaller number, so no
contribution can pull a rank down. Authoritative mode also starts from
`max(resolved, current)` so that ranks earned elsewhere (spy effects,
`VeteranBuildings`, country `VeteranX`) survive by default. The cap is the
single, explicit lowering tool — which keeps "this reduces veterancy" a thing
the modder opts into per academy rather than an emergent side effect.

**Known cosmetic hazard for any future *runtime* demote.** `HandlePromotion`
selects its sound, flash and `Promote_VeteranType` conversion by the **new**
rank, so demoting elite→veteran mid-life would fire the *veteran promotion*
effects. Initial-rank reduction is unaffected: at `Init` the object's
`CurrentRanking` is still `Rank::Invalid`, which that function guards against.

---

## 5. Hooks

Checked against the YR Hook Encyclopedia (`registry/hooks.csv`, 2026-08).
Every address below lists **only Ares + Antares** as consumers, and those two are
mutually exclusive, so there are **no real conflicts** — AcademyExt would be the
sole third consumer. Phobos, Kratos and CnCNet-Spawner touch none of them.

### 5a. Safe to chain — Antares' handler returns `0`

| Address | Antares name | Registers | Purpose |
|---|---|---|---|
| `0x517D51` | `InfantryClass_Init_Academy` | `ESI` = InfantryClass* | apply, infantry |
| `0x735678` | `UnitClass_Init_Academy` (inlined in CTOR) | `ESI` = UnitClass* | apply, vehicle |
| `0x74689B` | `UnitClass_Init_Academy` | `ESI` = UnitClass* | apply, vehicle |
| `0x413FD2` | `AircraftClass_Init_Academy` | `ESI` = AircraftClass* | apply, aircraft |
| `0x442D1B` | `BuildingClass_Init_Academy` | `ESI` = BuildingClass* | apply, building |
| `0x446366` | `BuildingClass_Place_Academy` | `EBP` = BuildingClass* | academy list add |
| `0x445905` | `BuildingClass_Remove_Academy` | `ESI` = BuildingClass* | academy list remove |
| `0x448AB2` | `..._ChangeOwnership_Remove_Academy` | `ESI` = BuildingClass* | list remove |
| `0x4491D5` | `..._ChangeOwnership_Add_Academy` | `ESI` = BuildingClass* | list add |

All nine of Antares' handlers `return 0`, so chaining is safe (same pattern the
encyclopedia records as verified at `0x702050` in `Techno-Instance-Lifecycle.md`).
AcademyExt's handlers must likewise always `return 0`.

### 5b. Infiltration — hook `0x4571E0` directly, and return `0`

**This section previously said the opposite.** It claimed that because Antares'
`BuildingClass_Infiltrate` at `0x4571E0` is a whole-function wrapper returning
`0x4575A2`, a second hook there would be silently dead, and that Phase 5 was
gated on verifying the jump target `0x4575A2` in a debugger.

**That was wrong, and it is now settled by runtime evidence.** Syringe invokes
*every* registered handler for an address; the first non-zero return decides only
where control ultimately transfers, not whether later handlers execute. Proof:
IntelExt registers a handler at `0x4571E0` that returns `0`, and its log line
appears 8 times across live-game `debug.*.log` history *even though Antares is
registered first in `wine-game.sh` and returns `0x4575A2`*. Recorded in the
encyclopedia at `Spy-Infiltration.md` § "VERIFIED — co-hooking `0x4571E0`".

| Address | Purpose | Status |
|---|---|---|
| `0x4571E0` | record (house, building) into AcademyExt's own house ext | ✅ safe to co-hook |

```
ECX       = BuildingClass* the building entered
[ESP+0x4] = HouseClass*    the infiltrator's house
→ MUST return 0 (observer only; never contend for control of this site)
```

`0x4575A2` is a dead end and should not be used — a documented calling convention
beats an unverified one. **Phase 5 is therefore unblocked**; nothing about it
requires a debugger session.

The one real constraint remains, and it is not about hooks: Antares' own
`SpyEffect.*Veterancy=` booleans call `SetVeteran()` (a hard `1.0`) and, because
everything here is raise-only, no external DLL can bring that back down. Partial
spy levels require the new `.Level` tags *and* the framework's booleans left
unset.

---

## 6. Behaviors to mirror exactly

Divergence from Antares here would produce visible inconsistencies:

- **`Unsorted::ScenarioInit` skip.** Antares bails out of `ApplyAcademy` during
  scenario init so preplaced objects get no academy bonus (and to dodge deploy
  "conversions"). AcademyExt must do the same or preplaced units gain bonuses
  Antares would never grant.
- **`RulesClass::VeteranCap`** caps the final value. (Vanilla read site
  `0x66EF87`.)
- **`Trainable=yes`** gate on the receiving type.
- **Whitelist/blacklist semantics**: empty whitelist means "all types"; blacklist
  always wins.

---

## 7. Risk register

| Risk | Severity | Mitigation |
|---|---|---|
| ~~`0x4575A2` register/stack state~~ | **RETIRED** | was never a real risk — `0x4571E0` is co-hookable, so `0x4575A2` is not used at all (§5b) |
| Syringe inter-DLL hook order uncontrollable | medium | design relies on raise-only commutativity, not ordering |
| Negative INI values break the `M ≥ A` proof | medium | explicit `max(0.0, ...)` clamp, §4 |
| Savegame: AcademyExt house ext must serialize | medium | own `.Process()` chain; academy list rebuilt identically on load |
| MP desync | medium | all state is deterministic house data derived from INI + game events; no RNG. Cf. `kratos-rng-desync-rootcause` for the class of bug to avoid |
| User sets both Antares bool and `.Level` | low | document loudly (§3c); consider a startup warning in the log |
| Antares changes its academy internals upstream | low | AcademyExt reads only INI + game state, never Antares' ext maps, so it is insulated |

---

## 8. Phases

1. ~~**Verify `0x4575A2`.**~~ **RETIRED — the premise was false.** See §5b:
   `0x4571E0` is safely co-hookable, so there is no jump-target seam to verify
   and no debugger session needed.
2. ~~**Scaffold.**~~ **DONE.** Repo, YRpp/Phobos submodules, CI (Windows-only
   build), house + buildingtype + housetype ext with serialization.
3. ~~**Academy core.**~~ **DONE.** Own academy list (four list hooks) +
   resolution engine + five apply hooks. `Academy.Stacks` shipped.
4. ~~**Country bonus.**~~ **DONE** — landed with the scaffold; `AcademyBonus*`
   on HouseType is folded in by `ApplyAcademy` as a contribution.
5. ~~**Spy levels.**~~ **DONE.** `SpyEffect.*.Level` recorded by a `return 0`
   observer at `0x4571E0` (`src/Hooks.Infiltration.cpp`) into
   `HouseExt::InfiltratedSources`, resolved by `AddSpyContributions`.
6. ~~**Encyclopedia contribution.**~~ **DONE** — `encyclopedia/Veterancy-Academy.md`
   plus a runtime-verified correction to the Syringe chain-semantics claim in
   `Spy-Infiltration.md` and `Buildability-Prerequisites.md`.

---

## 9. Open questions

- Should `Academy.Stacks` be per-category rather than per-building? Deferred
  until there is a concrete need.
- Should spy veterancy bonuses be **losable** when the infiltrated building is
  destroyed or recaptured? Vanilla infiltration is permanent; the `.Level` form
  could support expiry, but that adds state and is out of scope for v1.
- Does a country `AcademyBonus` apply to objects gained by **capture/mind-control**
  or only to newly produced ones? Current design follows the Init hooks, so it is
  creation-time only — capture does not re-run these.
