---
name: unreal-asset-manager
description: >
  Guia completo para usar o AssetManager do Unreal Engine 5.4 em C++.
  Use este skill sempre que o usuário trabalhar com AssetManager, Primary Assets,
  async loading, Asset Bundles, UPrimaryDataAsset, FPrimaryAssetId, soft references,
  cooking/chunking de assets, ou qualquer fluxo de carregamento assíncrono de assets no UE5.
  Inclui padrões de código, configurações de DefaultEngine.ini e boas práticas.
version: 1.0.0
---

# Unreal Engine 5.4 — AssetManager

## Visão Geral

O `UAssetManager` é um singleton global (subclasse de `UObject`) que categoriza, escaneia e carrega/descarrega **Primary Assets** em runtime. Ele integra o `FStreamableManager` internamente e foi projetado para ser estendido por jogos.

**Quando usar o AssetManager:**
- Carregamento assíncrono sem referências hard-coded
- Gerenciamento de memória por contexto (lobby vs in-game)
- Cooking e chunking controlado de assets
- Inventários, itens, personagens, mapas, data assets em escala

---

## Conceitos Fundamentais

### Primary vs Secondary Assets

| Tipo | Descrição | Exemplo |
|---|---|---|
| **Primary Asset** | Gerenciado diretamente pelo AssetManager via `FPrimaryAssetId` | Item, Personagem, Mapa |
| **Secondary Asset** | Carregado automaticamente como dependência de um Primary Asset | Texture, StaticMesh, Material |

Por padrão, apenas `UWorld` (levels) são Primary Assets. Todos os outros precisam ser configurados.

### Estrutura de IDs

```
FPrimaryAssetType  →  FName único por grupo  (ex: "Weapon", "Character")
FPrimaryAssetName  →  FName do asset específico  (ex: "BattleAxe_Tier2")
FPrimaryAssetId    →  Type:Name  →  "Weapon:BattleAxe_Tier2"
```

> ⚠️ O par `Type:Name` deve ser **único em todo o projeto**. Duplicatas causam erros em runtime.

---

## Setup Inicial

### 1. Configurar DefaultEngine.ini

Para usar uma subclasse customizada:

```ini
[/Script/Engine.Engine]
AssetManagerClassName=/Script/MeuJogo.UMeuAssetManager
```

Se não precisar de comportamento customizado, pule essa etapa — o `UAssetManager` padrão já está ativo.

### 2. Registrar Primary Asset Types em Project Settings

Em `Project Settings → Asset Manager`:

```ini
; DefaultGame.ini (gerado pela UI)
[/Script/Engine.AssetManagerSettings]
+PrimaryAssetTypesToScan=(PrimaryAssetType="Item",AssetBaseClass=/Script/MeuJogo.UMeuItem,bHasBlueprintClasses=False,bIsEditorOnly=False,Directories=((Path="/Game/Items")),SpecificAssets=,Rules=(Priority=-1,ChunkId=-1,bApplyRecursively=True,CookRule=Unknown))
```

---

## Criando Primary Assets

### Opção A: Herdar de `UPrimaryDataAsset` (recomendado)

```cpp
// MeuItem.h
#pragma once
#include "Engine/PrimaryAssetLabel.h"
#include "Engine/DataAsset.h"
#include "MeuItem.generated.h"

UCLASS(BlueprintType)
class MEUJOGO_API UMeuItem : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    // UPrimaryDataAsset já implementa GetPrimaryAssetId()
    // basta definir o tipo via GetPrimaryAssetType()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    FText Nome;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    int32 Valor;

    // Soft ref para icon: carregada apenas no bundle "UI"
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI",
              meta = (AssetBundles = "UI"))
    TSoftObjectPtr<UTexture2D> Icone;

    // Soft ref para mesh: carregada apenas no bundle "Game"
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay",
              meta = (AssetBundles = "Game"))
    TSoftObjectPtr<USkeletalMesh> Mesh;

    virtual FPrimaryAssetType GetPrimaryAssetType() const override
    {
        return FPrimaryAssetType("Item");
    }
};
```

### Opção B: Sobrescrever `GetPrimaryAssetId()` em qualquer UObject

```cpp
// MeuPersonagem.h
virtual FPrimaryAssetId GetPrimaryAssetId() const override
{
    return FPrimaryAssetId(FPrimaryAssetType("Character"), GetFName());
}
```

---

## Subclasse Customizada do AssetManager

```cpp
// MeuAssetManager.h
#pragma once
#include "Engine/AssetManager.h"
#include "MeuAssetManager.generated.h"

UCLASS()
class MEUJOGO_API UMeuAssetManager : public UAssetManager
{
    GENERATED_BODY()

public:
    static UMeuAssetManager& Get();

    // Tipo global reutilizável
    static const FPrimaryAssetType ItemType;

    virtual void StartInitialLoading() override;
};
```

```cpp
// MeuAssetManager.cpp
#include "MeuAssetManager.h"
#include "AbilitySystemGlobals.h"

const FPrimaryAssetType UMeuAssetManager::ItemType = TEXT("Item");

UMeuAssetManager& UMeuAssetManager::Get()
{
    UMeuAssetManager* Manager = Cast<UMeuAssetManager>(GEngine->AssetManager);
    check(Manager);
    return *Manager;
}

void UMeuAssetManager::StartInitialLoading()
{
    Super::StartInitialLoading();

    // Inicializações globais aqui
    // Ex: UAbilitySystemGlobals::Get().InitGlobalData();
}
```

---

## Carregamento Assíncrono (Async Loading)

### Padrão correto para async load de um Primary Asset

