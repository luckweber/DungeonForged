# Main Menu — Setup completo dos Blueprints

Documento prático para criar/configurar os assets do menu principal do
DungeonForged. Cobre os 9 Blueprints + GameMode + HUD + ajustes de
World Settings.

> **Mapa de referência:** `Content/DungeonForged/Maps/L_MainMenu.umap`
> **Pasta-alvo dos widgets:** `Content/DungeonForged/UI/MainMenu/`
> **Pasta-alvo do GameMode/HUD:** `Content/DungeonForged/Game/` e
> `Content/DungeonForged/UI/HUD/`

---

## 1. `BP_FMainMenuGameMode` (parent: `ADFMainMenuGameMode`)

Caminho: `Content/DungeonForged/Game/BP_FMainMenuGameMode.uasset` (já existe).

### Passo a passo

1. Content Browser → click direito no asset → **Edit** (ou duplo-click).
2. Aba **Class Defaults** (canto superior direito).
3. Categoria **DF | MainMenu | UI** → propriedade `MainMenuHUDClass`:
   - selecione **`BP_DFMainMenuHU`** (asset existente em
     `Content/DungeonForged/UI/HUD/`).
   - `HUDClass` herdada do `AGameModeBase` é copiada automaticamente em
     `InitGame` se você setar a propriedade acima — não precisa alterar
     `HUDClass` manualmente.
4. Categoria **DF | MainMenu | Cine** → `BackgroundLoopSequence`:
   - se você quiser câmera cinemática rodando atrás do menu, atribua
     uma `LevelSequence` aqui (a cinemática será tocada em loop pelo
     `UDFCinematicSubsystem`).
   - se ainda não tiver cinemática, deixe **None** — não há erro.
5. **Compile** + **Save**.

### Configurar o World Settings de `L_MainMenu`

> ⚠️ **Ajuste obrigatório.** O `Config/DefaultEngine.ini` define
> `GlobalDefaultGameMode = /Script/DungeonForged.DungeonForgedGameMode`,
> que é o GameMode da **gameplay padrão**. O mapa do menu precisa
> sobrescrever isso.

1. Abra `L_MainMenu.umap`.
2. Menu **Window → World Settings** (ou no menu superior **Settings →
   World Settings**).
3. Em **Game Mode → GameMode Override**, selecione
   **`BP_FMainMenuGameMode`**.
4. **Save the level**.

> Validação: ao dar Play no editor com `L_MainMenu` aberto, o output
> log deve mostrar `LogLoad: Game class is 'BP_FMainMenuGameMode_C'`.

---

## 2. `BP_DFMainMenuHUD` (parent: `ADFMainMenuHUD`)

Caminho: `Content/DungeonForged/UI/HUD/BP_DFMainMenuHU.uasset` (asset
existente — note o nome truncado; opcionalmente renomeie para
`BP_DFMainMenuHUD` para consistência com os HUDs de Nexus/Run).

### Class Defaults

| Propriedade C++ | Categoria | Asset a atribuir |
|---|---|---|
| `SplashWidgetClass` | DF \| MainMenu \| UI | `WBP_SplashScreen` |
| `MainMenuWidgetClass` | DF \| MainMenu \| UI | `WBP_MainMenu` |
| `CreditsWidgetClass` | DF \| MainMenu \| UI | `WBP_Credits` |
| `ConfirmWidgetClass` | DF \| MainMenu \| UI | `WBP_ConfirmDialog` |
| `SaveSlotWidgetClass` | DF \| MainMenu \| UI | `WBP_SaveSlotSelection` |
| `OptionsWidgetClass` | DF \| MainMenu \| UI | (opcional, Prompt 45) |
| `AchievementListWidgetClass` | DF \| MainMenu \| UI | (opcional, Prompt 61) |

> Se `SplashWidgetClass` ficar **None**, o HUD pula direto para
> `ShowMainMenu` — útil para iterações rápidas no editor.

### O que o HUD faz automaticamente

Você **não** escreve nenhum nó BP no Event Graph deste HUD. O fluxo já
está em C++:

- `OnLocalPlayerMenuReady` (chamado por `PostLogin` no GameMode):
  - cria e exibe o `WBP_SplashScreen` em ZOrder 0;
  - foco do teclado no splash → tecla/clique pula a sequência.
- `ShowMainMenu`:
  - remove splash, cria o `WBP_MainMenu` em ZOrder 5;
  - chama `RefreshForCurrentSaveState` (mostra/oculta "Continuar").
