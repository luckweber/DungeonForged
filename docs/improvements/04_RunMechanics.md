# 04 — Run Mechanics

> **Objetivo:** que uma run completa (≈ 30 min) tenha **pacing crescente**, decisões significativas entre andares, dificuldade que escala com o player, e recompensas que criam momentum.

---

## Sumário rápido

| Eixo | Atual | Alvo | Esforço |
|---|---|---|---|
| Enemy scaling por andar | **confirmar se existe** | `BaseHP × (1 + 0.15 × Floor) × DM` | 1h |
| Event chance | 40% fixo | 25/40/60% por terço, 0% no boss | 30min |
| Ability draft | toda transição | alternar com shrines passivos | 4h |
| Floor count | 10 fixo | 10 default; **Daily/Heat** ajustam | 1h |
| Run length target | desconhecido | 25-40 min | playtest |
| Currency drops | only end of fight | rolling small drops | 2h |
| Risk/reward rooms | sem | elite room, treasure room, shop room | 6h |
| Boss positioning | fim do floor 10 | fim do floor 5 (mini-boss) + 10 (final) | 4h |

---

## 1. Scaling de inimigos — `[CONFIG/CODE]` <a id="scaling"></a>

**🔥 Crítico — verificar primeiro.**

**Onde:** [`Source/DungeonForged/Private/Characters/ADFEnemyBase.cpp:502`](../../Source/DungeonForged/Private/Characters/ADFEnemyBase.cpp#L502) — `ApplyBaseStatsFromRow`

```cpp
void ADFEnemyBase::ApplyBaseStatsFromRow(const FDFEnemyTableRow& Row)
{
    // ...
    const float Hp = FMath::Max(1.f, Row.BaseHealth);   // ← não escala por floor!
    S->SetMaxHealth(Hp);
    S->SetHealth(Hp);
```

### 1.1 Corrigir

```cpp
void ADFEnemyBase::ApplyBaseStatsFromRow(const FDFEnemyTableRow& Row)
{
    if (!AbilitySystemComponent) return;
    UDFAttributeSet* const S = const_cast<UDFAttributeSet*>(AbilitySystemComponent->GetSet<UDFAttributeSet>());
    if (!S) return;

    int32 Floor = 0;
    float DM = 1.f;
    if (UWorld* const W = GetWorld())
    {
        if (UGameInstance* const GI = W->GetGameInstance())
        {
            if (UDFDungeonManager* const Dm = GI->GetSubsystem<UDFDungeonManager>())
            {
                Floor = Dm->CurrentFloor;
                DM    = Dm->GetCurrentDifficultyMultiplier();  // do DT_Dungeon row
            }
        }
    }
    const float FloorScale = 1.f + 0.15f * static_cast<float>(Floor);   // [CONFIG]
    const float ScaleFinal = FloorScale * DM;

    const float Hp = FMath::Max(1.f, Row.BaseHealth * ScaleFinal);
    S->SetMaxHealth(Hp);
    S->SetHealth(Hp);
    S->SetArmor(Row.BaseArmor * FMath::Sqrt(ScaleFinal));   // armor escala mais devagar
    S->SetStrength(FMath::Max(0.f, Row.BaseDamage * ScaleFinal));
}
```

### 1.2 Tabela de previsão (BaseHealth = 100, DM = 1.0)

| Floor | Scale | HP final |
|---|---|---|
| 1 | 1.15 | 115 |
| 3 | 1.45 | 145 |
| 5 | 1.75 | 175 |
| 7 | 2.05 | 205 |
| 9 | 2.35 | 235 |
| 10 (boss) | 2.50 + boss bonus | 4000+ (boss tier) |

### 1.3 DT_Dungeon ajustes

`FDFDungeonFloorRow::DifficultyMultiplier` deve ser usado **adicionalmente** à curva linear. Sugestão:

| Floor | Tipo | DM | Comentário |
|---|---|---|---|
| 1-3 | "Cripta" | 0.9 | introdução, dano "perdoa" |
| 4-6 | "Catacumbas" | 1.0 | baseline |
| 7-9 | "Profundezas" | 1.2 | tension building |
| 10 | "Câmara do Boss" | 1.4 | climax |

---

## 2. Random Event pacing — `[CODE]` <a id="event-pacing"></a>

**Onde:** [`Source/DungeonForged/Public/Events/UDFRandomEventSubsystem.h:71`](../../Source/DungeonForged/Public/Events/UDFRandomEventSubsystem.h#L71)

```cpp
float EventChancePerFloor = 0.4f;   // ← fixo
```

### 2.1 Curva por andar

Substituir por:

```cpp
UPROPERTY(EditAnywhere, Category="Events")
TArray<float> EventChancePerFloorCurve = {
    0.25f, 0.25f, 0.25f,   // floors 1-3: leve, foco em learning combat
    0.40f, 0.40f, 0.40f,   // floors 4-6: build-defining decisions
    0.60f, 0.60f, 0.60f,   // floors 7-9: jogador investido = mais decisões
    0.00f                  // floor 10: boss, sem distração
};

float UDFRandomEventSubsystem::GetEventChanceForFloor(int32 Floor) const
{
    if (EventChancePerFloorCurve.IsEmpty()) return EventChancePerFloor;
    const int32 Idx = FMath::Clamp(Floor - 1, 0, EventChancePerFloorCurve.Num() - 1);
    return EventChancePerFloorCurve[Idx];
}

bool UDFRandomEventSubsystem::ShouldTriggerEvent(int32 Floor) const
{
    return FMath::FRand() < FMath::Clamp(GetEventChanceForFloor(Floor), 0.f, 1.f);
}
```

### 2.2 Distribuição esperada por run de 10 andares

```
floors 1-3:  3 × 25% = 0.75 events
floors 4-6:  3 × 40% = 1.20 events
floors 7-9:  3 × 60% = 1.80 events
floor 10:    0
─────────────────────────────────
expected total: ~3.75 events/run
```

Antes (fixo 40%): 3.6 events/run, mas **distribuído homogeneamente**. Agora: clustering na segunda metade, onde decisões importam mais.

### 2.3 Evitar repetição

Manter `LastEventRowName` em `UDFRunManager` — eventos não repetem 2x na mesma run.

```cpp
TArray<FName> SeenEventsThisRun;
// ao pegar event:
FName Pick;
do {
    Pick = WeightedRandomFromTable();
} while (SeenEventsThisRun.Contains(Pick) && SeenEventsThisRun.Num() < EventTable->GetRowMap().Num() - 1);
SeenEventsThisRun.Add(Pick);
```

---

## 3. Ability draft — variedade de recompensa `[CODE]`

**Onde:** [`Source/DungeonForged/Public/UI/UDFAbilitySelectionSubsystem.h`](../../Source/DungeonForged/Public/UI/UDFAbilitySelectionSubsystem.h)

Atual: toda transição entre andares oferece **draft 1-of-3 de ability**. Vira "build inflation".

### 3.1 Padrão Hades — diversidade de room rewards

Em vez de **sempre** ability, sortear o tipo de reward:

```cpp
UENUM(BlueprintType)
enum class ERoomRewardType : uint8
{
    AbilityDraft,    // 1-of-3 ability  (40%)
    PassiveShrine,   // 1-of-3 stat boost permanente (20%)
    Gold,            // 50-200 gold     (15%)
    HealingFountain, // 30-50% HP heal  (10%)
    Item,            // 1 consumable    (10%)
    Mystery,         // ?               (5%)
};
```

Distribuição **por andar**:

| Floor range | Ability | Shrine | Gold | Heal | Item | Mystery |
|---|---|---|---|---|---|---|
| 1-3 | 60% | 10% | 15% | 10% | 5% | 0% |
| 4-6 | 40% | 25% | 10% | 10% | 10% | 5% |
| 7-9 | 25% | 30% | 15% | 10% | 10% | 10% |

Floor 10 não tem reward intermediário (segue boss direto).

### 3.2 Shrine passivo — exemplos

Cada shrine é um `UPrimaryDataAsset` com `FGameplayEffectSpec` permanent durante a run:

| Shrine | Effect |
|---|---|
| **Bastion** | +50 max HP |
| **Spire** | +10 Strength |
| **Wisdom** | +10 Intelligence |
| **Swift** | +10 Agility |
| **Fortune** | +5% Crit Chance |
| **Ire** | +0.2 Crit Multiplier |
| **Aether** | +10% Spell Damage Amp |
| **Bulwark** | +50 Armor |
| **Veil** | +50 Magic Resist |
| **Flow** | +15% Cooldown Reduction (cap respected) |

Player escolhe 1 de 3 oferecidos. Mesmo padrão de rarity weights.

### 3.3 Healing fountain

Heal 40% MaxHealth. Restaura 25% Mana. Limitar a **1 fountain a cada 3 andares** para não banalizar.

### 3.4 Item drop

`UDFInventoryComponent` adiciona um consumable rolado por raridade. Exemplos:
- Potion of Resurrection (auto-revive 1×)
- Smoke Bomb (escape from combat, +invis 3s)
- Phoenix Down (auto-heal 50% se HP < 10%, consumido)

---

## 4. Floor types — variedade `[CODE/CONFIG]`

Atualmente cada andar tem `EnemyRows[]` + opcional boss. Sugestão de **room subtypes**:

### 4.1 Room subtypes

```cpp
UENUM(BlueprintType)
enum class ERoomKind : uint8
{
    Standard,         // 4-6 inimigos comuns
    Elite,            // 1 elite + 2 minions (música muda)
    Treasure,         // 1 chest + 1 trap + 2 inimigos
    Shop,             // merchant temporary + 3 itens
    Trial,            // wave-based (3 waves) com reward maior
    Encounter,        // narrative event NPC + diálogo
    BossLair,         // exclusivo do boss
};
```

`FDFDungeonFloorRow` tem `RoomKind` ou similar — se não, adicionar.

### 4.2 Distribuição esperada

Em uma run de 10 andares:
```
2× Standard
2× Standard
1× Elite       ← floor 4 ou 5
1× Treasure    ← floor 3 ou 6
1× Shop        ← floor 5 ou 7
1× Trial       ← floor 7 ou 8
1× Encounter   ← random qualquer
1× BossLair    ← floor 10
```

Procedural / seeded para variedade. Daily seed = mesma sequência.

### 4.3 Trial room — wave-based

```cpp
class ADFTrialRoom : public AActor
{
    UPROPERTY(EditAnywhere) TArray<FDFWaveSpec> Waves;  // 3 waves típicas
    UPROPERTY(EditAnywhere) TSoftClassPtr<UPrimaryDataAsset> TrialReward;

    void StartWave(int32 Index);
    void OnWaveCleared(int32 Index);
    void OnAllWavesCleared();  // unlock reward chest
};
```

Music state especial: `Combat_Intense` durante todas as waves.

### 4.4 Shop room temporary

Mini-merchant durante uma run (diferente do Nexus). 3 slots, rolados por floor tier:
- Floor 1-3: comuns/uncommons
- Floor 4-6: uncommons/rare
- Floor 7-9: rare/epic

Compra com gold da run. Saída do shop fecha o widget e segue.

---

## 5. Currency drops — `[CODE]`

### 5.1 Rolling small drops

Atualmente o gold cai apenas no fim do andar / morte de inimigo. Sugerir **drops menores per kill** + bulk no end-of-floor:

```cpp
// ADFEnemyBase::HandleServerDeath
if (CachedGoldDropMax > 0)
{
    const int32 Gold = FMath::RandRange(
        FMath::Min(CachedGoldDropMin, CachedGoldDropMax),
        FMath::Max(CachedGoldDropMin, CachedGoldDropMax));
    if (Gold > 0)
    {
        SpawnGoldPickupActor(GetActorLocation(), Gold);  // pickup ao tocar
    }
}
```

Spawnar `ADFGoldPickup` no mundo (sprite com pulso, magnet-to-player a 200cm). Player **anda em cima** = picks up = SFX coin + +X gold pop em combat text.

### 5.2 Bigger drops em rooms especiais

- Elite kill: 100-300 gold
- Treasure room chest: 200-500
- Boss kill: 500-1500
- Daily seed bonus: 2× gold drops

---

## 6. Difficulty heat / daily — `[CODE]`

### 6.1 Heat system (Hades-style)

Após beat o boss 1x, unlock **modifiers opcionais**:

```cpp
USTRUCT(BlueprintType)
struct FDFHeatModifier
{
    UPROPERTY(EditAnywhere) FName ModifierName;          // "Lethal Boss"
    UPROPERTY(EditAnywhere) FText Description;          // "Boss damage +50%"
    UPROPERTY(EditAnywhere) int32 HeatValue = 1;
    UPROPERTY(EditAnywhere) TSubclassOf<UGameplayEffect> EffectClass;
};
```

Player ativa N modifiers totalizando heat X → reward escalado (mais gold, MetaXP).

Exemplos:
- **Lethal Boss**: boss dmg +50% (heat 2)
- **Crowded**: +2 inimigos por sala (heat 2)
- **Hard Bosses**: boss HP +30% (heat 1)
- **Anemia**: max HP -25% (heat 3)
- **Mortal**: revives -1 (heat 2)
- **Speedrun**: floor timer 4 min, falha = next floor mais difícil (heat 3)

### 6.2 Daily seed

Hoje (`2026-05-18`) → seed determinístico do RNG (`Hash(Date)`):
- Mesmo floor layout
- Mesmos events
- Mesmos drops
- Leaderboard daily

Implementação:
```cpp
FRandomStream UDFRunManager::CreateDailyStream() const
{
    const FDateTime Today = FDateTime::UtcNow();
    const int32 Seed = (Today.GetYear() * 10000) + (Today.GetMonth() * 100) + Today.GetDay();
    return FRandomStream(Seed);
}
```

Run normal = `FRandomStream(FPlatformTime::Cycles())`.

---

## 7. Tempo de uma run — target `[CONFIG]`

### 7.1 Estimativa por andar

Sugestão de timing alvo:

| Floor | Activities | Tempo alvo |
|---|---|---|
| 1 | 4-5 inimigos comuns | 1-2 min |
| 2 | 5 inimigos + trap | 1.5-2.5 min |
| 3 | Treasure room | 2-3 min |
| 4 | 6 inimigos | 2-2.5 min |
| 5 | Elite + 2 minions | 3-4 min |
| 6 | Shop + 5 inimigos | 3-4 min |
| 7 | Trial (3 waves) | 4-5 min |
| 8 | 7 inimigos + 2 traps | 3-4 min |
| 9 | Encounter + 5 inimigos | 3-4 min |
| 10 | Boss (3 phases) | 4-6 min |
| **Total** | | **27-38 min** |

### 7.2 Como medir

`UDFRunManager::RunStartTime` (já existe) → `RunDurationSeconds`. Logar a cada floor cleared:

```cpp
UE_LOG(LogDFRun, Log, TEXT("Floor %d cleared at %.1fs (delta %.1fs)"),
       Floor, ElapsedTotal, ElapsedThisFloor);
```

E mostrar no `UDFRunRecapWidget` (post-victory): "Run time: 32:14".

### 7.3 Speedrun mode

Toggle no main menu: ativar timer constante on-screen + leaderboard. **Não afeta drop rates** (anti-cheese).

---

## 8. Between-floors flow — UX `[CODE/UI]`

Atualmente entre floors: portal → level-up screen (if applicable) → event → ability draft → portal next.

### 8.1 Order sugerido

```
1. End-of-floor summary (3s auto-pass):
   • "Floor 4 cleared"
   • "+50 XP, +120 gold, 3 kills, 12% HP lost"
   • Speedrun: "Best floor time: 1:54"

2. Level-up (se aplicável):
   • spend point UI

3. Event (se rolado):
   • EventChoice UI

4. Room reward (sempre):
   • depending on ERoomRewardType

5. "Press SPACE to descend" prompt
```

Cada etapa **skippable** (não bloqueia) com `[ESC] skip all`.

### 8.2 Save checkpoint

Após cada floor cleared, `UDFWorldTransitionSubsystem::SaveCheckpoint(Floor, Class, Abilities, ...)`. Crash recovery = resume desta etapa.

---

## 9. Death recovery — opcional `[CODE]`

### 9.1 Revive system

Quando player morre, **se tem `Phoenix Down` no inventário** → consome auto, ressuscita com 50% HP. Já implementável via `OnOutOfHealth` listener.

Cap: 1 revive por floor.

### 9.2 Death = MetaXP bonus

Ao morrer: `MetaXP += (FloorReached × 10) + (Kills / 2)`. Falha não é zero progresso. Hades faz isso.

### 9.3 Death cause — bem nominado

Já existe `DefeatCause` string no defeat screen. Garantir que **toda fonte de dano populates**:

```cpp
void ADFPlayerCharacter::HandlePlayerOutOfHealth()
{
    FString Cause;
    if (LastDamageInstigator.IsValid())
    {
        if (const ADFBossBase* B = Cast<ADFBossBase>(LastDamageInstigator.Get()))
            Cause = FString::Printf(TEXT("Defeated by %s"), *B->GetBossDisplayName().ToString());
        else if (const ADFEnemyBase* E = Cast<ADFEnemyBase>(LastDamageInstigator.Get()))
            Cause = FString::Printf(TEXT("Slain by %s"), *E->GetClass()->GetName());
        else if (Cast<ADFTrapBase>(LastDamageInstigator.Get()))
            Cause = TEXT("Killed by a trap");
        else
            Cause = TEXT("Slain in combat");
    }
    Client_OpenDefeatScreen(RunSummary, Cause);
}
```

---

## 10. Boss positioning — `[CONFIG]`

### 10.1 Mini-boss no andar 5

Sugestão: cada run tem **dois encontros climáticos** — mini-boss no andar 5 + boss final no andar 10.

```cpp
// FDFDungeonFloorRow
bool bHasMiniBoss = false;
FName MiniBossEnemyRow;
```

Mini-boss:
- HP 4× enemy normal do andar
- 1 phase, 1 signature ability (ex.: AOE telegraphed)
- Music: `BossLayer` short, but no enrage timer
- Reward: guaranteed Epic ability draft (skip slot indisponível)

Cria pico de tensão no meio da run.

### 10.2 Boss final no andar 10

Mantém. 3 fases, enrage timer. Já implementado.

### 10.3 Boss roster

Atualmente 1 boss (presumido). Sugiro 3-4 bosses **rotativos por run** — cada run sorteia um:

| Boss | Estilo | Signature abilities |
|---|---|---|
| **Necromancer Lord** | Caster + summons | Meteor, summon undead, void barrier |
| **Iron Knight** | Melee tank | Charge, ground slam, terror shout |
| **Hydra Twin** | 2 alvos | Coordinated attacks, phase swap |
| **Voidcaller** | Elemental | Chain elements, phase transition vortex |

Após beat 1 boss, próximas runs sorteiam entre os restantes (variety).

---

## 11. Meta-progressão e run incentives `[CODE]`

### 11.1 Quests da run

Adicionar 3 quests gerados aleatoriamente por run (estilo Path of Exile sextants):
- "Kill 30 enemies in this run" → +200 gold ao Nexus
- "Don't take damage in 1 floor" → +50 MetaXP
- "Complete in under 30 min" → unlock cosmetic
- "Kill boss without dodging" → +100 MetaXP

UI: 3 ícones no canto superior direito + progress bar.

### 11.2 Streak bonus

Vitórias consecutivas dão multiplicador de MetaXP:
- 1st win: 1.0×
- 2nd consecutive: 1.2×
- 3rd: 1.5×
- 5th: 2.0× (cap)

Reset em derrota.

### 11.3 Pity timer

Após 5 derrotas seguidas sem chegar ao boss, próxima run **começa com +1 ability comum extra** (encourage retention).

---

## 12. Checklist de "pronto"

- [ ] Enemy stats escalam com floor (`1 + 0.15 × Floor`) + DifficultyMultiplier.
- [ ] Boss kill no floor 10 ainda dá HP 2.5× × boss bonus.
- [ ] Event chance segue curva 25/40/60/0 por terço.
- [ ] Events não repetem na mesma run.
- [ ] Room rewards rotacionam (ability/shrine/gold/heal/item).
- [ ] Pelo menos 5 shrines passivos implementados.
- [ ] Floor types incluem Treasure, Shop, Elite (mini-boss no 5), Trial.
- [ ] Gold drops são pickups no mundo (rolling small + bulk).
- [ ] Heat modifiers desbloqueiam após primeira vitória.
- [ ] Daily seed implementado e leaderboard local.
- [ ] Run total demora 25-40 min em playtest (medir e ajustar).
- [ ] Death cause é específico em 100% dos casos.
- [ ] Streak bonus aplica MetaXP multiplier corretamente.

---

## Apêndice — fórmulas de pacing

### Curva de excitação esperada (ASCII)

```
Excitement
   ▲
1.0│                                              ╱╲ Boss
   │                                          ╱╲╱
0.8│                                     ╱╲╱        \
   │                              Trial╱             \ enrage
0.6│                  Mini-boss ╱╲    ╱
   │              ╱╲╱       \  ╱
0.4│        Treasure        Shop
   │     ╱
0.2│  ╱ tutorial
   │╱
0.0└─┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───►
     F1  F2  F3  F4  F5  F6  F7  F8  F9  F10
```

- Floor 1-3: aprendizagem (low)
- Floor 4-5: spike (mini-boss + elite)
- Floor 6: calm (shop)
- Floor 7-8: rising (trial + crowded)
- Floor 10: climax (boss + enrage)

Se o player **sente** essa curva, o pacing está certo.
