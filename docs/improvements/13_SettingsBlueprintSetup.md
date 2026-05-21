# Settings / Options — Setup Blueprint / Editor

> Complementa [`MainMenu_Setup.md`](../blueprints/MainMenu_Setup.md) e [`08_Accessibility.md`](08_Accessibility.md).  
> **C++ pronto:** subsistemas + widgets base; **editor:** criar/ligar os WBP.

Legenda: **✅ C++ pronto** · **⚠️ configurar no editor** · **🟡 placeholder / parcial** · **❌ backlog**

---

## 1. Resposta rápida

| Pergunta | Resposta |
|----------|----------|
| Existe widget de settings? | ✅ `UDFOptionsScreenWidget` (parent do `WBP_OptionsScreen`) |
| Main Menu abre options? | ✅ botão `OptionsButton` → `ADFMainMenuHUD::OptionsWidgetClass` |
| Pause na Run abre options? | ✅ `UDFPauseMenuWidget` → botão `Options` → `OptionsScreenClass` |
| Settings persistem? | ✅ `UDFSaveGame` (volume, a11y, idioma, keybinds) |
| Graphics (resolução/qualidade)? | 🟡 aba existe no switcher; **sem lógica C++** ainda |
| Input de Pause (Esc)? | ⚠️ `ADFRunPlayerController::OnPause()` existe; **você liga** no IMC/BP |

**Um único `WBP_OptionsScreen`** pode ser reutilizado no Main Menu e no Pause.

---

## 2. Arquitetura

```
┌─────────────────────────────────────────────────────────────┐
│  UI (UMG)                                                    │
│  WBP_MainMenu ──Options──► WBP_OptionsScreen                 │
│  WBP_PauseMenu ─Options──► WBP_OptionsScreen (mesmo asset)   │
└────────────────────────────┬────────────────────────────────┘
                             │
┌────────────────────────────▼────────────────────────────────┐
│  UDFOptionsScreenWidget (C++)                                │
│  Tabs: Audio | Graphics* | Controls | Accessibility | Lang   │
└─────┬──────────────┬─────────────────┬──────────────────────┘
      │              │                 │
      ▼              ▼                 ▼
 UDFAccessibility   UDFInputRemapping  UDFLocalization
 Subsystem          Subsystem          Subsystem
      │              │                 │
      └──────────────┴─────────────────┘
                     │
                     ▼
              UDFSaveGame (slot meta)
```

\* Graphics = só UI vazia até implementar resolução/qualidade.

---

## 3. O que cada aba faz hoje

| Índice | Tab | C++ | Persiste |
|--------|-----|-----|----------|
| 0 | **Audio** | Sliders Master/Music/SFX/Voice → volume imediato | ✅ ao mover slider |
| 1 | **Graphics** | Só `ShowTabByIndex(1)` — **sem widgets ligados** | ❌ |
| 2 | **Controls** | Lista de keybinds + rebind + reset | ✅ via `SaveRemapping` |
| 3 | **Accessibility** | Font scale, contraste, motion, daltonismo, shake, hit-stop, números de dano | ✅ botão Apply ou live preview |
| 4 | **Language** | TileView 4 idiomas + preview | ✅ via `UDFLocalizationSubsystem` |

**Consumidores das settings em gameplay:** hit-stop, camera shake, combat text, screen FX leem `UDFAccessibilitySubsystem`.

---

## 4. Assets a criar

| Asset | Parent C++ | Pasta sugerida |
|-------|------------|----------------|
| `WBP_OptionsScreen` | `UDFOptionsScreenWidget` | `Content/DungeonForged/UI/Settings/` |
| `WBP_KeyBindRow` | `UDFKeyBindRowWidget` | idem |
| `WBP_PauseMenu` | `UDFPauseMenuWidget` | `Content/DungeonForged/UI/Run/` |

> Main Menu já documentado em [`MainMenu_Setup.md`](../blueprints/MainMenu_Setup.md) — falta só apontar `OptionsWidgetClass`.

---

## 5. `WBP_OptionsScreen`

### 5.1 Class Defaults (importante)

| Property | Onde | Valor |
|----------|------|-------|
| `KeyBindOrder` | DF \| Options \| Controls | Array de `FName` = **Mapping Name** do Enhanced Input (ver §8) |
| `KeyBindRowClass` | DF \| Options \| Controls | `WBP_KeyBindRow` |

