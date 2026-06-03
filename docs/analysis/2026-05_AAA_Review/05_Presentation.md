# 05 — Presentation: UI/UX, FX/Feel, Áudio

> Parte da [AAA Technical Review (Maio 2026)](00_Index.md).
> Cobre: HUD, combat text, widgets core, menus, screen FX, camera shake, música
> adaptativa. (Hit stop detalhado em [02](02_Combat_GAS.md) §3.)

---

## 1. Arquitetura de HUD

Três camadas: `ADFHUDBase` (cria `MainHUDWidgetClass`, Z=0, skip dedicated) →
`ADFRunHUD` (cria widgets separados com Z-order: minimap/status 1, boss 2,
lock-on 3, counters 4) → `UDFInGameHUDWidget` (composição BindWidget, fade por
`State_InCombat`). Binding GAS via `UDFUserWidgetBase::BindToAttributeChanges` com
cleanup automático.

### Gaps vs AAA (Diablo 4 / GoW / Hades)
| Gap | Detalhe | Ref |
|---|---|---|
| **Grafo de viewport fragmentado** | 6+ roots independentes, sem layout controller único | `ADFRunHUD.cpp:55-91` |
| 🟡 **CommonUI listado mas ocioso** | `CommonUI`/`CommonInput` em deps, mas nenhum widget herda `UCommonActivatableWidget`; menus usam `UButton` cru + focus manual | `DungeonForged.Build.cs:41-42` |
| **Lock-on widget duplicado** | HUD cria `WBP_LockOnIndicator` E `UDFLockOnComponent` cria o seu (Z=100) | `ADFRunHUD.cpp:88` / `UDFLockOnComponent.cpp:392-414` |
| **HUD root duplicado** | `MainHUDWidget` (base) vs `WBP_HUD` (run) | — |
| **Adaptividade limitada** | fade por combate só; sem layout boss/elite/lowHP | `UDFInGameHUDWidget.cpp:82-86` |
| **Sem safe-zone/scale policy** | só `ApplicationScale` global | — |

---

## 2. Combat text (`UDFCombatTextSubsystem`)

**Forças:** pool real (30, com overflow + cleanup), 17 tipos (inclui reações
elementais), projeção world→screen com follow 60Hz, respeita `bShowDamageNumbers`,
dispatch central evita texto duplicado, abreviação k/M, crit shake/scatter.

**Gaps:** só screen-space (sem decals de chão p/ DoT/AoE); cores/fontes
hardcoded em C++ (não data-driven/themeable); sem merge/stacking (cada hit é um
floater → exaustão de pool); N timers a 60Hz sob AoE pesado; sem iconografia de
tipo de dano; flag italic não usada.

---

## 3. Widgets core

| Widget | Forças | Gaps principais |
|---|---|---|
| **Vitals** (`UDFPlayerVitalsWidget`) | bind GAS H/M/S, bar+orb, retry 0.25s | `SetPercent` instantâneo (sem damage-lag/heal pulse/shield); sem pulse lowHP no widget; vitals podem duplicar com hotbar |
| **Hotbar** (`UDFAbilityHotbarWidget`/`SlotWidget`) | 12 slots, cooldown overlay (MID+numérico), drag-drop com RPC | poll 0.25s (não event-driven); sem charge stacks/cost/proc glow/ready SFX; sem glyph de rebind |
| **Lock-on reticle** (`UDFLockOnWidget`) | projeta target+Z | widget trivial (17 linhas), sem estado weak-point/elite/boss; dual-creation |
| **Minimap** (`UDFMinimapWidget`+capture+fog) | scene capture ortho, fog por overlap, lookahead, expand anim, color por tipo | `UpdateMinimapTexture()` **toda tick**; RT 256² baixo; sem ping de inimigo/player; icons sem pool |
| **Boss bar** (`UDFBossHealthBarWidget`) | bind ASC, phase text, enrage icon | barra single-fill (sem segmentos de fase, sem delayed-damage chunk); sem intro/outro anim; dual code path |
| **Status bars** (`UDFStatusEffectBarWidget`) | icon pooling, rows buff/debuff, sort, bounce | rebuild limpa/re-add filhos a cada mudança (flicker); sem stack count; high-contrast stub |
| **Damage direction** (`UDFDamageDirectionWidget`) | wired do player, fade por intensidade | **4-way cardinal** (não radial); pulse único (hits rápidos sobrescrevem) |

---

## 4. Menus & overlays

- **Class selection**: 3 modos de preview (SceneCapture→RT, pawn em frente,
  showcase camera), slow-mo 0.3×, drag-rotate, co-op lock-in, tooltip flutuante.
  *Gaps:* rebuild da lista inteira a cada `RefreshAll`; focus manual (sem CommonUI);
  iluminação básica; sem compare-to-current-build.
- **Shop** (`UDFShopWidget`): grid de estoque, gold/reroll, anim de compra.
  *Gaps:* recria slots a cada open (sem pool); grid 3-col fixo; sem sell/compare.
- **Character screen**: paper-doll via `ADFEquipmentPreviewActor`+RT, orbit.
  *Gaps:* só scaffolding C++; sem stat-diff/set-bonus nativo.
- **Tooltips**: item compare com ▲▼. *Gaps:* usa nomes de atributo crus (não
  localizados); sem parse de affix; sem pin/compare-lock.
- **Options** (`UDFOptionsScreenWidget`): tabs Audio/Graphics/Controls/
  Accessibility/Language, sliders shake/hit-stop/damage-numbers, color-blind MID.
  *Gaps:* **`ApplyAudioVolumes` só seta master** (Music/SFX/Voice ignorados);
  `ApplyHighContrast` no-op; `PropagateToPlayerPawns` vazio.

---

## 5. Screen FX (`UDFScreenEffectsComponent`)

