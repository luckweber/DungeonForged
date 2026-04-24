// Source/DungeonForged/Public/Merchant/DFMerchantData.h
#pragma once

#include "CoreMinimal.h"
#include "Data/DFDataTableStructs.h"
#include "Engine/DataTable.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DFMerchantData.generated.h"

/** One row in DT_MerchantStock: eligible items and pricing rules. */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFMerchantStockRow : public FTableRowBase
{
	GENERATED_BODY()

	/** FName of a row in DT_Items. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Merchant")
	FName ItemRowName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Merchant", meta = (ClampMin = "0"))
	int32 BasePrice = 10;

	/** 0.2 = random multiplier in [1-0.2, 1+0.2] for this slot's price. If 0, uses 0.8–1.2. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Merchant", meta = (ClampMin = "0.0", ClampMax = "0.99"))
	float PriceVariance = 0.1f;

	/** Item in DT_Items must be at least this rarity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Merchant")
	EItemRarity MinRarity = EItemRarity::Common;

	/** This row only appears when CurrentRunFloor >= this value. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Merchant", meta = (ClampMin = "0"))
	int32 MinFloorAvailable = 1;

	/** If true, quantity resets when the run advances a floor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Merchant")
	bool bIsConsumable = false;

	/** -1 = unlimited. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Merchant", meta = (ClampMax = "9999"))
	int32 StockQuantity = 1;
};

/** One generated offer slot; replicated on the merchant. */
USTRUCT(BlueprintType)
struct DUNGEONFORGED_API FDFMerchantStockEntry
{
	GENERATED_BODY()

	/** Key in DT_MerchantStock (row name) for this pick. */
	UPROPERTY(BlueprintReadOnly, Category = "Merchant")
	FName MerchantStockRowKey = NAME_None;

	/** FName in DT_Items. */
	UPROPERTY(BlueprintReadOnly, Category = "Merchant")
	FName ItemRowName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Merchant", meta = (ClampMin = "0"))
	int32 UnitPrice = 0;

	/** -1 = unlimited. */
	UPROPERTY(BlueprintReadOnly, Category = "Merchant")
	int32 Quantity = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Merchant")
	bool bIsConsumable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Merchant")
	bool bUnlimited = false;
};

/** Linear colors for FCT / UI. */
UCLASS()
class DUNGEONFORGED_API UDFMerchantUIStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "DF|Merchant|UI")
	static FLinearColor RarityToColor(const EItemRarity R);

	UFUNCTION(BlueprintPure, Category = "DF|Merchant|UI")
	static FText RarityToDisplayName(const EItemRarity R);
};