```cpp
void AMinhaGameMode::CarregarItem(FPrimaryAssetId ItemId)
{
    UMeuAssetManager& Manager = UMeuAssetManager::Get();

    // Lista de bundles a carregar (vazio = carrega tudo)
    TArray<FName> Bundles = { TEXT("Game") };

    FStreamableDelegate Callback = FStreamableDelegate::CreateUObject(
        this, &AMinhaGameMode::OnItemCarregado, ItemId);

    Manager.LoadPrimaryAsset(ItemId, Bundles, Callback);
}

void AMinhaGameMode::OnItemCarregado(FPrimaryAssetId ItemId)
{
    UMeuAssetManager& Manager = UMeuAssetManager::Get();

    // Obter o asset já carregado
    UMeuItem* Item = Cast<UMeuItem>(Manager.GetPrimaryAssetObject(ItemId));
    if (!Item) return;

    // usar Item...

    // ⚠️ Sempre liberar quando não precisar mais!
    Manager.UnloadPrimaryAsset(ItemId);
}
```

### Carregar múltiplos assets ao mesmo tempo

```cpp
void CarregarTodosItens()
{
    TArray<FPrimaryAssetId> IdList;
    Manager.GetPrimaryAssetIdList(UMeuAssetManager::ItemType, IdList);

    TArray<FName> Bundles = { TEXT("UI") };

    Manager.LoadPrimaryAssets(IdList, Bundles,
        FStreamableDelegate::CreateLambda([IdList, &Manager]()
        {
            for (const FPrimaryAssetId& Id : IdList)
            {
                UMeuItem* Item = Cast<UMeuItem>(Manager.GetPrimaryAssetObject(Id));
                // processar...
            }
        }));
}
```

### Consultar asset sem carregá-lo (Asset Registry)

```cpp
// Lê tags registradas SEM carregar o asset na memória
FAssetData AssetData;
Manager.GetPrimaryAssetData(ItemId, AssetData);

FString TagValue;
AssetData.GetTagValue(GET_MEMBER_NAME_CHECKED(UMeuItem, Valor), TagValue);
int32 Valor = FCString::Atoi(*TagValue);
```

---

## Asset Bundles

Bundles permitem carregamento parcial: carregue apenas o ícone na tela de seleção, e o mesh completo quando o item for equipado.

```cpp
// Definição no header (via meta tag)
UPROPERTY(meta = (AssetBundles = "UI"))
TSoftObjectPtr<UTexture2D> Icone;

UPROPERTY(meta = (AssetBundles = "Game"))
TSoftObjectPtr<USkeletalMesh> Mesh;
```

```cpp
// Carregar apenas bundle "UI" (ícone)
Manager.LoadPrimaryAsset(ItemId, { TEXT("UI") }, Callback);

// Mais tarde, atualizar para bundle "Game" (mesh)
Manager.LoadPrimaryAsset(ItemId, { TEXT("Game") }, Callback);
```

---

## Boas Práticas

### ✅ Faça

- Use `TSoftObjectPtr<T>` e `TSoftClassPtr<T>` para referências em Data Assets
- Defina `AssetBundles` nas `UPROPERTY` para controlar granularidade
- Sempre chame `UnloadPrimaryAsset()` quando terminar
- Use `GetPrimaryAssetData()` + AssetRegistry para leitura sem carregamento
- Subclasse o `UAssetManager` para centralizar tipos (`static const FPrimaryAssetType`)
- Use `LogAssetManager` verboso para debug: `log LogAssetManager all`

### ❌ Evite

- Hard references (`UPROPERTY() UTexture2D* Icone`) em Data Assets de inventário — carregam tudo junto
- Chamar `LoadSynchronous()` na game thread principal em runtime
- Duplicar `PrimaryAssetType:PrimaryAssetName` no projeto
- Esquecer de registrar o tipo no `DefaultGame.ini` / Project Settings

---

## Ferramentas de Auditoria

| Ferramenta | Como acessar | Uso |
|---|---|---|
| **Asset Audit** | Content Browser → clique direito → Audit Assets | Tamanho, dependências, chunks |
| **Reference Viewer** | Content Browser → clique direito → Reference Viewer | Grafo de dependências |
| **Size Map** | Content Browser → clique direito → Size Map | Visualização de tamanho em disco |
| **Asset Manager Editor** | Window → Asset Manager | Visão de todos Primary Assets |

---

## Snippets Rápidos

```cpp
// Obter o AssetManager de qualquer lugar
UAssetManager& AM = UAssetManager::Get();

// Listar todos os IDs de um tipo
TArray<FPrimaryAssetId> Ids;
AM.GetPrimaryAssetIdList(TEXT("Item"), Ids);

// Verificar se um asset está carregado
bool bCarregado = AM.GetPrimaryAssetObject(MeuId) != nullptr;

// Construir ID manualmente
FPrimaryAssetId Id(TEXT("Item"), TEXT("Espada_Lendaria"));

// Caminho do asset a partir do ID
FSoftObjectPath Path = AM.GetPrimaryAssetPath(Id);
```

---

## Referências

- [UE Docs — Asset Management](https://dev.epicgames.com/documentation/en-us/unreal-engine/asset-management-in-unreal-engine)
- [Community Wiki — Using the Asset Manager](https://unrealcommunity.wiki/using-the-asset-manager-qj38astq)
- [Tom Looman — Asset Manager & Async Loading](https://tomlooman.com/unreal-engine-asset-manager-async-loading/)
- [Epic Sample — Action RPG](https://www.unrealengine.com/marketplace/en-US/product/action-rpg)