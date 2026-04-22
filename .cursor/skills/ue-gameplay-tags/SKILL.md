---
name: unreal-gameplay-tags
description: >
  Guia completo para GameplayTags no Unreal Engine 5.4 em C++.
  Use este skill sempre que o usuário trabalhar com FGameplayTag, FGameplayTagContainer,
  Native GameplayTags (UE_DEFINE_GAMEPLAY_TAG), DefaultGameplayTags.ini, filtros de tag
  com Categories, FGameplayTagQuery, replicação de tags, UGameplayTagsManager,
  FGameplayTagNativeAdder, ou qualquer sistema hierárquico de labels no UE5.
  Inclui setup, macros nativos, padrões de consulta, replicação e boas práticas.
version: 1.0.0
---

# Unreal Engine 5.4 — GameplayTags

## Visão Geral

`FGameplayTag` é essencialmente um `FName` hierárquico (ex: `"Status.Debuff.Burning"`)
registrado num dicionário central gerenciado pelo `UGameplayTagsManager`. O sistema oferece:

- Hierarquia com `.` como separador → queries parciais e exatas
- Seleção visual no Editor (picker) sem digitar strings
- Replicação eficiente (por índice com Fast Replication)
- Redirect automático ao renomear tags
- Suporte a GAS, mas **independente** dele

---

## Hierarquia de Tags

```
Status
├── Buff
│   ├── Haste
│   └── Shield
└── Debuff
    ├── Burning
    └── Frozen

Ability
├── Active
└── Blocked

Input
└── Pressed
    └── Jump
```

> Tags mais profundas herdam os pais: um container com `Status.Debuff.Burning`
> **também responde** a queries por `Status` e `Status.Debuff` (a menos que use `Exact`).

---

## Setup: Build.cs

Adicione o módulo ao seu `MeuJogo.Build.cs`:

```csharp
PublicDependencyModuleNames.AddRange(new string[]
{
    "GameplayTags",
    // Se usar GAS também:
    // "GameplayAbilities"
});
```

---

## Definindo Tags

### Opção A: Native Tags via Macros (recomendado para UE5)

**Header** — expõe para outros módulos:

```cpp
// GameTags.h
#pragma once
#include "NativeGameplayTags.h"

// Declara extern para uso em qualquer .cpp que incluir este header
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Debuff_Burning)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Buff_Haste)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_Blocked)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_Pressed_Jump)
```

**CPP** — define e registra:

```cpp
// GameTags.cpp
#include "GameTags.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Debuff_Burning,  "Status.Debuff.Burning")
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Buff_Haste,      "Status.Buff.Haste")
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Blocked,        "Ability.Blocked")
UE_DEFINE_GAMEPLAY_TAG(TAG_Input_Pressed_Jump,     "Input.Pressed.Jump")
```

**Uso em qualquer lugar:**

```cpp
#include "GameTags.h"

// Acesso direto — sem RequestGameplayTag, sem race condition
if (MyContainer.HasTag(TAG_Status_Debuff_Burning))
{
    // ...
}
```

**Tag local (só no .cpp atual):**

```cpp
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Temp_Loading, "State.Loading")
```

---

### Opção B: INI File (`DefaultGameplayTags.ini`)

```ini
; Config/DefaultGameplayTags.ini
[/Script/GameplayTags.GameplayTagsList]
GameplayTagList=(Tag="Status.Debuff.Burning",DevComment="Queimando")
GameplayTagList=(Tag="Status.Buff.Haste",DevComment="Velocidade aumentada")
GameplayTagList=(Tag="Ability.Blocked",DevComment="Bloqueia uso de habilidades")
```

> ⚠️ Tags via INI e Native Tags podem **coexistir** — o engine não duplica.
> Se um packaging falhar, adicione a tag no INI mesmo tendo a macro.

---

### Opção C: `FGameplayTagNativeAdder` (padrão legado, ainda válido)

```cpp
// GlobalTags.h
#pragma once
#include "GameplayTagContainer.h"
#include "GameplayTagsManager.h"

struct MEUJOGO_API FGlobalTags : public FGameplayTagNativeAdder
{
    FGameplayTag Status_Burning;
    FGameplayTag Status_Frozen;

    FORCEINLINE static const FGlobalTags& Get() { return Tags; }

protected:
    virtual void AddTags() override
    {
        UGameplayTagsManager& M = UGameplayTagsManager::Get();
        Status_Burning = M.AddNativeGameplayTag(TEXT("Status.Debuff.Burning"));
        Status_Frozen  = M.AddNativeGameplayTag(TEXT("Status.Debuff.Frozen"));
    }

private:
    static FGlobalTags Tags;
};
```

```cpp
// GlobalTags.cpp
FGlobalTags FGlobalTags::Tags;
```

> **Prefira macros UE_DEFINE_GAMEPLAY_TAG** para novos projetos UE5 — são mais simples e seguras.

---

## Tipos Principais

| Tipo | Uso |
|---|---|
| `FGameplayTag` | Um único tag |
| `FGameplayTagContainer` | Conjunto de tags com queries |
| `FGameplayTagQuery` | Query declarativa serializável (pode ser editada no Editor) |
| `FGameplayTagCountContainer` | Container com contagem (usado no GAS, `FGameplayTagCountContainer`) |