### 5.2 Bindings obrigatórios (`BindWidgetOptional` — nomes **exatos**)

#### Shell / tabs

| Nome no Designer | Tipo | Função |
|------------------|------|--------|
| `MainTabSwitcher` | `WidgetSwitcher` | 5 filhos (Audio, Graphics, Controls, A11y, Language) |
| `Button_TabAudio` | `Button` | Tab 0 |
| `Button_TabGraphics` | `Button` | Tab 1 |
| `Button_TabControls` | `Button` | Tab 2 |
| `Button_TabAccessibility` | `Button` | Tab 3 |
| `Button_TabLanguage` | `Button` | Tab 4 |

Ordem dos filhos do `MainTabSwitcher` **deve** bater com os índices acima.

#### Audio (filho 0)

| Nome | Tipo |
|------|------|
| `Slider_Master` | `Slider` (0–1) |
| `Slider_Music` | `Slider` |
| `Slider_SFX` | `Slider` |
| `Slider_Voice` | `Slider` |

#### Controls (filho 2)

| Nome | Tipo |
|------|------|
| `Panel_KeyBinds` | `PanelWidget` (VerticalBox / ScrollBox) |
| `Button_ResetKeybinds` | `Button` |

#### Accessibility (filho 3)

| Nome | Tipo |
|------|------|
| `Slider_UIFont` | `Slider` (0–1 → escala 0.8–2.0) |
| `Check_HighContrast` | `CheckBox` |
| `Check_ReduceMotion` | `CheckBox` |
| `Check_ColorBlind` | `CheckBox` |
| `Combo_ColorBlind` | `ComboBoxString` |
| `Slider_CameraShakeIntensity` | `Slider` (0–1) |
| `Slider_HitStopIntensity` | `Slider` (0–1) |
| `Check_ShowDamageNumbers` | `CheckBox` |
| `Button_ApplyAccessibility` | `Button` (persiste tudo de a11y) |

#### Language (filho 4)

| Nome | Tipo |
|------|------|
| `LanguageTileView` | `TileView` |
| `Text_LanguagePreview` | `TextBlock` |

#### Graphics (filho 1) — placeholder

Crie um painel vazio com texto “Em breve” ou widgets de resolução **só visuais**. O C++ não lê nada dessa aba ainda.

### 5.3 Botão Voltar + índice da aba (C++)

| Nome | Tipo | Função |
|------|------|--------|
| `Button_Back` | `Button` | `CloseOptions()` — remove overlay + `RestoreMainMenuFocus` no Main Menu HUD |

**Ler aba atual no Blueprint / UMG:**

| API | Uso |
|-----|-----|
| `CurrentTabIndex` | propriedade `BlueprintReadOnly` (0=Audio … 4=Language) |
| `Get Current Tab Index` | mesmo valor |
| `Get Current Tab` | enum `EDFOptionsTab` |
| `Show Tab` | enum em vez de int |
| `Set Current Tab Index` | igual ao `Show Tab By Index` + opcional `Notify Even If Unchanged` |

Exemplo no Designer: binding de visibilidade/estilo do botão de tab → comparar com `CurrentTabIndex == 2` (Controls).

**Evento ao trocar aba (Designer / Event Graph):**

| Forma | Como usar |
|-------|-----------|
| `On Tab Changed` (multicast) | No `WBP_OptionsScreen` → **Graph** → **On Tab Changed** → atualizar estilo dos botões de tab |
| `On Tab Index Changed` | **Override Functions** no widget → highlight / SFX por aba |
| Parâmetros | `Tab Index` (int), `Tab` (`EDFOptionsTab`) |

Dispara em cada `Show Tab By Index` / clique nos `Button_Tab*` quando o índice muda, e uma vez no `Construct` com a aba inicial do switcher.

Marque o root como **Is Focusable = true** (padrão em widgets DF).

### 5.4 Layout sugerido

```
Canvas Panel (fullscreen overlay)
├─ Image (dark overlay α≈0.7)
├─ Border (painel central ~900×600)
│  ├─ HorizontalBox (tab buttons)
│  │  ├─ Button_TabAudio
│  │  ├─ Button_TabGraphics
│  │  ├─ Button_TabControls
│  │  ├─ Button_TabAccessibility
│  │  └─ Button_TabLanguage
│  └─ WidgetSwitcher MainTabSwitcher
│     ├─ [0] VerticalBox (sliders áudio)
│     ├─ [1] Text “Graphics — TODO”
│     ├─ [2] ScrollBox → Panel_KeyBinds + Button_ResetKeybinds
│     ├─ [3] ScrollBox (checks + sliders a11y + Apply)
│     └─ [4] LanguageTileView + Text_LanguagePreview
└─ Button_Back (canto)  ← BindWidget `Button_Back`
```

