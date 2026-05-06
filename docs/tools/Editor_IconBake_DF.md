# DF Icon Bake (Editor) — malha estática ou skeletal → Texture2D

Ferramenta **genérica** no código do projeto: não usa `UDFClassSelectionSubsystem`. Serve para gerar **ícones** ou **artes 2D de alta resolução** (ex.: perfil de hero) a partir de um `SceneCaptureComponent2D` no Editor.

## C++

### Módulos

- **`DungeonForged`** (Runtime): biblioteca **`UDFEditorIconBakeLibrary`**, enums partilhados (`EDFICAssetType`, presets de bake), **`UDFIconStudioWorkflowSettings`**. Builds de **jogo** não carregam `UEditorSubsystem`/`UEditorUtilityWidget` através deste DLL.
- **`DungeonForgedEditor`** (Editor only, em `Source/DungeonForgedEditor`): **`UDFEditorIconBakeWidget`** (EUW C++ base) e **`UDFEditorIconStudioSubsystem`** (auto-abrir EUW ao abrir o mapa). O UHT de targets de jogo **não aceita** `UEditorSubsystem` nem `#if WITH_EDITOR` em volta de `UCLASS` completo como atalho; por isso o código de editor ficou aqui — equivalente estável à «Solução 2» da tua análise.

- Classe: `UDFEditorIconBakeLibrary` (`Source/DungeonForged/.../Editor/IconBake/`).
- Nós Blueprint (categoria **DF|Editor|IconBake**, `DevelopmentOnly`):
  - **DF Icon Bake · Configure RT e capturar** — redimensiona o render target (8–8192), define formato, limpa, `CaptureScene`.
  - **DF Icon Bake · RT → Texture2D Asset** — grava no Content Browser com preset `EDFIconBakeTexturePurpose`.
  - **DF Icon Bake · Capturar e gravar Texture2D** — captura + grava num passo (preset).
  - **DF Icon Bake · RT → Texture2D (definições manuais)** — `CompressionSettings`, `MipGen`, `TextureGroup`, `bSRGB`; `bCreateUniqueAssetNameIfExists` gera nome livre (tipo `IAssetTools::CreateUniqueAssetName` em ferramentas tipo Icon Creator).
  - **DF Icon Bake · Capturar e gravar Texture2D (definições manuais)** — escolhes também `ETextureRenderTargetFormat`.
  - **DF Icon Bake · Atualizar preview skeletal (scrub)** — após mudar o tempo da animação no Editor, força refresh dos ossos no `USkeletalMeshComponent` antes de capturar.

## Como aceder ao bake (não há menu por defeito)

1. O Editor tem de estar com o projeto **DungeonForged** compilado (target **Editor**).
2. Os nós aparecem no **Palette** ao pesquisar **DF Icon Bake**, ou em **DF** → **Editor** → **IconBake** (nome na UI pode variar).
3. **Recomendado:** cria um **Editor Utility Widget** (tipo *Editor Widget Blueprint* / *Blutility widget* na pasta Content). No evento **Construct** ou num **Button → OnClicked**, chama **DF Icon Bake · Capturar e gravar Texture2D** e ligas manualmente ao **`SceneCaptureComponent2D`** (do actor no nível ou variável **Editor Utility Actor**) ao **`TextureRenderTarget2D`**, mais pasta `/Game/...` e nome do asset.
4. **Abrir a ferramenta:** clica com o direito no asset do EUW → **Run Editor Utility Widget** (ou equivalente UE 5.4), ou registar o EUW nos **toolbar/menus do Editor** nos **Developer Settings → Editor Utility** se quiseres atalho fixo — isso não vem criado pelo C++; é só UE.
5. Tens **mapa editor aberto** com o **BP**/actor de estúdio (captura + malhas); **não é obrigatório** dar Play.

### Presets (`EDFIconBakeTexturePurpose`)


| Valor           | RT capturado     | Textura gerada (resumo)                                                                                                     |
| --------------- | ---------------- | --------------------------------------------------------------------------------------------------------------------------- |
| **UiIcon**      | `RTF_RGBA8_SRGB` | BC7, `TEXTUREGROUP_UI`, SRGB, sem mips.                                                                                     |
| **HeroProfile** | `RTF_RGBA16f`    | `TC_HDR`, grupo **Cinematic**, linear, sem mips — melhor para **altíssima** fidelidade antes de compressão final no editor. |


Se já existir um asset com o mesmo caminho/nome, a gravação **nos nós por preset falha** (log `[DF|IconBake] Já existe …`); apaga, muda o nome ou usa os nós **(definições manuais)** com **Create Unique Asset Name** activado.

### Paridade com ferramentas tipo Icon Creator (Fab / Strawberry Hat)

Para alinhar o teu `EUW` ao fluxo típico **Icon Creator** (compressão tipo `EditorIcon`, grupo UI, `TMGS_FromTextureGroup`, iterations): usa **DF Icon Bake · Capturar e gravar (definições manuais)** e define explicitamente cada enum. Os UPROPERTIES dum widget de exemplo (mesh, overlay, tempo de anim…) continuam melhor como **EUW/Object + Details View**, não há necessidade do plugin externo.

## Checklist Icon Studio (onde criar e o que ligar)

Sugestão de pastas em Content (podes ajustar; mantém tudo sob `/Game/DungeonForged/...`):