- `ShowSaveSlotLayer(Mode)`: ZOrder 20.
- `ShowCredits`: ZOrder 15.
- `ShowConfirmDialog`: ZOrder 100 (sempre por cima).
- `RestoreMainMenuFocus`: devolve foco ao Main após fechar overlays.

---

## 3. `WBP_SplashScreen` (parent: `UDFSplashScreenUserWidget`)

### Bindings obrigatórios (`meta=(BindWidget)`)

| Nome no Designer | Tipo | Obrigatório |
|---|---|---|
| `SplashImage` | `Image` | ✅ |

### Bindings opcionais

| Nome | Tipo | Quando usar |
|---|---|---|
| `SubtitleText` | `TextBlock` | exibe "Um Roguelike ARPG" no 3º splash |

### Layout sugerido

- Root: `Canvas Panel` em fullscreen.
- `Image` cobrindo a tela inteira com cor preta (background).
- `Image` chamado **`SplashImage`** centralizado (Anchor center, Size To
  Content **off**, escolha um tamanho fixo, p.ex. 1280×720 com Aspect
  Ratio FitInside).
- Opcional: `TextBlock` chamado **`SubtitleText`** abaixo do logo
  (visível apenas no 3º splash; o C++ seta o texto).

### Configurar Class Defaults

| Propriedade | Tipo | Como preencher |
|---|---|---|
| `SplashImages` | `TArray<UTexture2D*>` | 3 elementos: `[0]=Logo Epic/UE`, `[1]=Logo Studio`, `[2]=Logo DungeonForged` |
| `HoldDurations` | `TArray<float>` | opcional — sobrescreve `HoldSeconds` por índice |
| `PhaseConfig` | `TArray<FDFSplashPhaseConfig>` | opcional — `FadeIn / Hold / FadeOut` por índice |

> Defaults internos (caso `PhaseConfig` esteja vazio):
> - índices 0 e 1: 1.2 s in / 1.0 s hold / 0.8 s out
> - índice 2 (title card): 1.2 s in / 1.5 s hold / 0.5 s out

### Eventos Blueprint (opcional, para polish)

Override em **Class Defaults → Functions** ou no **Event Graph** via
"Override Function":

- `On Splash Index Changed (Index, Texture)` — útil para tocar SFX por
  fase.
- `On Title Card Shown (TitleTexture)` — toque o "scale up 90%→100%"
  do logo do jogo (UMG Animation com `Render Transform → Scale`).
- `Apply Splash Visible Alpha (Opacity, Index)` — se quiser um fade
  custom (ex.: ColorAndOpacity de várias camadas), sobrescreva aqui.

### Skip

Já implementado em C++ — qualquer tecla ou clique após 0,5 s pula tudo.

---

## 4. `WBP_MenuButton` (parent: `UDFMenuButtonUserWidget`)

Botão "parchment-style" reutilizável. Usado pelo `WBP_MainMenu`.

### Bindings

| Nome | Tipo | Required |
|---|---|---|
| `Button` | `Button` | ✅ |
| `ButtonLabel` | `TextBlock` | opcional (mas recomendado) |
| `SubLabel` | `TextBlock` | opcional (sub-texto pequeno) |

### Animations (opcional)

| Nome da animation | Quando dispara |
|---|---|
| `HoverAnim` | mouse enter (shift right + glow) |
| `PressAnim` | botão pressed (scale 0.97) |

### Audio

Class Defaults → DF | MainMenu | Button | Audio:
- `HoverSound`: SFX leve de papel.
- `ClickSound`: SFX de carimbo / selo.

> Compatível com **Common UI**, mas você pode usar UMG puro. O `Button`
> precisa estar com **Is Focusable = true** para navegação por gamepad.

---

## 5. `WBP_MainMenu` (parent: `UDFMainMenuUserWidget`)

Layout vertical com a árvore de botões + textos.

### Bindings (todos `BindWidgetOptional` — só configure os que existirem)