---

## 6. `WBP_KeyBindRow`

Parent: `UDFKeyBindRowWidget`

| Nome | Tipo | Obrigatório |
|------|------|-------------|
| `ActionLabel` | `TextBlock` | ✅ |
| `KeyButton` | `Button` | ✅ |
| `KeyNameText` | `TextBlock` | opcional (mostra tecla atual) |

Fluxo: clique em `KeyButton` → C++ entra em modo rebind → próxima tecla (Esc cancela) → atualiza linha.

---

## 7. Main Menu — wiring

### 7.1 `BP_DFMainMenuHUD` (Class Defaults)

| Property | Valor |
|----------|-------|
| `OptionsWidgetClass` | `WBP_OptionsScreen` |

### 7.2 `WBP_MainMenu`

| Nome | Tipo |
|------|------|
| `OptionsButton` | `Button` |

C++ em `OnOptions()` cria o widget e `AddToViewport(30)`.

### 7.3 Teste

PIE em `L_MainMenu` → **Opções** → sliders respondem → fechar → foco volta ao menu (`RestoreMainMenuFocus` se implementar no Back).

---

## 8. Run — Pause + Options

### 8.1 `BP_RunPlayerController` (ou filho de `ADFRunPlayerController`)

| Property | Categoria | Valor |
|----------|-----------|-------|
| `PauseMenuClass` | Run \| UI | `WBP_PauseMenu` |

> `OptionsScreenClass` no **PlayerController** existe no header mas **não é usado** — configure no **Pause Menu widget**.

### 8.2 `WBP_PauseMenu`

Parent: `UDFPauseMenuWidget`

| Nome | Tipo | Obrigatório |
|------|------|-------------|
| `Resume` | `Button` | ✅ |
| `Options` | `Button` | ✅ |
| `AbandonRun` | `Button` | ✅ |
| `RunStatsText` | `TextBlock` | opcional (andar/kills/ouro/tempo) |

**Class Defaults:**

| Property | Valor |
|----------|-------|
| `OptionsScreenClass` | `WBP_OptionsScreen` |
| `BlurBackgroundMaterial` | MID blur opcional |

Comportamento C++:

- **Resume** — remove pause menu, `SetGamePaused(false)`, input gameplay.
- **Options** — spawna `WBP_OptionsScreen` em ZOrder 25 (jogo continua pausado).
- **Abandon Run** — `RequestReturnToNexus(Abandon)`.

### 8.3 Ligar tecla Pause (⚠️ editor)

`OnPause()` no Run PC **não** tem `IA_Pause` no C++. Escolha uma opção:

**Opção A — Enhanced Input (recomendado)**

1. Crie `IA_Pause` (Digital bool).
2. Adicione ao IMC de gameplay com tecla **Escape** (e Start no gamepad).
3. No `BP_RunPlayerController` Event Graph:

```
Enhanced Input Action IA_Pause (Started)
  → Cast to ADFRunPlayerController → OnPause
```

**Opção B — Input legacy no widget pause**

No `WBP_PauseMenu`, override `OnKeyDown` → se Escape e widget visível → `HandleResume`.

### 8.4 Teste Pause

PIE na Run → Esc → pause menu → Opções → sliders → Voltar → Resume → gameplay.

---

## 9. Keybinds — `KeyBindOrder`

Cada entrada em `KeyBindOrder` deve ser o **Mapping Name** mappable do Enhanced Input (Project Settings → Enhanced Input → User Settings, ou no Input Action → Mappable Keys).

Exemplo típico (ajuste aos seus assets):

| KeyBindOrder (FName) | Ação |
|----------------------|------|
| `MoveForward` | W / stick |
| `MoveBackward` | S |
| `MoveLeft` | A |
| `MoveRight` | D |
| `Jump` | Space |
| `Attack` | LMB |
| `Dodge` | Space / Shift |
| `Sprint` | Shift |
| `Interact` | G |
| `Ability1` … `Ability4` | Q E R F |

