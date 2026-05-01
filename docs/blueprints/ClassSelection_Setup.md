# Class Selection — Setup completo dos Blueprints

Documento prático para criar/configurar os assets da tela de seleção
de classe do DungeonForged. A `UDFClassSelectionSubsystem` (UWorldSubsystem)
é instanciada **por mundo** — ou seja, funciona tanto no `L_MainMenu`
(quando aberta a partir de um Save Slot) quanto no `L_Nexus` (quando
aberta pelo Portal do Nexus).

> **Pasta-alvo dos widgets:** `Content/DungeonForged/UI/ClassSelection/`
> (criar caso não exista)
>
> **Mapas onde abre:** `L_MainMenu` (entry) e `L_Nexus` (Portal — Prompt 47).

---

## Visão geral

Diferente do menu principal — que tem um HUD próprio criando os widgets
— a Class Selection é **disparada por código** via:

```cpp
UDFClassSelectionSubsystem* Sub = GetWorld()->GetSubsystem<UDFClassSelectionSubsystem>();
Sub->SetMainMenuClassPickDestination(EDFMainMenuClassPickDestination::RunDungeon);
Sub->OpenClassSelection();
```

Isso já é feito automaticamente pelos cards do `WBP_SaveSlotCard` e
pelo `ADFNexusInteractable` do portal (Prompt 47). Você não precisa
adicionar nenhum hook no menu principal.

A subsystem cuida de:

- aplicar slow-mo (`SetGlobalTimeDilation(PreviewTimeDilation)`);
- adicionar a tag `UI.ClassSelectionOpen` no AbilitySystem do PlayerState;
- spawnar uma `ADFPlayerCharacter` em `PreviewSpawnLocation` (ou no
ator marcado com tag `ClassSelectionPreview`);
- criar `SpringArm + SceneCapture2D` que renderiza para um
`UTextureRenderTarget2D` consumido pelo `WBP_ClassSelection`;
- instanciar `WBP_ClassSelection` em ZOrder 15.

---

## 1. Configurar o `UDFClassSelectionSubsystem`

A subsystem é uma `UWorldSubsystem` — por padrão você não cria um
asset para ela. Mas suas propriedades `EditAnywhere` permitem
configurar via:

- **Project Settings** (se o seu projeto definir o subsystem como
"Mutable Project Settings", o que **não é o caso** aqui), **ou**
- atribuição em runtime via Blueprint (recomendado).

### Caminho recomendado: configurar no `BP_DFGameInstance`

Como a subsystem é por mundo, configure-a quando o mundo carrega.
No `BP_DFGameInstance` (ou no `BP_DFNexusGameMode` / `BP_FMainMenuGameMode`),
adicione no **Event BeginPlay** (ou `OnPostLoadMap`):

1. **Get World Subsystem (UDFClassSelectionSubsystem)** — node BP.
2. Setar:
  - `Preview Pawn Class` → `**BP_DFPlayerCharacter`** (ou variante de
   preview leve).
  - `Class Selection Widget Class` → `**WBP_ClassSelection**`.
  - `Preview Render Target` → opcional — deixe **None** para que o
  subsystem crie um RT transient automaticamente (1024×1024).
  - `Preview Spawn Location` / `Preview Spawn Rotation` → onde o
  pawn de preview aparece. Default `(0, 5000, 120)` é "off-screen"
  no eixo Y para não colidir com a câmera.
  - `Preview Time Dilation` → `0.3` (slow-mo enquanto a tela está
  aberta).

### Caminho alternativo: tag `ClassSelectionPreview` no mapa

Em vez de usar `Preview Spawn Location`, você pode marcar um
**Target Point** (ou qualquer Actor) no mapa com a tag
`ClassSelectionPreview`. O subsystem usa o transform desse Actor
automaticamente.

> Bom para `L_Nexus`: posicione o ator de preview num "stage" iluminado
> off-screen para a câmera real.

---

## 2. `WBP_ClassSelection` (parent: `UDFClassSelectionWidget`)

Tela full-screen de 3 colunas: **lista de classes** | **preview 3D** |
**detalhes**.

### Bindings — bottom bar (**obrigatórios**)