| Nome | Tipo | Função em C++ |
|---|---|---|
| `ContinueAdventureButton` | `Button` | retoma run ativa, ou abre slot select |
| `NewAdventureButton` | `Button` | abre slot select (opcional confirma) |
| `ManageProfilesButton` | `Button` | abre slot select em modo Manage/Delete |
| `OptionsButton` | `Button` | abre overlay de opções (Prompt 45) |
| `AchievementsButton` | `Button` | abre overlay de conquistas (Prompt 61) |
| `CreditsButton` | `Button` | abre `WBP_Credits` |
| `QuitButton` | `Button` | abre confirm dialog → `QuitGame` |
| `LogoImage` | `Image` | logo do jogo |
| `SubtitleText` | `TextBlock` | C++ seta "Um Roguelike ARPG" |
| `VersionText` | `TextBlock` | C++ seta "vX.Y.Z \| UE 5.4" |
| `CopyrightText` | `TextBlock` | C++ seta texto de copyright |
| `ContinueSubText` | `TextBlock` | C++ seta "Andar X — Classe" sob "Continuar" |

> **Importante:** os botões podem ser instâncias de `WBP_MenuButton`,
> mas o `meta=(BindWidget)` é **`UButton`**. Para usar `WBP_MenuButton`:
> - na hierarquia, o `WBP_MenuButton` interno expõe a propriedade
>   `Button` se você marcar **"Is Variable"** + **"Expose on Spawn"** —
>   ou crie no `WBP_MainMenu` um `Button` com o nome esperado e
>   posicione um `WBP_MenuButton` como filho dele apenas para visual.
> - **Caminho mais simples e funcional:** usar `Button` puro com nome
>   `ContinueAdventureButton` etc., e estilizá-lo via Brush. O
>   `WBP_MenuButton` é opcional para o efeito de hover; se quiser usá-lo,
>   conecte o evento `OnMenuButtonClicked` (delegate Multicast) no
>   Event Graph do `WBP_MainMenu`.

### Layout sugerido (Designer)

```
Canvas Panel
└─ Horizontal Box (anchor: Fill)
   ├─ (Left) Vertical Box  ← painel à esquerda
   │  ├─ Image LogoImage
   │  ├─ TextBlock SubtitleText
   │  └─ Vertical Box  ← stack de botões
   │     ├─ Button ContinueAdventureButton
   │     ├─ TextBlock ContinueSubText
   │     ├─ Button NewAdventureButton
   │     ├─ Button ManageProfilesButton
   │     ├─ Button OptionsButton
   │     ├─ Button AchievementsButton
   │     ├─ Button CreditsButton
   │     └─ Button QuitButton
   └─ (Right) spacer / cinematic area (vazio — câmera atrás)
TextBlock VersionText (anchor bottom-left)
TextBlock CopyrightText (anchor bottom-right)
```

### Visibilidade automática

Ao construir, o C++ chama `RefreshForCurrentSaveState`, que:

- Esconde **`ContinueAdventureButton`** se nenhum slot tem
  `bHasActiveRun = true`.
- Esconde **`AchievementsButton`** e **`ManageProfilesButton`** se
  nenhum slot/legacy save existe.
- Dispara `OnNewAdventureEmphasisChanged(true)` quando não há perfis
  — implemente no Blueprint para destacar visualmente "Nova Aventura"
  (ex.: tocar uma `WidgetAnimation` de glow).

### Override de evento (opcional, polish)

`OnNewAdventureEmphasisChanged (bEmphasize: bool)` — sobrescreva para
ativar/desativar uma animação UMG de destaque.

---

## 6. `WBP_Credits` (parent: `UDFCreditsUserWidget`)

### Bindings obrigatórios

| Nome | Tipo |
|---|---|
| `CreditsScroll` | `ScrollBox` |
| `BackButton` | `Button` |

### Opcional

| Nome | Tipo |
|---|---|
| `SkipButton` | `Button` (pula até o fim) |

### Layout sugerido

- `Canvas Panel` fullscreen, fundo preto semi-transparente.
- `ScrollBox` chamado **`CreditsScroll`** (anchor fill, com padding
  lateral). Dentro dele coloque os créditos como `TextBlock`s
  empilhados em um `Vertical Box`. Conteúdo livre — design.
- `Button BackButton` no canto superior esquerdo ("← Voltar").
- `Button SkipButton` no canto inferior direito (opcional).

### Class Defaults

- `AutoScrollSpeed = 60.0` (px/s).
- `AutoCloseHoldSeconds = 2.0` (segundos parado no fim antes de
  fechar; **0 ou negativo** desativa o auto-close).

### Comportamento automático (C++)

- Auto-scroll com multiplicador 4× se uma tecla estiver pressionada.
- Esc/clique no Back chama `RemoveFromParent` + `RestoreMainMenuFocus`.

---

## 7. `WBP_ConfirmDialog` (parent: `UDFConfirmDialogUserWidget`)

