# UE Project Context

*Last updated: 2026-04-22*

## Engine & Project Overview
**Engine version:** UE 5.4 (Launcher build — *confirm if using source build*)
**Project name:** DungeonForged
**Description:** WoW-style third-person ARPG roguelike / dungeon crawler with data-driven design.
**Project type:** Game
**Genre / domain:** ARPG, roguelike, dungeon crawler
**Target platforms:**
- Windows (primary; *document others when you lock them*)

## Module Structure
**Primary game module:** `DungeonForged`

| Module         | Type    | Notes                                      |
|----------------|---------|---------------------------------------------|
| DungeonForged  | Runtime | Single game module; `IMPLEMENT_PRIMARY_GAME_MODULE` |

**Key dependencies per module (current `DungeonForged.Build.cs`):**
- **DungeonForged** — PublicDeps: `Core`, `CoreUObject`, `Engine`, `InputCore`, `EnhancedInput`

**Intended for GAS and gameplay (add when implemented):** `GameplayAbilities`, `GameplayTags`, and any modules required by your ASC/AttributeSet setup (e.g. `UMG` / `Slate` for UI tie-ins). *Not yet declared in Build.cs — add alongside first GAS/attributes code.*

## Plugin Dependencies
**Engine plugins (from `DungeonForged.uproject`):**
- **GameplayAbilities** — GAS; ASC on `PlayerState` and enemy base per project rules
- **ModelingToolsEditorMode** — Editor only (`TargetAllowList`: Editor)

**Marketplace / Fab plugins:** *None in repo — add as you integrate assets.*

**Custom plugins:** *No `Plugins/` directory yet.*

## Coding Conventions
**UObject / Actor naming:** `UDF` for `UObject` / component-style classes, `ADF` for `AActor` (project-specific; standard UE `F` / `A` / `U` / `E` / `I` still apply inside macro usage).
**Data:** All **DataTable** row structs inherit **`FTableRowBase`**.
**Input:** **Enhanced Input only** — no legacy `PlayerInput` / axis bindings for gameplay.
**Gameplay code:** **No Blueprint gameplay logic** — Blueprints may call C++ only.
**Design:** **Composition over inheritance** where practical.
**State / abilities:** **GameplayTags** for state and ability identification.
**GAS placement:** `UAbilitySystemComponent` on **PlayerState** and on **Enemy base** class (per design).
**Engine API:** **UE 5.4** — avoid deprecated APIs; no Blueprint-driven gameplay.

**Log categories, assertions, `TObjectPtr` policy:** *Not yet established in codebase — use project-wide categories once defined (e.g. `LogDungeonForged`).*

**Header style:** `#pragma once` (Epic default in new files).

**File layout (mandatory for new C++):**
- Headers: `Source/DungeonForged/Public/<Subsystem>/ClassName.h`
- Sources: `Source/DungeonForged/Private/<Subsystem>/ClassName.cpp`
- **Subsystem folders:** `Characters`, `GAS`, `Input`, `Dungeon`, `Items`, `UI`, `Camera`, `Combat`
- Top-of-file path comment, e.g. `// Source/DungeonForged/Public/Characters/DFPlayerCharacter.h`

## Subsystems in Use
**From `Config/DefaultEngine.ini` (class redirects / defaults):**
- `GlobalDefaultGameMode` points to `DungeonForgedGameMode` (`/Script/DungeonForged.DungeonForgedGameMode`)
- Class redirects: `TP_ThirdPersonGameMode` → `DungeonForgedGameMode`, `TP_ThirdPersonCharacter` → `DungeonForgedCharacter`

**Gameplay framework (target architecture — class names to align in C++):**
| Role            | Class name (intended)     |
|-----------------|---------------------------|
| GameMode        | `ADungeonForgedGameMode`  |
| Default Pawn/Character (template) | `ADungeonForgedCharacter` |
| GAS on player   | ASC on `PlayerState` (custom `APlayerState` / `UAbilitySystemComponent` as you implement) |
| GAS on enemies  | ASC on enemy base         |

**Subsystems (UGS / UWorld / ULocal):** *Not yet listed in project — add table rows as you add `UGameInstanceSubsystem` / `UWorldSubsystem` classes.*

**GAS usage (intended):**
- Full C++ GAS: abilities, effects, attribute sets, tags
- key classes/tags: TBD; reference `Config/DefaultGameplayTags.ini` when present

**Current source tree note:** The module currently only exposes `DungeonForged.h` / `DungeonForged.cpp` at the module root. New gameplay code should use the `Public/<Subsystem>` / `Private/<Subsystem>` layout above and extend `Build.cs` as needed for IWYU and dependencies.

## Build Configuration
**Build targets:** `Game` (`DungeonForged.Target.cs`), `Editor` (`DungeonForgedEditor.Target.cs`); no dedicated Server/Client targets in repo.
**Build settings:** `BuildSettingsVersion.V5`, `EngineIncludeOrderVersion.Unreal5_4`
**Custom macros / build flags:** *None documented.*
**Third-party libraries:** *None in Build.cs.*
**Platform-specific notes:** `DefaultEngine.ini` configures Windows (DX12/SM6), Linux Vulkan SM6, Android file server (editor), hardware targeting desktop/maximum.
**Engine modifications:** None

## Team Context
*Optional — not specified. For solo: Git (see `.gitignore`); document branching/review if the team grows.*

---

## AI / Generator Rules (short reference)
- Prefix: **ADF** (actors), **UDF** (UObjects, components)
- **DataTables:** `FTableRowBase` rows; **PrimaryDataAssets** for data-driven content where used
- **GAS** on **PlayerState** and **Enemy base**; tags for identification
- **Enhanced Input** exclusively for player actions
- **UE 5.4**, no deprecated APIs, Blueprints do not own gameplay flow

All other UE skills in `.cursor/skills` should use this file for engine version, module names, and project rules.