| Nome            | Tipo     | Função                                              |
| --------------- | -------- | --------------------------------------------------- |
| `BackButton`    | `Button` | "← Voltar" → `CloseClassSelection(false)`           |
| `ConfirmButton` | `Button` | "⚔️ Iniciar Aventura" → `CloseClassSelection(true)` |


### Bindings — coluna central (preview)


| Nome                  | Tipo        | Required                                    |
| --------------------- | ----------- | ------------------------------------------- |
| `PreviewRenderTarget` | `Image`     | ✅ — recebe o RT do SceneCapture             |
| `SelectedClassName`   | `TextBlock` | ✅ — nome grande sob o preview               |
| `PreviewBorderImage`  | `Image`     | opcional — borda colorida pela `ClassColor` |


### Bindings — coluna esquerda (lista)


| Nome              | Tipo        | Required                                      |
| ----------------- | ----------- | --------------------------------------------- |
| `ClassListScroll` | `ScrollBox` | ✅ — recebe `WBP_ClassListEntry`s instanciados |


E em **Class Defaults** atribua:


| Propriedade             | Tipo                                   | Valor                |
| ----------------------- | -------------------------------------- | -------------------- |
| `ClassEntryWidgetClass` | `TSubclassOf<UDFClassListEntryWidget>` | `WBP_ClassListEntry` |


### Bindings — coluna direita (detalhes)


| Nome                      | Tipo                           | Required                             |
| ------------------------- | ------------------------------ | ------------------------------------ |
| `ClassTitle`              | `TextBlock`                    | ✅ — nome grande na cor da classe     |
| `ClassDescription`        | `TextBlock`                    | ✅ — lore + playstyle                 |
| `PlaystyleTag`            | `TextBlock`                    | ✅ — pill "Agressivo" / "Estratégico" |
| `DifficultyBarImage`      | `Image`                        | opcional — pip de dificuldade        |
| `DifficultyLabel`         | `TextBlock`                    | opcional                             |
| `BaseStatsHeader`         | `TextBlock`                    | opcional — "Atributos Base"          |
| `StatBarsBox`             | `VerticalBox`                  | opcional — recebe 5 `WBP_StatBar`    |
| `AbilitiesHeader`         | `TextBlock`                    | opcional                             |
| `StartingAbilitiesScroll` | `ScrollBox`                    | opcional — `WBP_AbilityPreviewIcon`s |
| `MetaUpgradesHeader`      | `TextBlock`                    | opcional                             |
| `MetaUpgradesWrap`        | `WrapBox`                      | opcional                             |
| `HistoryHeader`           | `TextBlock`                    | opcional                             |
| `RunCountText`            | `TextBlock`                    | opcional — "X runs"                  |
| `BestFloorText`           | `TextBlock`                    | opcional — "Melhor: Andar Y"         |
| `WinCountText`            | `TextBlock`                    | opcional — "Z vitórias"              |
| `NoHistoryText`           | `TextBlock`                    | opcional — "Nunca jogada"            |
| `ActiveChallengePanel`    | `WBP_ActiveChallengeIndicator` | opcional                             |
| `CoopStatusText`          | `TextBlock`                    | opcional                             |
| `PartnerClassPreview`     | `WBP_PartnerClassPreview`      | opcional                             |


E em **Class Defaults**:


| Propriedade              | Tipo                                       | Valor                                        |
| ------------------------ | ------------------------------------------ | -------------------------------------------- |
| `StatBarWidgetClass`     | `TSubclassOf<UDFClassStatBarWidget>`       | `WBP_StatBar`                                |
| `AbilityIconWidgetClass` | `TSubclassOf<UDFAbilityPreviewIconWidget>` | `WBP_AbilityPreviewIcon`                     |
| `bHasActiveChallenge`    | `bool`                                     | seta o BP que abrir a tela com desafio ativo |
| `bIsCoopSession`         | `bool`                                     | seta no Prompt 70 (co-op)                    |


### Layout sugerido (Designer)