Modal genérico de confirmação. Usado por:
- "Sair" no menu principal;
- "Nova Aventura" quando já existe perfil;
- "Apagar Perfil X?";
- "Abandonar run atual?".

### Bindings obrigatórios

| Nome | Tipo |
|---|---|
| `DarkOverlay` | `Image` (full-screen, bloqueia clique embaixo) |
| `TitleText` | `TextBlock` |
| `BodyText` | `TextBlock` |
| `ConfirmButton` | `Button` |
| `CancelButton` | `Button` |

### Animation opcional

| Nome | Quando dispara |
|---|---|
| `OpenAnim` | aberto via `ShowDialog` (scale 0.8 → 1.0) |

### Layout sugerido

- `Canvas Panel` fullscreen (essencial — o widget é adicionado em
  ZOrder 100).
- `Image DarkOverlay` em **Fill** com cor preta α=0.6.
- Caixa central (`Border` + `Vertical Box`):
  - `TitleText` (header, fonte ~28).
  - `BodyText` (multi-line, fonte ~16).
  - `Horizontal Box` com `CancelButton` ("Cancelar") e `ConfirmButton`
    ("Confirmar").

### Como o C++ usa

O dialog é **construído sob demanda** pelos widgets do menu
(`WBP_MainMenu`, `WBP_SaveSlotSelection`, `WBP_SaveSlotCard`). Você
não precisa de nenhuma lógica BP no `WBP_ConfirmDialog`. O título,
body, e o callback `OnConfirm` chegam via `ShowDialog(Title, Body,
SimpleDelegate)`.

> Se quiser polish extra (ex.: trocar texto do botão Confirmar para
> "Apagar permanentemente" + cor vermelha em casos de delete), exponha
> uma função BP custom no `WBP_ConfirmDialog` (`SetConfirmButtonStyle`)
> e chame-a no Blueprint dos chamadores. Não precisa mudar o C++.

---

## 8. `WBP_SaveSlotSelection` (parent: `UDFSaveSlotSelectionUserWidget`)

Tela com 3 cards de perfil lado a lado.

### Bindings

#### Caminho A — painel dinâmico (recomendado para 3 cards iguais)

| Nome | Tipo |
|---|---|
| `SlotRow` | `HorizontalBox` (ou `WrapBox`, `UniformGridPanel`, qualquer `PanelWidget`) |
| `BackButton` | `Button` |
| `TitleText` | `TextBlock` (opcional) |
| `ManageHintText` | `TextBlock` (opcional, aparece em modo Manage) |

> Nesse caminho o C++ instancia 3 `WBP_SaveSlotCard` em runtime e
> adiciona como filhos de `SlotRow`. Você precisa setar
> `SlotCardClass = WBP_SaveSlotCard` no **Class Defaults** do
> `WBP_SaveSlotSelection`.
>
> Se o painel tem outro nome no Designer, sobrescreva
> `UmgNameSlotRow` em Class Defaults (default: `"SlotRow"`).

#### Caminho B — 3 cards manuais (controle total de layout)

Adicione 3 instâncias de `WBP_SaveSlotCard` nomeadas exatamente:
- **`SlotCard0`**
- **`SlotCard1`**
- **`SlotCard2`**

E marque cada uma como **"Is Variable"** no Designer. O C++ chama
`RefreshSlotData(I, Mode)` em cada uma.

> Não é necessário usar nenhum dos dois caminhos exclusivamente —
> o C++ tenta primeiro o B (cards manuais); se não encontrar,
> reconstrói com o A.

### Class Defaults

| Propriedade | Tipo | Valor |
|---|---|---|
| `SlotCardClass` | `TSubclassOf<UDFSaveSlotCardUserWidget>` | `WBP_SaveSlotCard` |
| `UmgNameSlotRow` | `FName` | `SlotRow` (deixe assim) |

### Layout sugerido

```
Canvas Panel
├─ Image (background dark)
├─ TextBlock TitleText (anchor top-center, fonte grande)
├─ HorizontalBox SlotRow (anchor center, slot 0/1/2 lado a lado)
├─ TextBlock ManageHintText (anchor bottom-center)
└─ Button BackButton (anchor top-left, "← Voltar")
```

### Modos de operação (`EDFSlotScreenMode`)

| Modo | Acionado por | Comportamento |
|---|---|---|
| `SelectToPlay` | "Continuar" / "Nova Aventura" | mostra Play/NewRun/Create por card |
| `SelectToDelete` | "Gerenciar Perfis" | mostra apenas Delete por card; oculta os outros |

