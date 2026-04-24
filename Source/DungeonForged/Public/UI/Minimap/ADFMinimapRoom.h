// Source/DungeonForged/Public/UI/Minimap/ADFMinimapRoom.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ADFMinimapRoom.generated.h"

class UBoxComponent;
class UTexture2D;

UENUM(BlueprintType)
enum class EDFRoomType : uint8
{
	Normal		UMETA(DisplayName = "Normal"),
	Elite		UMETA(DisplayName = "Elite"),
	Boss		UMETA(DisplayName = "Boss"),
	Treasure	UMETA(DisplayName = "Treasure"),
	Merchant	UMETA(DisplayName = "Merchant"),
	Start		UMETA(DisplayName = "Start"),
	Exit		UMETA(DisplayName = "Exit"),
};

/**
 * Placed in each dungeon room. Overlap (player fog sphere) drives reveal/visit; neighbors drive lookahead.
 */
UCLASS(Blueprintable)
class DUNGEONFORGED_API ADFMinimapRoom : public AActor
{
	GENERATED_BODY()

public:
	ADFMinimapRoom();

	UFUNCTION(BlueprintCallable, Category = "DF|Minimap|Room")
	void RevealRoom();

	UFUNCTION(BlueprintCallable, Category = "DF|Minimap|Room")
	void VisitRoom();

	/** If null, the minimap widget uses its EDFRoomType → texture map. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DF|Minimap|Room")
	UTexture2D* GetIconTexture() const;
	UTexture2D* GetIconTexture_Implementation() const;

	/** True when any NeighborRooms has bIsRevealed (for fog-of-war lookahead silhouettes). */
	UFUNCTION(BlueprintPure, Category = "DF|Minimap|Room")
	bool IsAdjacentToRevealed() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DF|Minimap|Room")
	TObjectPtr<UBoxComponent> RoomBounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Minimap|Room")
	EDFRoomType RoomType = EDFRoomType::Normal;

	UPROPERTY(BlueprintReadOnly, Category = "DF|Minimap|Room")
	uint8 bIsRevealed : 1 = false;

	UPROPERTY(BlueprintReadOnly, Category = "DF|Minimap|Room")
	uint8 bIsVisited : 1 = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Minimap|Room")
	FVector RoomCenter = FVector::ZeroVector;

	/** X/Y = full width/height in world units (Z extent of the box is fixed in ctor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Minimap|Room")
	FVector2D RoomSize = FVector2D(2000.0, 2000.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Minimap|Room")
	FText RoomDisplayName;

	/** One-room connections for adjacency (lookahead). Set per-room in the editor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Minimap|Room")
	TArray<TObjectPtr<ADFMinimapRoom>> NeighborRooms;

	/** Optional per-room art (boss skull, chest, etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DF|Minimap|Room|Icon")
	TObjectPtr<UTexture2D> IconOverride = nullptr;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