| Asset             | Exemplo de caminho                                                    | Função                                                                     |
| ----------------- | --------------------------------------------------------------------- | -------------------------------------------------------------------------- |
| Nível de trabalho | `/Game/DungeonForged/Maps/L_IconStudio`                               | Cena com luzes + **uma** instância de `BP_IconCreatorActor`.               |
| Actor de estúdio  | `/Game/DungeonForged/Editor/BP_IconCreatorActor`                      | `SceneCaptureComponent2D`, malhas, luzes.                                  |
| Render target     | `/Game/DungeonForged/Editor/RT_IconStudio` (Texture Render Target 2D) | Destino da captura; assign no **Scene Capture** → *Texture Target*.        |
| EUW               | `/Game/DungeonForged/Editor/EUW_IconCreator`                          | Botões / campos que chamam **DF Icon Bake · Capturar e gravar Texture2D**. |
| Saída de texturas | `/Game/DungeonForged/Art/Icons` (pasta vazia no Content)              | Valor de `ContentFolder` no nó (string `/Game/DungeonForged/Art/Icons`).   |


**No `BP_IconCreatorActor`**

1. No **SceneCaptureComponent2D**, *Texture Target* = `RT_IconStudio` (ou o RT que criaste).
2. Ajusta *Capture Source*, *Show Only Components* ou post-process conforme queres isolamento / fundo.
3. Opcional mas útil: no actor colocado em `L_IconStudio`, adiciona **Actor Tag** `IconStudio` (ou nome fixo único).

**No `EUW_IconCreator` (Event Graph)**

1. **`World Context Object`** → liga **`self`** (o próprio Editor Utility Widget). Se deixares desligado, o C++ ainda tenta resolver mundo via `SceneCapture`; com `self` fica mais previsível.
2. Obtém referência ao estúdio no nível (**com `L_IconStudio` aberto no Editor**):
  - Na Paleta / menu de contexto, procura **Get Editor World** (categoria **Editor Scripting**). Se não aparecer, ativa o plugin **Editor Scripting Utilities** no `.uproject`.
  - Encadeamento típico: **Get Editor World** → **Get All Actors of Class** (`BP_IconCreatorActor`) → **Get** (copy) índice `0` (por isso há **uma** cópia do actor no nível).
  - Nesse actor: **Get Component by Class** → `Scene Capture Component 2D` → liga ao pin **Scene Capture** do bake.
3. **`Render Target`** → o mesmo asset `RT_IconStudio` (arrasta do Content Browser ou variável).
4. **`Width` / `Height`**: para ícone de lista usa **256–512** (ou **1024**); para hero/splash **2048–4096** (VRAM). **100×100** só serve para teste rápido.
5. **`ContentFolder`**: string exata `/Game/DungeonForged/Art/Icons` (sem barra no fim).
6. **`AssetName`**: ex. `T_Class_Warrior` (sem extensão; se já existir asset com o mesmo nome no caminho, usa o nó com **nome único** ou muda o nome).
7. **`Purpose`** (nós preset): **UiIcon** para HUD/listas; **HeroProfile** para máxima qualidade antes de tratares a textura no editor.
8. **`Background Clear Color`**: alpha **0** se quiseres transparência no RT (junto com captura adequada).

**Auto-abrir o EUW**

- *Project Settings → Dungeon Forged | Icon Studio Workflow*: **Auto Open** + **Icon Studio Persistent Map** = `L_IconStudio` + **EUW** = `EUW_IconCreator`.

**Aviso "DefaultEditor.ini não é gravável"**

O Editor não consegue escrever o ficheiro (read-only ou controlo de versões). Ou tornas `Config/DefaultEditor.ini` gravável na cópia local, ou gravas só via **Project Settings** (vai para `Saved/…` até poderes atualizar o default no repo).

## Setup recomendado (sem Fab / sem dependências)

1. **Mapa de estúdio** (qualquer level de trabalho, ex. `L_IconStudio`):
  - Cria o nível vazio no Content (ex. `Content/.../L_IconStudio`) e abre-o no **Editor** — **não precisas de dar Play** para o bake.
  - Actor com `SceneCaptureComponent2D`, luzes e **StaticMeshComponent** e/ou **SkeletalMeshComponent** (ou child actors).
  - Assigna um `TextureRenderTarget2D` dedicado (ex. `RT_DFIconBake`) ao capture.
  - **Abrir o EUW ao abrir o mapa (opcional):** *Edit → Project Settings → Dungeon Forged | Icon Studio Workflow* → ativa **Auto Open**, escolhe o asset **Persistent Map** (`UWorld`, ex. `L_IconStudio`) e o **Editor Utility Widget**. Só quando esse mapa for aberto no Editor é que o EUW abre. Em alternativa, abre o EUW manualmente com **Run Editor Utility Widget**.
2. **Editor Utility Widget (EUW)** ou Blueprint de editor:
  - Expõe largura/altura (ex. **4096** ou **8192** para hero; atenção a VRAM).
  - Chama **Capturar e gravar** com `ContentFolder` = `/Game/DungeonForged/Art/Icons` (ou subpasta tua) e `AssetName` = `T_Hero_Warrior`.
3. **Transparência**: usa `BackgroundClearColor` com **alpha 0** e configura o capture (ShowOnly list, fundo, etc.) como no teu pipeline de arte.

## Notas

- **Static vs skeletal**: o C++ não spawna malhas; colocas o que quiseres na cena visível ao capture.
- **Resolução máxima** clamp em **8192**; para 8K+ personalizado, altera `IconBakeMaxRes` no `.cpp`.
- **Packaging**: funções devolvem `false`/`nullptr` em build **não-Editor** (`#if !WITH_EDITOR`).
- Após gravar, podes reimportar/recompressar no editor de textura ou apontar o campo `ClassPortrait` (ou outro) na Data Table.