O título e o `ManageHintText` mudam automaticamente.

---

## 9. `WBP_SaveSlotCard` (parent: `UDFSaveSlotCardUserWidget`)

Card de perfil com **dois estados**: "Empty" e "Occupied". O C++
suporta 2 padrões de árvore — escolha **um**:

### Padrão A — Widget Switcher (recomendado)

Crie um `Widget Switcher` chamado **`StateSwitcher`** com 2 filhos:
1. Filho **0** = bloco "Empty" (vazio)
2. Filho **1** = bloco "Occupied"

(A ordem é configurável em Class Defaults via `SwitcherEmptyIndex` e
`SwitcherOccupiedIndex`.)

### Padrão B — Roots manuais (alternativo)

Crie dois containers raiz com **"Is Variable"** marcado:
- Um `Border`/`SizeBox` chamado **`EmptyRoot`** com o conteúdo do
  estado vazio.
- Outro `Border`/`SizeBox` chamado **`OccupiedRoot`** com o conteúdo
  do estado ocupado.

O C++ alterna `Visibility` entre eles.

### Bindings (todos `BindWidgetOptional`)

#### Estado **Occupied** (perfil com save)

| Nome | Tipo | Conteúdo |
|---|---|---|
| `SlotBorderImage` | `Image` | borda do card; cor cinza vazia / dourada ocupada |
| `ClassPortraitArt` | `Image` | retrato da classe (`FDFClassTableRow.ClassPortrait`) |
| `SlotLabel` | `TextBlock` | "Perfil 1/2/3" |
| `ClassNameText` | `TextBlock` | nome localizado da classe |
| `MetaLevelText` | `TextBlock` | "Nexus Nv. X" |
| `MetaXPBar` | `ProgressBar` | XP normalizado 0..1 |
| `LastFloorText` | `TextBlock` | "★ Vitória", "Run ativa", ou "Melhor andar" |
| `TotalRunsText` | `TextBlock` | "X runs - Y vitorias" |
| `PlayTimeText` | `TextBlock` | "Xh Ymin jogadas" |
| `LastPlayedText` | `TextBlock` | tempo relativo ("Há 2 horas") |
| `IncompatibleVersionText` | `TextBlock` | aviso de versão antiga |
| `UnlockedClassIcons` | `WrapBox` | ícones 24×24 das classes desbloqueadas |
| `PlayButton` | `Button` | "▶ Jogar" |
| `NewRunButton` | `Button` | "+ Nova Run" |
| `DeleteButton` | `Button` | ícone lixeira |
| `ActiveRunBadge` | `Image` | "Run em andamento" |

#### Estado **Empty**

| Nome | Tipo |
|---|---|
| `EmptySlotArt` | `Image` (silhueta) |
| `EmptyText` | `TextBlock` ("Slot Vazio") |
| `HintText` | `TextBlock` ("Clique para criar...") |
| `CreateButton` | `Button` ("+ Criar Perfil") |

#### Animação opcional

| Nome | Quando dispara |
|---|---|
| `CardRefreshAnim` | a cada `RefreshSlotData` (fade leve) |

### Class Defaults

| Propriedade | Default | Quando alterar |
|---|---|---|
| `SwitcherEmptyIndex` | `0` | se inverter ordem do `StateSwitcher` |
| `SwitcherOccupiedIndex` | `1` | idem |
| `DeleteSound` | nullptr | SFX de "papel amassado" no delete |

### Lógica já implementada (C++) — você não precisa fazer no BP

- **Play (occupied):** seleciona slot, e:
  - se `bHasActiveRun = true` → `TravelToNexus(FirstLaunch)`;
  - senão → abre `WBP_ClassSelection` com destino `RunDungeon`.
- **NewRun (occupied):** se há run ativa, abre `ConfirmDialog` para
  abandoná-la; ao confirmar, reseta a run, salva, abre `ClassSelection`.
- **Create (empty):** seleciona slot e abre `ClassSelection` com destino
  `NexusFirstLaunch`.
- **Delete:** abre `ConfirmDialog`, ao confirmar chama
  `Slots->DeleteSlot(SlotIndex)` e refaz o card.

---

## 10. `WBP_SaveSlotCardEmpty` (variante visual — opcional)

Você pode criar **um Widget Blueprint separado** que herda de
`UDFSaveSlotCardUserWidget` e contém **apenas** o layout do estado
"Empty" (sem switcher / sem `OccupiedRoot`). Use-o em situações onde
você quer um look distinto para slots vazios (ex.: arte gigante de
"+" cobrindo todo o card).

