# AcademyExt — INI Reference

All veterancy values use Antares' numeric scale:

| Value | Meaning |
|---|---|
| `0.0` | no bonus (default) |
| `0.5` | halfway to veteran (partial) |
| `1.0` | veteran |
| `2.0` | elite |

Everything is capped by `[General] VeteranCap=`, and only types with
`Trainable=yes` receive bonuses.

## The resolution rule

Every source of veterancy becomes a *contribution* carrying a value and a
"stacks" flag. For each object built:

```
result = max( best non-stacking contribution, sum of all stacking ones )
```

With three academies — A non-stacking, B and C stacking — the game awards
`max(A, B+C)`. Whichever is genuinely better wins.

By default **AcademyExt only ever raises veterancy**. A contribution can never
pull a rank down — `max(best, sum)` cannot produce a smaller number. To lower a
rank you need a cap *and* authoritative mode, both described below.

---

## Authoritative mode & caps — `[General]`

```ini
[General]
AcademyExt.Authoritative=no     ; default no
```

**Default (`no`):** the resolved bonus is applied raise-only. This is
commutative with Antares, so the result does not depend on which order the DLLs
were injected. Caps are ignored (and the DLL logs a warning if you set one).

**`yes`:** AcademyExt becomes the final writer for veterancy at unit creation
and writes unconditionally *whenever it has something to say* — at least one
contribution, or an applicable cap. This is what allows a cap to lower a rank.

> **⚠ Requires AcademyExt to be injected AFTER Antares** in the Syringe `-i=`
> list. Syringe runs handlers in `-i=` order; if Antares ran after us it would
> raise the value back and every cap would silently do nothing. The DLL logs
> this requirement when the mode is enabled.

### Caps

```ini
[SomeBuilding]
Academy.Cap=1.0                 ; this building ceilings the final rank

[SomeCountry]
AcademyBonus.Cap=1.0            ; this country ceilings the final rank
```

A cap is a ceiling on the **final** veterancy from *any* source — including spy
effects, `VeteranBuildings` and Antares' own academy. When several apply, the
**lowest** wins: a cap is a promise about a maximum, so two caps must never be
able to raise each other.

A building may declare `Academy.Cap=` **without** any `Academy.*Veterancy` — it
then grants nothing and only imposes its ceiling.

| Goal | How |
|---|---|
| **Cap** — nothing exceeds veteran | `Academy.Cap=1.0` |
| **Demote** — force rookie | `Academy.Cap=0.0` |
| **Override** — force exactly 0.5 | `Academy.Cap=0.5` plus a contribution of `0.5` |

Everything is still bounded by `[General] VeteranCap=`.

---

## Academy buildings — `BuildingType`

```ini
[SomeBuilding]
Academy.InfantryVeterancy=0.0   ; InfantryTypes and Organic=yes vehicles
Academy.VehicleVeterancy=0.0    ; VehicleTypes that are neither organic nor aircraft
Academy.AircraftVeterancy=0.0   ; AircraftTypes and ConsideredAircraft=yes vehicles
Academy.BuildingVeterancy=0.0   ; BuildingTypes
Academy.Types=                  ; whitelist; empty means all types
Academy.Ignore=                 ; blacklist; always wins over the whitelist
Academy.Stacks=no               ; NEW
Academy.Cap=                    ; NEW -- ceiling; needs authoritative mode
```

The first six tags are Antares' own and behave exactly as before.

**`Academy.Stacks`** is the new one. `no` (the default) keeps the stock
behaviour — this building competes for "highest single bonus". `yes` means it
adds into the stacking pool instead.

Because the default is `no`, an existing mod behaves identically until you
opt in.

### Example

```ini
[GAACAD]          ; elite academy, does not stack
Academy.InfantryVeterancy=2.0
Academy.Stacks=no

[NAFLAG]          ; small stacking bonuses
Academy.InfantryVeterancy=0.75
Academy.Stacks=yes
```

Own one `GAACAD`: infantry spawn at `2.0` (elite).
Own three `NAFLAG`: `0.75 × 3 = 2.25`, capped to `VeteranCap`.
Own both: `max(2.0, 2.25)` → the stack wins.

---

## Passive country bonus — `Country`

```ini
[Americans]
AcademyBonus=1.0                ; shorthand: applies to all four categories
AcademyBonus.Infantry=          ; per-category override
AcademyBonus.Vehicle=
AcademyBonus.Aircraft=
AcademyBonus.Building=
AcademyBonus.Stacks=yes         ; default YES
AcademyBonus.Cap=               ; optional ceiling -- see "Authoritative mode & caps"
AcademyBonus.Types=             ; optional whitelist
AcademyBonus.Ignore=            ; optional blacklist
```

A per-category tag overrides the shorthand for that category only; categories
you leave unset fall back to `AcademyBonus=`.

This replaces the need for `VeteranX=` / `EliteX=` country lists: rather than
enumerating types, grant a level and optionally scope it.

`AcademyBonus.Stacks` defaults to **yes**, unlike academy buildings — a passive
national bonus is meant to combine with what the player builds.

> **Not inherited from `ParentCountry=`.** A child country must declare its own
> `AcademyBonus`. Silently inheriting a stacking bonus would give a country a
> bonus its INI never mentions.

### Example

```ini
[Russians]
AcademyBonus.Vehicle=1.0        ; all Russian vehicles roll off veteran
AcademyBonus.Stacks=yes
```

Paired with a stacking `0.5` academy, Russian vehicles reach `1.5`.

---

## Spy / infiltration levels — `BuildingType`

Infiltrating a building grants the **infiltrator's house** a permanent
contribution at the given level, for each branch the building defines.

```ini
[SomeFactory]
SpyEffect.InfantryVeterancy.Level=
SpyEffect.VehicleVeterancy.Level=
SpyEffect.NavalVeterancy.Level=
SpyEffect.AircraftVeterancy.Level=
SpyEffect.BuildingVeterancy.Level=
SpyEffect.Veterancy.Stacks=yes  ; default yes
SpyEffect.Veterancy.Types=
SpyEffect.Veterancy.Ignore=
```

### ⚠ Do not combine with Antares' booleans

Antares' own `SpyEffect.InfantryVeterancy=yes` (no `.Level` suffix) promotes to
veteran through a codepath AcademyExt cannot lower. Setting both:

```ini
SpyEffect.InfantryVeterancy=yes           ; Antares -> hard 1.0
SpyEffect.InfantryVeterancy.Level=0.5     ; AcademyExt -> wants 0.5
```

yields **1.0**, not 0.5. Use one or the other, never both. To use magnitudes,
leave Antares' booleans unset.

The same applies to the legacy **`SpyEffect.UnitVeterancy=yes`**, which promotes
via the building's `Factory=` and sets the stock `BarracksInfiltrated` /
`WarFactoryInfiltrated` house flags. Those are read by the engine itself, so that
floor cannot be lowered either. Leave it unset if you want partial infantry or
vehicle levels.

Re-infiltrating the same building *type* does not stack with itself.

**Naval** is a spy-only branch. Naval units take the `Vehicle` academy bonus but
can carry an additional naval infiltration contribution, matching how Antares
treats `SpyEffect.NavalVeterancy`.

---

## Interaction notes

- **Preplaced objects get no bonus.** Objects placed by the map at scenario
  start are skipped, matching Antares. Bonuses apply to what you build.
- **Existing rank is never reduced.** A unit already promoted by other means
  keeps its rank.
- **Order does not matter.** Contributions are commutative; owning the same
  academies in a different order always gives the same result.
