# DungeonForged — Animation Blueprint authoring (UE 5.4)

C++ support classes live under `Animation/`: `UUDFAnimInstance`, `UUDFAnimInstance_Enemy`, and anim notifies. This document is the spec for the **ABP_Player** and blend spaces the Blueprint should mirror.

## Blend space: `BS_Locomotion_8Way`

- **Type:** 2D `UBlendSpace` (or `AimOffset`-style 2D sampling if you prefer a custom asset; standard is 2D Blend Space).
- **X axis (horizontal):** `Direction` = **-180 to 180** (degrees, strafe angle relative to facing). Drives: `UUDFAnimInstance::Direction` (from `CalculateDirection` or equivalent).
- **Y axis (vertical):** `Speed` = **0 to 700** (cm/s, horizontal). Drives: `UUDFAnimInstance::Speed`.
- **When used:** `bShouldStrafe == true` (lock-on / in-combat strafe — from `UUDFAnimInstance::bShouldStrafe`).

**Sample grid (X = Direction, Y = Speed, animation goal):**

| X (dir) | Y (speed) | Sample label |
|--------:|----------:|-------------|
| 0 | 0 | Idle |
| 0 | 400 | Walk_Fwd |
| 0 | 700 | Run_Fwd |
| 180 | 400 | Walk_Bwd |
| 180 | 700 | Run_Bwd |
| 90 | 400 | Walk_Right |
| 90 | 700 | Run_Right |
| -90 | 400 | Walk_Left |
| -90 | 700 | Run_Left |

Interpolate diagonals from neighbors; for cleaner corners add samples at 45° if your pipeline uses 8 directions at walk/run.

## Blend space: `BS_Locomotion_Standard`

- **Type:** 1D `UBlendSpace1D` (or 2D with second axis fixed — prefer **1D**).
- **X axis only:** `Speed` from **0 to 700** (no strafe angle; character faces input/motion in third-person as usual).
- **When used:** `bShouldStrafe == false` (free move — not strafing around a target).

## ABP_Player — state machines (logical layout)

### Base layer (full body)

**States (sequential / transitions):** `Idle` → `Locomotion` → `InAir` → `Land`

- **Idle**  
  - Plays idle loop.  
  - **To Locomotion:** `Speed > 10` (use `UUDFAnimInstance::Speed` or `FMath::IsNearlyZero` inverse).

- **Locomotion**  
  - If `bShouldStrafe` → use **`BS_Locomotion_8Way`** with `Direction` + `Speed`.  
  - Else → use **`BS_Locomotion_Standard`** with `Speed` only.  
  - `MovementDirection` / `bShouldStrafe` come from the anim instance (GAS + targeting).

- **InAir**  
  - When `!IsGrounded` (e.g. `bIsInAir` on anim instance, or CMC `IsFalling()`).  
  - Blend a fall/loop; optional second axis: vertical velocity or `GroundDistance` for land anticipation.

- **Land**  
  - When grounded after air; can fire a short land **montage** and blend out.  
  - `GroundDistance` and/or a land notify can tighten timing.

**Implementation in Editor:** one **State Machine** (e.g. `LocoSM`) with the above states; set blend times (e.g. 0.2s idle↔move, 0.1s to air).

### Upper body (layered blend per bone)

- **Layering:** `Layered blend per bone`, base = root→`spine_01` from base layer, blend above `spine_01` (or your rig’s “upper” split).

**States (upper SM):** `None` → `Aiming` → `Casting` → `Dead`

- **Aiming**  
  - Use an **Aim Offset** (or 2D Look node) driven by `AimPitch` / `AimYaw` from `UUDFAnimInstance`.

- **Casting**  
  - When `bIsCasting` (GAS `State.Casting`); play cast/upper body idle or a montage slot.

- **Dead**  
  - When `bIsDead`; optionally full-body override or switch base layer to ragdoll in **Blueprint** (not in this C++). Upper layer can be bypassed if ragdoll owns the mesh.

- **Attacking**  
  - Often implemented as **Slot** (default slot / upper body) with montages; `bIsAttacking` / `State.Attacking` gates transitions.

**Tags / booleans to drive transitions:** `bIsStunned`, `bIsAttacking`, `bIsCasting`, `bIsDead`, and `AimPitch` / `AimYaw` for aim offset.

## Enemy ABP

Use **`UUDFAnimInstance_Enemy`** for simpler state: drive `Speed`, `bIsInCombat`, `bIsDead`, `bIsStunned`, and set `HitReactionDirection` (or `HitFromWorldLocation`) from combat; play `SelectHitMontage(...)` to pick **Front/Back/Left/Right** reaction montages.

## Root motion and notifies

- **`UUDFAnimNotify_EnableRootMotion` / `UUDFAnimNotify_DisableRootMotion`:** call into `UUDFAnimInstance` push/pop of movement mode. Implement **`MOVE_Custom` / `PhysCustom`** on your `UCharacterMovementComponent` subclass (e.g. `UDFCharacterMovementComponent`) to consume root motion as needed.
- **Footsteps / trails:** `UUDFAnimNotify_FootStep`, `UUDFAnimNotify_SpawnTrailVFX`, `UUDFAnimNotify_DisableTrailVFX` — place on walk/run and attack montages as designed.
