# AcademyExt

A standalone [Syringe](https://github.com/Ares-Developers/Syringe) DLL for
Red Alert 2: Yuri's Revenge that makes veterancy bonuses **composable**.

Vanilla academies follow a fixed rule: *"if a player owns more than one academy
building, the highest bonus is awarded — the effect does not stack."*
AcademyExt replaces that with one configurable model covering academy buildings,
spy/infiltration effects, and a new passive country bonus.

```
result = max( best non-stacking contribution, sum of stacking contributions )
```

So with three academies — A (non-stacking), B and C (stacking) — the game awards
`max(A, B+C)`: whichever is actually better, the standalone bonus or the combined
stack.

## Features

| | |
|---|---|
| **Optional academy stacking** | `Academy.Stacks=yes` on any academy building |
| **Spy veterancy magnitudes** | `SpyEffect.*Veterancy.Level=` — veteran, elite, or partial, instead of a hardcoded veteran |
| **Passive country bonus** | `AcademyBonus=` under a country, stackable with every other source |

See [docs/INI-REFERENCE.md](docs/INI-REFERENCE.md) for the full tag list and
[DESIGN.md](DESIGN.md) for the architecture and rationale.

## Requirements

- **Antares** — AcademyExt is designed to co-load alongside it. It never edits or
  replaces Antares.
- Do **not** use Ares. Antares is the supported framework.

## Status

**Feature-complete.** Academy stacking, the passive country bonus and
configurable spy veterancy levels are all implemented and hooked. CI is green.

### All five features verified in game

| Feature | Result |
|---|---|
| **Stacking** | `0.5` academy: 1 building → no chevron, 2 → veteran, 4 → elite |
| **Non-stacking vs the stack** | `1.0` academy + 1 stacking building → veteran (it competed); + 4 → elite (the stack overtook it) |
| **Passive country bonus** | stacking `0.5` shifts the whole curve by one building — veteran at 1 / elite at 3, vs 2 and 4 without it |
| **Spy veterancy levels** | non-stacking `2.0` on infiltration → `bestSingle` 1.0→2.0, `stackSum` untouched → elite; 12 infiltrations, 12 recorded |
| **Authoritative mode + caps** | `resolved=2.000, cap=1.000 → wrote 1.000` — a rank genuinely **lowered** |

Antares reads the same `Academy.*Veterancy` tags and takes the `max`, so it
resolved `0.5` throughout the stacking rows and cannot account for any of these.

> **Caveat on the last row:** the reduction was *invisible on screen* — `1.5` and
> `1.0` both render as one chevron. It is only demonstrable from the decision log
> (`AcademyExt.Debug=yes`). Any reducing effect here needs one.

## Building

Windows only, MSVC v142 toolset, x86. CI builds on every push to `master`.

```
msbuild AcademyExt.sln /p:Configuration=DevBuild /p:Platform=x86
```

Submodules are pinned to known-good commits; clone with
`git clone --recursive`.

## Licence

Follows the licensing of the YRpp / Phobos utility code it links against.