**Triggers (amplos):** damage (flash vermelho + shake se >30% + chroma), heal,
berserk (vignette+grain+FOV), death (hit-stop + dessaturação 2s), low-health
(pulse <25%), teleport, bands de combate, dodge, kill/room-clear spectacle.
Escala de acessibilidade `DF_VfxScaleFromWorld` em spectacle/dodge/teleport.

**Gaps:**
- 🟢 **`SetBlurAmount` declarado, nunca implementado** (`.h:88`).
- Vários writers de vignette competem na mesma tick (lowHP, berserk, hit,
  room-clear) — falta priority/blend stack.
- Manipula FOV da câmera de gameplay (pode conflitar com camera component).
- Flash de dano não escala por acessibilidade.

---

## 6. Camera shake (`UDFCameraShakes`)

5 shakes `ULegacyCameraShake` (LightHit 0.2s → Explosion 0.8s) com escala de
acessibilidade `DF_ShakeScale`. Só Explosion tem falloff por distância (radial).

**Gaps:** stack legacy (não `UCameraShakeBase` modular UE5); sem perfis por arma;
impactos melee ignoram distância ao player; sem direção do hit normal; sem
camada de háptico.

---

## 7. Áudio

### Música adaptativa (`UDFMusicManagerSubsystem`)
7 estados (MainMenu/Exploration/Combat/Elite/Boss/Victory/Death), 3 layers
(Base/Combat/Boss) em `ADFMusicLayerHost`, crossfade por lerp (~2s), stings
(victory 8s, death one-shot). Combat layer modula por contagem de inimigos.

### `UDFAudioComponent`
3D ability/footstep/impact com attenuation + concurrency; `Intensity` em MetaSound;
volume de impacto mapeado por força; `PlayDFUISound`.

### Gaps
| Gap | Detalhe | Ref |
|---|---|---|
| 🟡 **Sem SoundClass/Submix** | zero referências de submix no projeto — sem compressão de bus, sem LPF sob menu | — |
| 🟡 **Sliders Music/SFX/Voice não roteados** | `ApplyAudioVolumes` só master | `UDFAccessibilitySubsystem.cpp:81-83` |
| **`CrossfadeDuration` não usado** | lerp hardcoded speed 5 | `:359` |
| **Estado Elite sem branch** | enum existe, Combat e Elite compartilham case | `:250-262` |
| **Impact SFX bypassa concurrency** | `UDFCombatFeedbackLibrary` usa `PlaySoundAtLocation` cru | `:116-118` |
| **Sem density adaptativa de sting** | só victory/death; sem kill-streak/lowHP | — |

---

## 8. Coesão de juice & acessibilidade de apresentação

O dispatcher central (`DispatchOnHitConfirmed`) **dispara** as camadas juntas mas
**não as orquestra** — sem ID de "momento de apresentação" compartilhado, sem
beat-sync, sem háptico, e o impact SFX foge do mix do audio component.

**Acessibilidade (apresentação):** high-contrast stub; font-scale só global;
color-blind só blendable de câmera (cores de HUD hardcoded em combat text/status);
reduce-motion escala VFX mas **não** anims de HUD (gold pulse, status bounce);
sem narração/screen-reader.

---

## 9. Recomendações priorizadas — Presentation

| # | Recomendação | Tag | Esforço |
|---|---|---|---|
| 1 | **Unificar HUD sob root único** (adotar CommonUI ou `UDFRunHUDRootWidget`) — matar lock-on/HUD duplicados | 🟡 | M |
| 2 | **Mix de áudio** — SoundClasses/submixes + wirar sliders Music/SFX/Voice + roteаr impact pelo audio component | 🟡 | M |
| 3 | **Orquestrador de "momento de apresentação"** — uma struct coordenando hit-stop/shake/screen-FX/SFX/combat-text | 🟡 | M |
| 4 | **Juice de vitals** — damage-lag bar, heal shimmer, pulse lowHP sincronizado com screen FX | 🟡 | M |
| 5 | **Boss bar com fases** — barra segmentada + delayed-damage chunk + intro/outro anim | 🟡 | M |
| 6 | **Damage indicator radial** (360°) substituindo 4-way | 🟢 | M |
| 7 | **Combat text 2.0** — batch de projeção, merge/stack, estilos data-driven, decals de chão p/ DoT | 🟢 | M |
| 8 | **Minimap perf** — parar rebind de RT por tick; pool de icons; RT maior só no expand; pings | 🟢 | M |
| 9 | **Migrar camera shakes p/ `UCameraShakeBase`** + perfis por arma + falloff em melee | 🟢 | M |
| 10 | **Estado Elite de música** + density adaptativa de sting | 🟢 | L |
| 11 | **Acessibilidade completa** — high-contrast theme, cores de HUD safe, implementar/remover `SetBlurAmount`, reduce-motion em anims de HUD | 🟢 | M |
| 12 | **Hotbar event-driven** (sem poll 0.25s) + charge stacks/proc glow/glyph de rebind | 🟢 | M |

> Arquivos-chave: `Source/DungeonForged/Public/UI/ADFHUDBase.h`,
> `UDFInGameHUDWidget.h`, `GameModes/Run/ADFRunHUD.h`,
> `UI/Combat/UDFCombatTextSubsystem.h`, `UI/UDFAbilityHotbarWidget.h`,
> `UI/UDFBossHealthBarWidget.h`, `UI/Combat/UDFDamageDirectionWidget.h`,
> `UI/Minimap/UDFMinimapWidget.h`, `FX/UDFScreenEffectsComponent.h`,
> `FX/UDFCameraShakes.h`, `Audio/UDFMusicManagerSubsystem.h`,
> `Localization/UDFOptionsScreenWidget.h`.
