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
- **DungeonForged** — PublicDeps: `Core`, `CoreUObject`, `Engine`, `InputCore`, `NetCore`, `GameplayAbilities`, `GameplayTags`, `GameplayTasks`, `EnhancedInput`, `UMG`, `Slate`, `SlateCore`, `CommonUI`, `CommonInput`, `AIModule`, `NavigationSystem`, `PCG`, `Niagara`. Editor-only private: `UnrealEd`, `PropertyEditor`, `ToolMenus`, `EditorStyle`, `UMGEditor` (when `Target.bBuildEditor`).

## Plugin Dependencies
**Engine plugins (from `DungeonForged.uproject`):**
- **GameplayAbilities** — GAS
- **CommonUI** — UI stack
- **PCG** — procedural content (dungeon manager)
- **Niagara** — VFX
- **ModelingToolsEditorMode** — Editor only (`TargetAllowList`: Editor)

**Built-in (not listed in `.uproject`):** **Enhanced Input** — enabled with the engine for UE5.

**Marketplace / Fab plugins:** *None in repo — add as you integrate assets.*

**Custom plugins:** *No project `Plugins/` directory in repo.*

## Coding Conventions
**UObject / Actor naming:** `UDF` for `UObject` / component-style classes, `ADF` for `AActor` (project-specific; standard UE `F` / `A` / `U` / `E` / `I` still apply inside macro usage).
**Data:** **DataTable** row structs inherit **`FTableRowBase`**; plain `USTRUCT`s used only inside components/arrays (not as table rows) may omit it.
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

**Gameplay framework (implemented C++):**
| Role | Class |
|------|--------|
| Game instance | `UDungeonForgedGameInstance` — GAS `InitGlobalData` + native tag registration |
| Player state / GAS | `ADFPlayerState` — `UAbilitySystemComponent` + `UDFAttributeSet`, replication mode **Mixed** |
| Player character | `ADFPlayerCharacter` — `IAbilitySystemInterface`, `InitAbilityActorInfo` in `PossessedBy` / `OnRep_PlayerState` |
| Enemy / GAS | `ADFEnemyBase` — ASC on pawn, replication mode **Minimal** |
| Run / roguelike | `UDFRunManager` (`UGameInstanceSubsystem`) — run state, death/completion, offers |
| Dungeon | `UDFDungeonManager` (`UGameInstanceSubsystem`) — floors, spawns, PCG |

**GAS usage:**
- Attribute set: `UDFAttributeSet`; native tags: `FDFGameplayTags`; abilities extend `UDFGameplayAbility` / `UGameplayAbility`
- Globals: `UDFAbilitySystemGlobals` (see `DefaultGame.ini`)

**Data:** DataTable rows use `FTableRowBase` where the row is table-backed; runtime-only structs (e.g. inventory slots in arrays) may be plain `USTRUCT` without `FTableRowBase`.

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