### Como acionar

Ele é instanciado quando:
- o `WBP_SaveSlotSelection` cria cards via `RebuildCardWidgets` e
  você troca `SlotCardClass` em runtime baseado em `IsSlotEmpty(I)`,
  ou
- você posiciona manualmente `SlotCard0..2` no Designer e usa o
  `WBP_SaveSlotCardEmpty` para slots vazios e `WBP_SaveSlotCard` para
  ocupados.

> ⚠️ Atenção: como o estado pode mudar (delete → fica vazio), o
> caminho mais robusto é **um único `WBP_SaveSlotCard` com
> StateSwitcher**. Use `WBP_SaveSlotCardEmpty` apenas como variante
> visual auxiliar.

### Bindings (apenas estado Empty)

Idênticos ao subconjunto "Empty" do `WBP_SaveSlotCard`:
`EmptySlotArt`, `EmptyText`, `HintText`, `CreateButton`, `SlotLabel`.

---

## 11. Checklist final de validação

Após criar/configurar todos os assets, valide:

- [ ] `L_MainMenu` → World Settings → GameMode Override =
  `BP_FMainMenuGameMode`.
- [ ] `BP_FMainMenuGameMode` → `MainMenuHUDClass` =
  `BP_DFMainMenuHU` (ou `BP_DFMainMenuHUD`).
- [ ] `BP_DFMainMenuHUD` → 5 widget classes preenchidas (Splash, Main,
  Credits, Confirm, SaveSlot).
- [ ] `WBP_SplashScreen` → `SplashImages` tem 3 texturas, `SplashImage`
  está bound.
- [ ] `WBP_MainMenu` → todos os botões estão nomeados conforme tabela.
- [ ] `WBP_SaveSlotSelection` → `SlotCardClass` apontando para
  `WBP_SaveSlotCard` (ou 3 cards manuais nomeados `SlotCard0..2`).
- [ ] `WBP_SaveSlotCard` → `StateSwitcher` com 2 filhos OU
  `EmptyRoot`/`OccupiedRoot` em variável.
- [ ] `WBP_ConfirmDialog` → 5 bindings obrigatórios + 2 botões.
- [ ] `WBP_Credits` → `CreditsScroll` + `BackButton` bound.
- [ ] PIE com `L_MainMenu` aberto:
  1. splash roda e termina;
  2. menu principal aparece com botões corretos para o estado de save;
  3. "Nova Aventura" abre slot select;
  4. "+ Criar" em slot vazio abre `WBP_ClassSelection`.

## 12. Troubleshooting

| Sintoma | Causa provável | Como resolver |
|---|---|---|
| Tela preta após PIE | GameMode Override não aplicado | World Settings de `L_MainMenu` |
| Splash some imediatamente | `SplashImages` vazio | popular array no Class Defaults |
| Menu sem texto | `SubtitleText`/`VersionText` não bound | renomear no Designer |
| "Continuar Aventura" sempre escondido | nenhum slot tem `bHasActiveRun` | esperado se você nunca jogou |
| Card não atualiza após delete | `SlotCard0..2` não estão **Is Variable** ou `SlotRow` está com nome diferente | renomear / marcar Is Variable |
| Logs `WBP_SaveSlotSelection: nenhum painel 'SlotRow'` | ver acima | mesma causa |
| Logs `WBP_SaveSlotCard: slot vazio sem EmptyText/SlotLabel` | nenhum dos `BindWidgetOptional` foi nomeado | adicione um `TextBlock` nomeado `EmptyText` ou `SlotLabel` no estado vazio |
| Splash não pula com tecla | `WBP_SplashScreen` não está focado | confirme que o root tem `Is Focusable = true` (já default em C++) e que há 0,5 s mínimo desde abertura |

---

## 13. Referências cruzadas

- Save subsystem: `Source/DungeonForged/Public/Run/UDFSaveSlotManagerSubsystem.h`
- Travel: `Source/DungeonForged/Public/World/UDFWorldTransitionSubsystem.h`
- Cinemática de fundo: `Source/DungeonForged/Public/World/UDFCinematicSubsystem.h`
- Música: `Source/DungeonForged/Public/Audio/UDFMusicManagerSubsystem.h`
- Para configurar a tela seguinte → [`ClassSelection_Setup.md`](ClassSelection_Setup.md).