```
Canvas Panel (fullscreen)
├─ Border (background dark, fill)
├─ HorizontalBox (anchor: fill, padding 24)
│  ├─ ScrollBox ClassListScroll  (Size 280, vertical)
│  ├─ VerticalBox (flex)
│  │  ├─ SizeBox (Image PreviewRenderTarget) — 1024×1024 ratio FitInside
│  │  ├─ TextBlock SelectedClassName (alinhado center, fonte grande)
│  │  └─ Image PreviewBorderImage (decoração)
│  └─ VerticalBox (Size 360, scroll)
│     ├─ TextBlock ClassTitle
│     ├─ TextBlock ClassDescription
│     ├─ TextBlock PlaystyleTag
│     ├─ HorizontalBox (DifficultyBarImage + DifficultyLabel)
│     ├─ TextBlock BaseStatsHeader
│     ├─ VerticalBox StatBarsBox
│     ├─ TextBlock AbilitiesHeader
│     ├─ ScrollBox StartingAbilitiesScroll (orient: Horizontal)
│     ├─ TextBlock MetaUpgradesHeader
│     ├─ WrapBox MetaUpgradesWrap
│     ├─ TextBlock HistoryHeader
│     ├─ TextBlock RunCountText
│     ├─ TextBlock BestFloorText
│     ├─ TextBlock WinCountText
│     └─ TextBlock NoHistoryText
└─ HorizontalBox (anchor: bottom, padding)
   ├─ Button BackButton ("← Voltar ao Nexus")
   ├─ WBP_ActiveChallengeIndicator ActiveChallengePanel  (opcional, no centro)
   ├─ Spacer
   └─ Button ConfirmButton ("⚔️ Iniciar Aventura")
```

### Lógica já implementada (C++) — **não precisa de Event Graph**

- `NativeConstruct`:
  - bind dos botões;
  - faz cache do RT e seta no `PreviewRenderTarget`;
  - cria um `WBP_AbilityTooltip` em ZOrder 20 (escondido até hover);
  - chama `BuildClassList` + `RefreshAll`.
- `BuildClassList`: itera linhas de `DT_Class` (via `UDFRunManager::ClassDataTable`),
cria `WBP_ClassListEntry`s e chama `InitializeEntry`.
- `FillDetailsPanel`: lê `FDFClassTableRow` da classe selecionada,
popula textos, instancia `WBP_StatBar` ×5 e `WBP_AbilityPreviewIcon`
para cada starting ability.
- Drag do mouse no `PreviewRenderTarget` rotaciona o pawn.
- Mouse wheel sobre o preview faz zoom (Spring Arm 200..600).
- `OnConfirmClicked` é bloqueado se a classe não estiver desbloqueada.

---

## 3. `WBP_ClassListEntry` (parent: `UDFClassListEntryWidget`)

Cada item da lista de classes na coluna esquerda.

### Bindings obrigatórios


| Nome             | Tipo                                    |
| ---------------- | --------------------------------------- |
| `ClassPortrait`  | `Image`                                 |
| `ClassName`      | `TextBlock`                             |
| `ClassArchetype` | `TextBlock` ("Tanque Corpo-a-corpo")    |
| `LockOverlay`    | `Image` (overlay translúcido se locked) |
| `UnlockHintText` | `TextBlock` ("Complete 5 runs")         |
| `SelectButton`   | `Button`                                |


### Bindings opcionais (decoração)


| Nome                               | Tipo                                       |
| ---------------------------------- | ------------------------------------------ |
| `DifficultyPip0`..`DifficultyPip4` | `Image` (5 caveiras de dificuldade)        |
| `SelectionBorder`                  | `Image` (borda dourada quando selecionado) |
| `EntryBackground`                  | `Image` (background do item)               |


### Layout sugerido

```
Border / SizeBox (~280×96)
├─ Image EntryBackground (fill)
├─ Image SelectionBorder (fill, hidden por default)
├─ HorizontalBox (padding 8)
│  ├─ Image ClassPortrait (64×64)
│  └─ VerticalBox (flex)
│     ├─ TextBlock ClassName (bold)
│     ├─ TextBlock ClassArchetype (sub-text)
│     ├─ HorizontalBox (DifficultyPip0..4)
│     └─ TextBlock UnlockHintText (visível só se locked)
├─ Image LockOverlay (fill, hidden quando unlocked)
└─ Button SelectButton (fill, transparente)
```

### Lógica já implementada

- `InitializeEntry` é chamado pelo widget pai com (`ClassRow`, owner,
`bUnlocked`, `bSelected`).
- `OnSelectPressed` (já bound em C++) chama
`OwnerScreen->SelectClassByRow(ClassRow)`.

