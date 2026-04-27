# Blueprints — Main Menu & Class Selection

Documentação passo a passo para criar e configurar os Blueprints da camada de
**fluxo de entrada** do DungeonForged (UE 5.4).

## Convenções

- Toda classe `WBP_*` é um **Widget Blueprint** que herda de uma classe C++
  já existente. **Você nunca cria a classe pai.** Apenas o asset Blueprint
  e a árvore de UMG dentro dele.
- Todo binding `meta=(BindWidget)` exige que o **nome do widget** no
  Designer seja **exatamente igual** ao nome da `UPROPERTY` em C++. O nome
  é case-sensitive.
- `meta=(BindWidgetOptional)` significa que o widget não é obrigatório —
  mas se você criar um com o nome correto, o C++ liga automaticamente.
- Animações `meta=(BindWidgetAnim)` seguem a mesma regra para o nome da
  Widget Animation.

## Índice

| Documento | Cobertura |
|---|---|
| [`MainMenu_Setup.md`](MainMenu_Setup.md) | `BP_FMainMenuGameMode`, `BP_DFMainMenuHUD`, `WBP_SplashScreen`, `WBP_MainMenu`, `WBP_MenuButton`, `WBP_Credits`, `WBP_ConfirmDialog`, `WBP_SaveSlotSelection`, `WBP_SaveSlotCard`, `WBP_SaveSlotCardEmpty` |
| [`ClassSelection_Setup.md`](ClassSelection_Setup.md) | `WBP_ClassSelection`, `WBP_ClassListEntry`, `WBP_StatBar`, `WBP_AbilityPreviewIcon`, `WBP_AbilityTooltip`, `WBP_ActiveChallengeIndicator`, `WBP_PartnerClassPreview` + configuração do `UDFClassSelectionSubsystem` |

## Mapa do fluxo

```
Launch
  └─> L_MainMenu (GameMode = BP_FMainMenuGameMode, HUD = BP_DFMainMenuHUD)
        ├─> WBP_SplashScreen        (3 fases: Epic, Studio, Title — fade in/hold/out)
        └─> WBP_MainMenu            (Continuar, Nova Aventura, Gerenciar, Opções, Conquistas, Créditos, Sair)
              ├─> WBP_Credits       (rolagem automática)
              ├─> WBP_ConfirmDialog (modal genérico)
              └─> WBP_SaveSlotSelection
                    └─> 3× WBP_SaveSlotCard (occupied) / WBP_SaveSlotCardEmpty (empty)
                          ├─> [Continuar/Play]   → UDFWorldTransitionSubsystem::TravelToNexus
                          ├─> [Nova Run]         → UDFClassSelectionSubsystem::OpenClassSelection (RunDungeon)
                          └─> [Criar / Empty]    → UDFClassSelectionSubsystem::OpenClassSelection (NexusFirstLaunch)
                                └─> WBP_ClassSelection
                                      ├─> Confirm (RunDungeon)        → TravelToRun(ClassName)
                                      └─> Confirm (NexusFirstLaunch)  → TravelToNexus(FirstLaunch)
```

## Fluxo recomendado de execução

1. Comece por **`MainMenu_Setup.md`**, pois ele é a porta de entrada do
   jogo e define os assets de referência (HUD, GameMode, Confirm Dialog).
2. Depois siga **`ClassSelection_Setup.md`** — o Save Slot Card aciona a
   tela de classes, e o Subsystem precisa de um `PreviewPawnClass` que só
   é configurado nesta etapa.

## Pré-requisitos do projeto

Antes de começar, garanta que:

- O módulo C++ `DungeonForged` compila (Live Coding ou rebuild). As
  classes pai abaixo precisam estar disponíveis no Content Browser:
  - `ADFMainMenuGameMode`, `ADFMainMenuHUD`
  - `UDFSplashScreenUserWidget`, `UDFMainMenuUserWidget`,
    `UDFCreditsUserWidget`, `UDFConfirmDialogUserWidget`,
    `UDFSaveSlotSelectionUserWidget`, `UDFSaveSlotCardUserWidget`,
    `UDFMenuButtonUserWidget`
  - `UDFClassSelectionWidget`, `UDFClassListEntryWidget`,
    `UDFClassStatBarWidget`, `UDFAbilityPreviewIconWidget`,
    `UDFClassAbilityTooltipWidget`,
    `UDFActiveChallengeIndicatorWidget`,
    `UDFPartnerClassPreviewWidget`
  - `UDFClassSelectionSubsystem` (UWorldSubsystem)
- `Content/DungeonForged/DataTables/DT_Class.uasset` está populada e
  referenciada por `UDFRunManager::ClassDataTable`.
- `BP_DFGameInstance` está como `GameInstanceClass` em
  `Config/DefaultEngine.ini` (já está).
- O mapa `Content/DungeonForged/Maps/L_MainMenu.umap` existe (já existe).

## Status atual de implementação (verificado em 2026-04-26)

| Item | Status | Observação |
|---|---|---|
| C++ `ADFMainMenuGameMode` / `ADFMainMenuHUD` | ✅ pronto | nada a alterar |
| C++ `UDFSplashScreenUserWidget` (timers, fade, skip) | ✅ pronto | — |
| C++ `UDFMainMenuUserWidget` (botões, refresh por slot) | ✅ pronto | `OnNewAdventureEmphasisChanged` é Blueprint Native — implementar no WBP se quiser brilho |
| C++ `UDFSaveSlotSelectionUserWidget` (3 cards, refresh) | ✅ pronto | suporta `SlotRow` panel **ou** `SlotCard0..2` direto |
| C++ `UDFSaveSlotCardUserWidget` (Empty/Occupied) | ✅ pronto | suporta `StateSwitcher` **ou** `EmptyRoot/OccupiedRoot` |
| C++ `UDFConfirmDialogUserWidget` (open anim, delegates) | ✅ pronto | — |
| C++ `UDFClassSelectionWidget` (3 colunas) | ✅ pronto | — |
| C++ `UDFClassSelectionSubsystem` (preview pawn, RT, slow-mo) | ✅ pronto | precisa `PreviewPawnClass` configurada |
| Asset `BP_FMainMenuGameMode` | ⚠️ existe mas **alterado em git**; reabrir e revalidar referência ao HUD |
| Asset `BP_DFMainMenuHU.uasset` (HUD) | ✅ existe | **renomear para `BP_DFMainMenuHUD`** se desejar consistência |
| Asset `WBP_SaveSlotSelection.uasset` | ⚠️ alterado, validar bindings |
| Asset `WBP_SaveSlotCard.uasset` | ⚠️ alterado, validar `StateSwitcher` |
| Asset `WBP_SaveSlotCardEmpty.uasset` | 🆕 novo (untracked) — usado na variante "vazio" |
| `Config/DefaultEngine.ini` `GlobalDefaultGameMode` | ⚠️ aponta para `DungeonForgedGameMode` — sobrescrever no **World Settings de L_MainMenu** com `BP_FMainMenuGameMode` |

> **Conclusão:** o C++ está completo. Os ajustes pendentes são todos
> de configuração de assets / WorldSettings — descritos passo a passo
> nos documentos individuais.