---

## Consultas: FGameplayTag

```cpp
FGameplayTag Tag = TAG_Status_Debuff_Burning;

// Validade
bool bValido = Tag.IsValid();

// Nome completo como FName
FName Nome = Tag.GetTagName(); // "Status.Debuff.Burning"

// Comparação hierárquica (inclui parents)
bool bMatch      = Tag.MatchesTag(TAG_Status_Debuff_Burning);   // true: exato
bool bMatchPai   = Tag.MatchesTag(OutroTag_Status);             // true: pai

// Comparação exata (sem herança de hierarquia)
bool bExato      = Tag.MatchesTagExact(TAG_Status_Debuff_Burning); // true
bool bExatoPai   = Tag.MatchesTagExact(OutroTag_Status);           // false

// Verifica se é pai de outro
bool bEhPai      = TAG_Status.MatchesTag(TAG_Status_Debuff_Burning); // true
```

---

## Consultas: FGameplayTagContainer

```cpp
FGameplayTagContainer Container;
Container.AddTag(TAG_Status_Debuff_Burning);
Container.AddTag(TAG_Status_Buff_Haste);

// HasTag — hierárquico (inclui parents implícitos)
bool b1 = Container.HasTag(TAG_Status_Debuff_Burning); // true
bool b2 = Container.HasTag(TAG_Status);                // true (parent!)
bool b3 = Container.HasTag(TAG_Ability_Blocked);       // false

// HasTagExact — apenas o tag exato, sem herança
bool b4 = Container.HasTagExact(TAG_Status);           // false
bool b5 = Container.HasTagExact(TAG_Status_Buff_Haste); // true

// HasAny / HasAll
FGameplayTagContainer Query;
Query.AddTag(TAG_Status_Debuff_Burning);
Query.AddTag(TAG_Ability_Blocked);

bool bQualquer  = Container.HasAny(Query);    // true  (tem Burning)
bool bTodos     = Container.HasAll(Query);    // false (não tem Ability.Blocked)

// HasAnyExact / HasAllExact — versões sem herança
bool bQualquerE = Container.HasAnyExact(Query);
bool bTodosE    = Container.HasAllExact(Query);

// Remoção
Container.RemoveTag(TAG_Status_Debuff_Burning);
Container.Reset(); // limpa tudo
```

---

## FGameplayTagQuery (queries declarativas/serializáveis)

Útil quando a lógica de query precisa ser editável no Editor por designers.

```cpp
// Montar via código
FGameplayTagQuery Query = FGameplayTagQuery::MakeQuery_MatchAnyTags(
    FGameplayTagContainer::CreateFromArray({
        TAG_Status_Debuff_Burning,
        TAG_Status_Debuff_Frozen
    })
);

bool bPassou = Query.Matches(Container);

// Também disponível: MakeQuery_MatchAllTags, MakeQuery_MatchNoTags
// E queries combinadas com And/Or/Not:
FGameplayTagQuery QueryComplexo;
QueryComplexo.Build(FGameplayTagQueryExpression()
    .AllTagsMatch()
        .AddTag(TAG_Status_Debuff_Burning)
    .NoTagsMatch()
        .AddTag(TAG_Ability_Blocked)
);
```

**UPROPERTY para o editor:**

```cpp
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tags")
FGameplayTagQuery ActivationQuery;
```

---

## UPROPERTY com Filtro por Categoria

Restringe o picker do Editor a uma sub-árvore específica:

```cpp
// Mostra apenas tags filhas de "Status"
UPROPERTY(EditAnywhere, meta = (Categories = "Status"))
FGameplayTag StatusTag;

// Mostra apenas tags filhas de "Ability"
UPROPERTY(EditAnywhere, meta = (Categories = "Ability"))
FGameplayTagContainer AbilityTags;

// Filtragem em parâmetro de UFUNCTION
UFUNCTION(BlueprintCallable, meta = (GameplayTagFilter = "Status"))
void AplicarStatus(FGameplayTag Tag);
```

---

## Replicação

### Fast Replication (recomendado)

Replica tags por índice em vez de FName completo — mais barato na rede.

```ini
; DefaultGameplayTags.ini
[/Script/GameplayTags.GameplayTagsSettings]
bFastReplication=true
```

> ⚠️ Requer lista de tags **idêntica** entre servidor e clientes.
> Não misture tags carregadas dinamicamente com Fast Replication.

### CommonlyReplicatedTags

Tags frequentes recebem índice menor → cabem em menos bits:

```ini
[/Script/GameplayTags.GameplayTagsSettings]
CommonlyReplicatedTags=(TagName="Status.Debuff.Burning")
CommonlyReplicatedTags=(TagName="Status.Buff.Haste")
CommonlyReplicatedTags=(TagName="Ability.Blocked")
```

Use o console em jogo para descobrir quais tags replicar mais:

```
GameplayTags.PrintReplicationFrequencyReport
```

---

## Redirect: Renomear Tags com Segurança

Quando renomear um tag, adicione um redirect para não quebrar assets salvos:

```ini
; DefaultGameplayTags.ini
[/Script/GameplayTags.GameplayTagsSettings]
GameplayTagRedirects=(OldTagName="Status.OnFire",NewTagName="Status.Debuff.Burning")
```

---

## Reagir a Mudanças de Tag

```cpp
// Registrar callback quando um tag específico mudar no ASC
AbilitySystemComponent->RegisterGameplayTagEvent(
    TAG_Status_Debuff_Burning,
    EGameplayTagEventType::NewOrRemoved
).AddUObject(this, &UMinhaClas::OnBurningTagChanged);

void UMinhaClasse::OnBurningTagChanged(const FGameplayTag Tag, int32 NewCount)
{
    if (NewCount > 0)
    {
        // Tag foi adicionada
    }
    else
    {
        // Tag foi removida
    }
}
```

---

## Convenções de Nomenclatura

| Categoria | Exemplos |
|---|---|
| Estado do personagem | `Status.Buff.Haste`, `Status.Debuff.Burning`, `Status.Dead` |
| Habilidades | `Ability.Active.Sprint`, `Ability.Blocked` |
| Input | `Input.Pressed.Jump`, `Input.Held.Sprint` |
| Gameplay Events | `Event.Combat.Hit`, `Event.Item.Pickup` |
| UI | `UI.Menu.Open`, `UI.HUD.Visible` |
| Dano | `Damage.Type.Fire`, `Damage.Type.Physical` |
| Variáveis de GAS | `SetByCaller.Duration`, `SetByCaller.Magnitude` |

**Convenção de variável C++:**
```
TAG_<Nivel1>_<Nivel2>_<Nivel3>
TAG_Status_Debuff_Burning   →   "Status.Debuff.Burning"
TAG_Ability_Blocked         →   "Ability.Blocked"
```

---

## Boas Práticas

### ✅ Faça

- Use `UE_DEFINE_GAMEPLAY_TAG` / `UE_DECLARE_GAMEPLAY_TAG_EXTERN` para tags nativas no UE5
- Agrupe tags por domínio em um `GameTags.h` central por feature/módulo
- Use `FGameplayTagContainer` no lugar de `TArray<FGameplayTag>`
- Ative `bFastReplication` em projetos multiplayer
- Adicione `CommonlyReplicatedTags` para tags replicadas com frequência
- Sempre adicione `DevComment` no INI para documentar a intenção da tag
- Use `meta = (Categories = "X")` para guiar designers no Editor
- Ao renomear, sempre crie um `GameplayTagRedirects`

### ❌ Evite

- `FGameplayTag::RequestGameplayTag(FName(...))` em hot paths — é mais lento e sujeito a race conditions se chamado cedo demais
- Criar tags dentro de construtores de objetos — o dicionário pode não estar pronto
- Tags hardcoded como `FName` ou `FString` espalhadas pelo código
- Uso de `TArray<FGameplayTag>` — prefira `FGameplayTagContainer`
- Duplicar a mesma lógica tag ↔ bool quando `FGameplayTagContainer` resolve
- Confundir `HasTag` (hierárquico) com `HasTagExact` (exato) — erros silenciosos

---

## Snippets Rápidos

```cpp
// Obter tag do manager em runtime (evite em hot path)
FGameplayTag Tag = UGameplayTagsManager::Get()
    .RequestGameplayTag(FName("Status.Debuff.Burning"));

// Converter string → tag (apenas em ferramentas/editor)
FGameplayTag FromString = FGameplayTag::RequestGameplayTag(
    FName(*TagString), /*ErrorIfNotFound=*/false);

// Iterar tags de um container
for (const FGameplayTag& T : MeuContainer)
{
    UE_LOG(LogTemp, Log, TEXT("Tag: %s"), *T.ToString());
}

// Checar se container é subconjunto de outro
bool bSubset = ContainerA.HasAll(ContainerB); // B ⊆ A

// Criar container inline
FGameplayTagContainer Inline = FGameplayTagContainer::CreateFromArray(
    TArray<FGameplayTag>{ TAG_Status_Debuff_Burning, TAG_Ability_Blocked });

// Tag vazia / inválida
FGameplayTag Vazia = FGameplayTag::EmptyTag;
ensure(!Vazia.IsValid());
```

---

## Depuração

```
; Console commands
GameplayTags.PrintReplicationFrequencyReport   ; tags mais replicadas
GameplayTags.DumpTagList                        ; lista todas as tags registradas
showdebug abilitysystem                         ; visualiza tags ativas no ASC
```

---

## Referências

- [UE Docs — Using Gameplay Tags](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-gameplay-tags-in-unreal-engine)
- [Tom Looman — GameplayTags & Data-Driven Design](https://tomlooman.com/unreal-engine-gameplaytags-data-driven-design/)
- [The Games Dev — Native Gameplay Tags UE5](https://www.thegames.dev/?p=106)
- [GAS Documentation — tranek](https://github.com/tranek/GASDocumentation)
- [itsBaffled — GameplayTags & FNames In Depth](https://itsbaffled.github.io/posts/UE/GameplayTags-And-FNames-In-Depth)