> Você só precisa estilizar — nenhum Event Graph customizado.

### Visual recomendado

- `ClassPortrait` desaturada quando `bUnlocked = false` — implemente
via `Render Transform → Tint` ou um material com parâmetro
`Saturation`.
- `SelectionBorder` visível só quando `bSelected = true`. O C++ chama
`RefreshVisual` ao mudar; sobrescreva o evento Blueprint `**Refresh Visual**` se quiser animar a transição.

---

## 4. `WBP_StatBar` (parent: `UDFClassStatBarWidget`)

Barra horizontal com label + valor.

### Bindings obrigatórios


| Nome        | Tipo          |
| ----------- | ------------- |
| `StatLabel` | `TextBlock`   |
| `StatBar`   | `ProgressBar` |
| `StatValue` | `TextBlock`   |


### Layout sugerido

```
HorizontalBox
├─ TextBlock StatLabel (Size 80)  — "Força"
├─ ProgressBar StatBar (flex)
└─ TextBlock StatValue (Size 40, alinhado direita) — "12"
```

### Lógica

`SetData(Label, NormalizedFill, ValueDisplay, FillColor)` é chamada
pelo widget pai. Ele recolore o `FillColor` do ProgressBar com a cor
do atributo (vermelho=Força, azul=Inteligência, etc.).

---

## 5. `WBP_AbilityPreviewIcon` (parent: `UDFAbilityPreviewIconWidget`)

Ícone clicável de habilidade na coluna direita.

### Bindings


| Nome          | Tipo        | Required                          |
| ------------- | ----------- | --------------------------------- |
| `AbilityIcon` | `Image`     | ✅                                 |
| `AbilityName` | `TextBlock` | ✅                                 |
| `HitButton`   | `Button`    | opcional (recomendado para hover) |


### Layout sugerido

```
SizeBox (64×80)
└─ VerticalBox
   ├─ Image AbilityIcon (48×48)
   ├─ TextBlock AbilityName (small, centralizado)
   └─ Button HitButton (overlay, transparente)
```

### Lógica

- O C++ lê `FDFAbilityTableRow` (passada via `SetAbilityRow`) e popula
ícone + nome.
- Hover/Unhover do `HitButton` mostra/esconde o `WBP_AbilityTooltip`
partilhado (instância única criada pelo `WBP_ClassSelection`).

---

## 6. `WBP_AbilityTooltip` (parent: `UDFClassAbilityTooltipWidget`)

Card flutuante com detalhes da habilidade.

### Bindings obrigatórios


| Nome                 | Tipo                            |
| -------------------- | ------------------------------- |
| `AbilityIcon`        | `Image` (~80×80)                |
| `AbilityName`        | `TextBlock` (bold)              |
| `AbilityDescription` | `TextBlock`                     |
| `CostText`           | `TextBlock` ("Custo: 30 Mana")  |
| `CooldownText`       | `TextBlock` ("Recarga: 8s")     |
| `TagText`            | `TextBlock` ("Fogo · Projétil") |


### Layout

`Border` com background semi-transparente, `Vertical Box` com os
campos acima. Anchor center; o C++ chama `PositionNear` para alinhar
ao ícone, com bordas evitadas.

> **Nota:** este widget é instanciado em código pelo `WBP_ClassSelection`
> (não precisa estar como filho no Designer dele). Apenas exista como
> Widget Blueprint na pasta.

---

## 7. `WBP_ActiveChallengeIndicator` (parent: `UDFActiveChallengeIndicatorWidget`)

Mostrado entre `BackButton` e `ConfirmButton` quando há um Daily/Weekly
Challenge ativo (Prompt 52).

### Bindings (todos opcionais)


| Nome              | Tipo                              |
| ----------------- | --------------------------------- |
| `ChallengeIcon`   | `Image`                           |
| `ChallengeName`   | `TextBlock` ("Desafio: Iron Man") |
| `ModifierSummary` | `TextBlock` ("HP inimigos +50%")  |
| `RemoveChallenge` | `Button` ("×")                    |


### Lógica

C++ não tem lógica interna além do layout — você decide no Blueprint
(Event Graph) como remover o desafio. Tipicamente o evento OnClicked
do `RemoveChallenge` chama o subsystem de challenges, depois
`SetVisibility(Collapsed)`.

