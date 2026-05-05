# Esfera envolvente com textura / shading “cel” no preview de classe

Este guia descreve como criar uma **esfera grande** com o jogador (ou o **preview character** do `UDFClassSelectionSubsystem`) **no interior**, com aparência **cel-shaded** (“cell”, estilo cartoon/anime). Funciona bem com o modo **`WorldShowcaseCamera`** e o actor com tag **`ClassSelectionPreview`** no mapa.

## O que queres obter

- Uma **superfície esférica** visível **de dentro** (como um “domo” ou fundo de vitrine).
- Um material com **bandas de luz** e/ou **textura** que lembre **cel shading**, sem precisar de plugins.

## 1. Colocar a esfera no nível

1. No **Content Browser**, usa uma malha esférica:
   - **Engine** → `Engine/BasicShapes/Sphere` (ou o teu `StaticMesh` equivalente), **ou**
   - Importa uma esfera simples (FBX) se preferires controlo total das normais.
2. Arrasta para o mapa (ex.: **Nexus** ou nível onde abres a seleção de classes) um **Static Mesh Actor**.
3. **Posiciona e escala** de forma a envolver o ponto onde o preview spawna:
   - Alinha o centro da esfera ao **Empty Actor** (ou **Target Point**) que usas com a tag **`ClassSelectionPreview`**, ou ajusta manualmente até o personagem ficar no meio.
4. **Escala** típica: valores grandes (ex. `20`–`80`) dependendo do tamanho do mesh base; o importante é o interior ter espaço para câmara e personagem.

## 2. Ver o interior da esfera (normais)

O mesh esférico padrão foi feito para ser visto **por fora**. De **dentro**, as faces podem ficar **invisíveis** (back-face culling).

Tens três caminhos comuns (usa **um** deles):

| Abordagem | Notas |
|-----------|--------|
| **Two Sided** no material | Mais simples; ativa renderização nos dois lados da face. Custo um pouco maior. |
| **Escala negativa** no actor | Ex.: `Scale X = -1` (e eventualmente ajustar Y/Z conforme orientação) para **inverter** a malha visualmente e tratar como “dentro para fora”. |
| **Malha com normais invertidas** no DCC | Máximo controlo para produção final. |

Recomendação rápida: começar com **Two Sided** no material (secção seguinte).

## 3. Material “cel” básico (Surface, Opaque)

Cria um **Material** (ex.: `M_PreviewSphere_Cel`) e configura assim:

1. **Material Domain**: Surface  
2. **Blend Mode**: Opaque (ou Masked se usares máscaras de rasgo)  
3. No painel principal do material:
   - Ativa **Two Sided** (se estiveres a ver de dentro da esfera).

### Bandas tipo “cell” sem textura complexa

1. Usa **`Pixel Normal WS`** ou o node **`VertexNormalWS`** (normal no espaço mundo).
2. Cria uma direção de “luz falsa” fixa como **Constant3Vector** normalizado (ex. `(0.3, 0.7, 0.2)`).
3. Faz **`DotProduct(Normalize(FakeLightDir), Normalize(VertexNormalWS))`**.
4. Passa esse valor por **`Ceil`** × escala, **`Floor`** / **`Round`** com multiplicador, ou usa **`Posterize`** (quando disponível) para **2–4 bandas de luminância**.
5. Multiplica pela **Cor Base** (**Constant3Vector** ou **Texture Sample** para “textura cel”).
6. Liga ao **Base Color**. Para um look mais cartoon, **`Roughness`** alto e **`Specular`** baixo ajudam quando usas Default Lit.

### Com textura pintada (“textura cell”)

- Faz **`Texture Sample`** → multiplica pelo resultado das bandas acima ou usa a textura só como **`Base Color`** e aplica **`Multiply`** com um **`Scalar Parameter`** tipo `CelBands` para intensidade das sombras.
- Texturas noise orgânicas subtis + cores chapadas lembram muitos jogos tipo showcase.

### Shading Lit vs Unlit

- **Unlit**: controlo total pelo fake N·L + cor; sempre estável independentemente das luzes do nível (bom para HUD/showcase consistente).
- **Default Lit**: reage ao **Sky Light**/**Directional Light** do mapa; podes acrescentar ainda assim um **multiply** por bandas em cima para reforço “cel”.

Para um fundo só de vitrine (Genshin-style stage), **Unlit ou Lit simples + bandas fortes** costuma ser suficiente.

### Parâmetros expostos (Material Instance)

Cria **`Material Instance`** (`MI_PreviewSphere_Cel`) e torna parametrizável:

- `BaseTint` (Vector)
- `CelIntensity` ou `PosterizeSteps` (Scalar)
- `FakeLightDirection` (Vector) — opcional

Assim podes clarificar por mapa ou por hora do dia fictícia sem recompilar o material.

## 4. Opcional — contorno grosso (“outline”) no limite

Um truque comum para reforço “anime”:

- **`Fresnel`** com **normal personalizada** ou **`PixelDepthOffset`** grosso só em mesh separado maior (outline mesh). Para uma esfera de fundo, muitas vezes **não é necessário**; prioriza ler bem o interior antes.

## 5. Integração com o DungeonForged (preview de classe)

- O **spawn** do personagem de preview segue **`ClassSelectionPreview`** (âncora) e opcionalmente **`ClassSelectionPreviewCamera`** — ver código e `docs/blueprints/ClassSelection_Setup.md`.
- Coloca a esfera no **mesmo espaço local** dessa âncora:
  - Podes **tornar** a esfera **filha** do actor Empty com a tag **`ClassSelectionPreview`** no editor (**Attach**), assim move tudo junto.
- Garante que a **layer de colisão** da esfera **não bloqueie** pawn/câmaras (collision **NoCollision** ou **Overlap only** conforme precisares).
- Se usares **`WorldShowcaseCamera`**, ajusta **FOV** e posição da câmara para que o horizon da esfera não corte estranhamente — **subir ou afastar** a câmara corrige vignettes grandes.

## 6. Luz e bake

- Com material **Unlit**, luz ambiente quase irrelevante para a esfera.
- Com **Lit**, um **Sky Light** + **Directional** suaves ajudam; evita **Lumen/reflexos fortes na esfera** se quiseres um look “poster” chapado — podes usar **Roughness máximo**.

## Checklist rápido

- [ ] Spawning do preview coincide com o centro visual da esfera  
- [ ] Material **Two Sided** ou malha invertida — interior visível  
- [ ] Collision desligada ou apenas overlap  
- [ ] **`ClassSelectionPreviewCamera`** enquadrado para não ver apenas “pole” da esfera  
- [ ] **`MI_*`** criado para iterar arte sem mexer no material mestre  

## Referências úteis (Unreal)

- Materiais Surface + **bandas** (dot + floor/step/posterize)  
- **`Two Sided`** na documentação de materiais UE5  
- `ClassSelection_Showcase`/tags no projeto: `UDFClassSelectionSubsystem` (`WorldShowcaseCamera`)

---

*Este ficheiro é documentação do projeto DungeonForged; ajusta caminhos de assets aos teus pastas sob `/Game/DungeonForged/...`*
