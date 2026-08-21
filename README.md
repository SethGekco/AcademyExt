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

**Scaffold.** The build, the extension data model and the resolution engine are
in place. Hooks are not yet wired — see the phase list in
[DESIGN.md](DESIGN.md#8-phases).

## Building

Windows only, MSVC v142 toolset, x86. CI builds on every push to `master`.

```
msbuild AcademyExt.sln /p:Configuration=DevBuild /p:Platform=x86
```

Submodules are pinned to known-good commits; clone with
`git clone --recursive`.

## Licence

Follows the licensing of the YRpp / Phobos utility code it links against.