---

## 8. `WBP_PartnerClassPreview` (parent: `UDFPartnerClassPreviewWidget`)

Thumbnail pequeno mostrando a classe atualmente "hovered" pelo parceiro
em sessão co-op (Prompt 70).

### Bindings (opcionais)


| Nome                | Tipo        |
| ------------------- | ----------- |
| `PartnerThumbnail`  | `Image`     |
| `PartnerClassLabel` | `TextBlock` |


### Lógica

O C++ do `WBP_ClassSelection` lê
`ADFNexusGameState::CoopPartnerHoveredClass` (replicado) e atualiza
visibilidade. Você decide no Blueprint como animar a transição entre
classes hovered.

---

## 9. Configurar `DT_Class` (`UDFRunManager::ClassDataTable`)

A tabela `Content/DungeonForged/DataTables/DT_Class.uasset` já existe.
Cada linha precisa de (struct `FDFClassTableRow`):


| Campo                 | Como preencher                                                                                   |
| --------------------- | ------------------------------------------------------------------------------------------------ |
| `ClassName`           | FText localizado ("Guerreiro")                                                                   |
| `ClassDescription`    | FText 3-4 frases                                                                                 |
| `PlaystyleTag`        | FText ("Agressivo / Tanque")                                                                     |
| `ClassColor`          | FLinearColor — usado em borda + título                                                           |
| `ClassPortrait`       | UTexture2D                                                                                       |
| `CharacterMesh`       | USkeletalMesh — exibido no preview                                                               |
| `BaseAttributeValues` | TMap<FGameplayAttribute, float> — Strength, Intelligence, Agility, Armor, MagicResist, MaxHealth |
| `StartingAbilities`   | TArray — linhas de `DT_Abilities`                                                                |
| `ClassPreviewIdle`    | TSoftObjectPtr — idle anim no preview                                                            |
| `ClassChangeVFX`      | TSoftObjectPtr — burst ao trocar                                                                 |
| `PreviewCosmeticTint` | FLinearColor — aplicado no material do mesh                                                      |


> **Ajuste pendente comum:** se `ClassPortrait`, `CharacterMesh` ou
> `StartingAbilities` ficarem vazios, o card do menu e os ícones do
> Class Selection ficam em branco. Popule **todas** as 5 classes
> conforme Prompt 49/50.

### Regras de unlock (já em C++ — em `DfIsUnlockedByRules`)


| Classe (FName)                     | Condição               |
| ---------------------------------- | ---------------------- |
| `Guerreiro` / `Warrior`            | sempre desbloqueada    |
| `Mago` / `Mage`                    | sempre desbloqueada    |
| `Assassino` / `Rogue` / `Assassin` | `Save->TotalRuns >= 2` |
| `Paladino` / `Paladin`             | `Save->TotalWins >= 1` |
| `Necromante` / `Necromancer`       | `Save->MetaLevel >= 5` |


> O `Save->UnlockedClasses` (TSet) também sobrescreve essas
> regras — útil para conquistas que dão acesso antecipado.

---

## 10. Configurar pawns/atores no mapa

### `L_MainMenu`

A tela pode abrir aqui (clique em "+ Criar" ou "Nova Run" num save
slot). Para isso:

- Coloque um **Target Point** com tag `**ClassSelectionPreview`** num
ponto fora da câmera principal do menu (ex.: a 50 m no eixo Y).
Pode ser numa "ilha" iluminada, semelhante a um stage.
- Garanta que existe **iluminação suficiente** (uma DirectionalLight
ou PointLight próxima) para o `SceneCapture2D` capturar o pawn
legível.
- O `BackgroundLoopSequence` (cinemática do menu) **não** controla a
câmera do preview — o preview tem câmera própria via SpringArm.

### `L_Nexus`

A tela é aberta ao interagir com o **Nexus Portal** (Prompt 47).
Adicione no Nexus:

- Um **Stage** dedicado off-screen com luz ambiente neutra.
- Um Target Point com tag `ClassSelectionPreview` no centro do stage.
- (Opcional) um material com fundo escuro/gradiente para o RT
capturar.

---

## 11. Fluxo automático de saída

