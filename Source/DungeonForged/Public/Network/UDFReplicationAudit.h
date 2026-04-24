// Source/DungeonForged/Public/Network/UDFReplicationAudit.h
#pragma once

/**
 * @file UDFReplicationAudit.h
 * DungeonForged (UE 5.4) — **documentation-only** replication and GAS audit.
 * Keep this header in sync when adding `DOREPLIFETIME`, ASC replication modes, or RPCs.
 *
 * **Target:** single-player + optional co-op (2 players). Authority model: server-authoritative
 * GAS, owner-only for private player data where noted.
 *
 * ---------------------------------------------------------------------------
 * GAS replication modes (set on `UAbilitySystemComponent` at construction / PostInit)
 * ---------------------------------------------------------------------------
 * | Actor / class        | Mode    | Rationale |
 * |----------------------|---------|-----------|
 * | `ADFPlayerState`     | `Mixed` | Player ASC on PlayerState: owner sees full active-effect
 * |                      |         | data for UI; other connections get minimal GAS rep. |
 * | `ADFEnemyBase`       |`Minimal`| AI enemies: tag-driven FX/UI; minimal GE payload to sim proxies. |
 * | `ADFBossBase`        | `Full`  | Phased boss: all clients need consistent GE/phase state for
 * |                      |         | VFX, phase transitions, and co-op telegraphing. (Overrides enemy.) |
 *
 * `ADFPlayerState::AbilitySystemComponent` and `UDFAttributeSet` are **not** listed with
 * `DOREPLIFETIME` on the PlayerState: the ASC is created with `SetIsReplicated(true)`; GAS
 * replicates the component and `UAttributeSet` registration through the standard ASC pipeline.
 * Document here as “implicit / GAS” rather than a raw property rep on `APlayerState`.
 *
 * ---------------------------------------------------------------------------
 * Declared `DOREPLIFETIME` (representative; see each class’s `.cpp` for the source of truth)
 * ---------------------------------------------------------------------------
 * `ADFPlayerCharacter`
 * - `DOREPLIFETIME(ADFPlayerCharacter, CurrentAbilitySlots)` — co-op: party UI / nameplates
 *   can show equipped ability loadout; server-authoritative updates only.
 *
 * `ADFPlayerState`
 * - `DOREPLIFETIME(ADFPlayerState, ReplicatedRunGold)` — run gold for HUD; see `OnRep_ReplicatedRunGold`.
 * - (GAS) `UAbilitySystemComponent` + `UDFAttributeSet` — implicit via ASC + attribute replication.
 * - (Component) `UDFLevelingComponent` on the same `Outer` replicates with owner-only or shared rules
 *   (see `UDFLevelingComponent` below). Not `DOREPLIFETIME(ADFPlayerState, LevelingComponent)`.
 *
 * `UDFLevelingComponent` (on `ADFPlayerState`)
 * - `DOREPLIFETIME_CONDITION(UDFLevelingComponent, CurrentXP, COND_OwnerOnly)`
 * - `DOREPLIFETIME_CONDITION(UDFLevelingComponent, CurrentLevel, COND_OwnerOnly)`
 * - `DOREPLIFETIME_CONDITION(UDFLevelingComponent, UnspentAttributePoints, COND_OwnerOnly)`
 *
 * `UDFInventoryComponent` (Pawn and/or `ADFPlayerState` in this project)
 * - `DOREPLIFETIME_CONDITION(UDFInventoryComponent, MaxSlots, COND_OwnerOnly)` (tuned for co-op)
 * - `DOREPLIFETIME_CONDITION(UDFInventoryComponent, ItemDataTable, COND_OwnerOnly)`
 * - `DOREPLIFETIME_CONDITION(UDFInventoryComponent, Items, COND_OwnerOnly)`
 *
 * `ADFEnemyBase`
 * - `DOREPLIFETIME(ADFEnemyBase, bHasDied)` — all clients: death VFX, minimap, AI teardown visibility.
 *   (Project uses `bHasDied` as the server-set death flag; “bIsDead” in design docs = this.)
 *
 * `ADFBossBase` (subclass of `ADFEnemyBase`)
 * - `DOREPLIFETIME(ADFBossBase, CurrentPhase)`
 * - `DOREPLIFETIME(ADFBossBase, bIsEnraged)`
 * - GAS: ASC replication mode `Full` (see constructor).
 *
 * `UDFDungeonManager` (**`UGameInstanceSubsystem`**, not an `AActor`)
 * - `CurrentFloor`, `EnemiesRemaining`, and related fields are **not** network-replicated by the engine
 *   for `UGameInstanceSubsystem` instances. They are **server-simulated** in this project (`IsAuthorityWorld`
 *   checks in `ADFDungeonManager` implementation). For full co-op UI sync, mirror critical scalars
 *   onto a replicated `AGameState` / `APlayerState` or use targeted Client RPCs in a future pass.
 * - This audit documents **intent**: treat floor/enemy count as “authoritative on server, local trust in SP”.
 *
 * ---------------------------------------------------------------------------
 * Other replicated types (not exhaustive; search `GetLifetimeReplicatedProps` in the module)
 * ---------------------------------------------------------------------------
 * - `UDFAttributeSet` — attribute replication with `DOREPLIFETIME_CONDITION_NOTIFY`.
 * - `UDFEquipmentComponent` — `ReplicatedLoadout`.
 * - `UDFLootDrop`, `ADFInteractableBase`, `ADFChest`, `ADFDoor`, `ADFTrapBase`, `ADFMerchantActor`, etc.
 *
 * ---------------------------------------------------------------------------
 * RPC index (see `ADFPlayerController`, `ADFPlayerState`, pawns, merchant)
 * ---------------------------------------------------------------------------
 * - `ADFPlayerState`: `Client_OpenAbilitySelectionScreen`, `Client_ResumeAfterAbilitySelection`,
 *   `Server_FinishAbilitySelection`, `Server_ExecuteRandomEventChoice`, …
 * - `ADFPlayerController`: `Server_*` / `Client_*` / `Multicast_*` **patterns** in `UDFNetworkLibrary` docs
 *   and `ADFPlayerController` declarations (BFL cannot own RPCs; wrappers call the controller).
 * - Shop / combat: `ADFPlayerCharacter::ServerMerchantPurchase`, `ServerMerchantReroll`, `Client_HitFeedback`, …
 */