**IMC:** em `UDFInputRemappingSubsystem`, set `CurrentIMC` no CDO ou deixe o options screen registrar o IMC do Run ao abrir (`RegisterInputMappingContextForLocalPlayer` já roda no `NativeConstruct`).

**Reset:** `Button_ResetKeybinds` → defaults do Enhanced Input User Settings.

---

## 10. Persistência (`UDFSaveGame`)

| Campo | Subsystem | Quando salva |
|-------|-----------|--------------|
| `AccessibilitySettings` | `UDFAccessibilitySubsystem` | Sliders áudio (live), Apply a11y, alguns toggles live |
| `SavedKeyBindings` | `UDFInputRemappingSubsystem` | Após cada `RemapKey` / Reset |
| `PreferredLanguage` | `UDFLocalizationSubsystem` | Ao escolher idioma no TileView |

Carrega automaticamente no `Initialize` dos subsystems (boot do jogo).

---

## 11. Extras opcionais no editor

### 11.1 Material daltonismo

No CDO de `UDFAccessibilitySubsystem` (ou via BP que seta no GameInstance):

- `ColorBlindPostProcessMaterial` = material de correção (Protan/Deutan/Tritan).

Sem material, o combo daltonismo salva mas **efeito visual depende do asset**.

### 11.2 High contrast

`ApplyHighContrast()` em C++ é stub — para efeito real, escute `OnAccessibilitySettingsChanged` no root UMG e troque paleta/brushes.

### 11.3 Z-order recomendado

| UI | ZOrder |
|----|--------|
| HUD gameplay | 0–10 |
| Pause menu | 20 |
| Options overlay | 25–30 |
| Confirm dialog | 100 |

---

## 12. Checklist

- [ ] `WBP_OptionsScreen` criado com todos os `BindWidget` da §5.2
- [ ] `MainTabSwitcher` com **5 filhos** na ordem correta
- [ ] `KeyBindOrder` + `WBP_KeyBindRow` configurados
- [ ] `Button_Back` no WBP (RemoveFromParent)
- [ ] `BP_DFMainMenuHUD` → `OptionsWidgetClass = WBP_OptionsScreen`
- [ ] `WBP_MainMenu` → `OptionsButton` bound
- [ ] `WBP_PauseMenu` → Resume/Options/Abandon + `OptionsScreenClass`
- [ ] `BP_RunPlayerController` → `PauseMenuClass` + input Esc → `OnPause`
- [ ] PIE: volumes persistem após restart
- [ ] PIE: rebind persiste após restart
- [ ] PIE: hit-stop / shake respondem aos sliders

---

## 13. Troubleshooting

| Sintoma | Causa | Fix |
|---------|-------|-----|
| Opções não abre no menu | `OptionsWidgetClass` vazio no HUD | §7.1 |
| Opções no pause não abre | `OptionsScreenClass` vazio no **Pause WBP** | §8.2 |
| Tabs não trocam | Nomes dos botões errados ou switcher sem 5 filhos | §5.2 |
| Keybind list vazia | `Panel_KeyBinds` não bound ou `KeyBindOrder` vazio | §5.2, §9 |
| Rebind não funciona | Mapping names ≠ Enhanced Input | §9 |
| Sliders não salvam | SaveGame slot inexistente | jogar uma vez / criar perfil |
| Esc não abre pause | Input não ligado a `OnPause` | §8.3 |
| Graphics não faz nada | Esperado — placeholder | §3 |

---

## 14. Referência C++

| Classe | Ficheiro |
|--------|----------|
| `UDFOptionsScreenWidget` | `Localization/UDFOptionsScreenWidget.h` |
| `UDFAccessibilitySubsystem` | `Localization/UDFAccessibilitySubsystem.h` |
| `UDFInputRemappingSubsystem` | `Localization/UDFInputRemappingSubsystem.h` |
| `UDFLocalizationSubsystem` | `Localization/UDFLocalizationSubsystem.h` |
| `UDFPauseMenuWidget` | `GameModes/Run/UDFPauseMenuWidget.h` |
| `UDFMainMenuUserWidget` | `GameModes/MainMenu/UDFMainMenuUserWidget.h` |
| `ADFMainMenuHUD` | `GameModes/MainMenu/ADFMainMenuHUD.h` |
| `ADFRunPlayerController` | `GameModes/Run/ADFRunPlayerController.h` |
| `FDFAccessibilitySettings` | `Localization/DFAccessibilityData.h` |