`OnConfirmClicked` em `UDFClassSelectionWidget` chama
`Sub->CloseClassSelection(true)`. Em `CloseClassSelection`:


| Origem (`MainMenuClassDestination`) | Comportamento                                                                                                 |
| ----------------------------------- | ------------------------------------------------------------------------------------------------------------- |
| `NexusFirstLaunch`                  | salva `LastRunClass` + `bHasActiveRun=true` + `bIsFirstLaunch=false`, depois `WT->TravelToNexus(FirstLaunch)` |
| `RunDungeon`                        | mesmas modificações no save, depois `WT->TravelToRun(ConfirmedClass)`                                         |
| `None`                              | (caso aberto pelo Portal no Nexus) chama `ADFNexusPlayerController::Server_BeginRunWithClass(ConfirmedClass)` |


> Não há ação BP necessária — o C++ já cobre tudo.

---

## 12. Checklist final de validação

- `WBP_ClassSelection` criado com 3 colunas + bottom bar.
- `ClassEntryWidgetClass` / `StatBarWidgetClass` /
`AbilityIconWidgetClass` setados em Class Defaults.
- `WBP_ClassListEntry`, `WBP_StatBar`, `WBP_AbilityPreviewIcon`,
`WBP_AbilityTooltip` existem.
- `BP_DFGameInstance` (ou GameMode equivalente) atribui
`PreviewPawnClass` e `ClassSelectionWidgetClass` na subsystem ao
carregar o mundo.
- `DT_Class` populada com as 5 classes (mesh, portrait, attrs,
starting abilities).
- `Target Point` com tag `ClassSelectionPreview` em `L_MainMenu`
e `L_Nexus`.
- PIE com `L_MainMenu`, "+ Criar Perfil" → tela de classe abre,
preview pawn aparece, lista enumera classes, botões funcionam.
- Confirm em "+ Criar" loga `TravelToNexus(FirstLaunch)`.
- Confirm em "+ Nova Run" loga `TravelToRun(<classname>)`.

## 13. Troubleshooting


| Sintoma                                         | Causa                                                             | Solução                                                                  |
| ----------------------------------------------- | ----------------------------------------------------------------- | ------------------------------------------------------------------------ |
| Tela aberta mas preview em branco               | `PreviewPawnClass = None`                                         | atribuir `BP_DFPlayerCharacter` na subsystem (BP_DFGameInstance)         |
| Preview aparece "no chão" do mapa               | `Preview Spawn Location` colide com geometria                     | mover o Target Point ou ajustar `(0, 5000, 120)`                         |
| Lista de classes vazia                          | `DT_Class` não populada ou `UDFRunManager::ClassDataTable` é None | revisar `BP_DFGameInstance` ou o asset DT                                |
| ConfirmButton sempre disabled                   | classe selecionada está locked, ou em co-op nem todos confirmaram | desbloquear no Save, ou desligar `bIsCoopSession`                        |
| Tooltip de habilidade não aparece               | `HitButton` faltante no `WBP_AbilityPreviewIcon`                  | adicionar e nomear corretamente                                          |
| Drag no preview não rotaciona                   | `PreviewRenderTarget` está com `Visibility = Hit Test Invisible`  | mude para `Visible`                                                      |
| Slow-mo persiste após sair                      | `CloseClassSelection` não foi chamado (crash, etc.)               | em PIE o subsystem `Deinitialize` restaura para 1.0 — recarregar o nível |
| Logs `LogTemp: Warning: WBP_ClassSelection ...` | binding obrigatório faltando                                      | renomear no Designer                                                     |


---

## 14. Referências cruzadas

- Tabela: `Source/DungeonForged/Public/Data/DFDataTableStructs.h`
(struct `FDFClassTableRow`).
- Subsystem: `Source/DungeonForged/Public/UI/ClassSelection/UDFClassSelectionSubsystem.h`.
- Travel: `Source/DungeonForged/Public/World/UDFWorldTransitionSubsystem.h`.
- Tags: `FDFGameplayTags::UI_ClassSelectionOpen` (em
`Source/DungeonForged/Public/GAS/DFGameplayTags.h`).
- Para configurar o menu que abre esta tela →
`[MainMenu_Setup.md](MainMenu_Setup.md)`.

