#pragma once

#include "CoreMinimal.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Items/MasterItemStructs.h"
#include "MasterItemDataAsset.generated.h"

/**
 * Тут дизайнер заполняет данные предмета (конфиги).
 * Runtime значения (CurrDurability/CurrCharge/Stack) живут в ItemObject.
 */
UCLASS(BlueprintType)
class UMasterItemDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Details")
	FMasterItemDetails ItemDetails;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Trade")
	FItemTradeConfig TradeConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Durability")
	FItemDurabilityConfig DurabilityConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Charge")
	FItemChargeConfig ChargeConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Stats|Outfit")
	FItemOutfitStatsConfig OutfitStatsConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Stats|Weapon")
	FItemWeaponsStatsConfig WeaponStatsConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Stats|Magazine")
	FItemMagazineConfig MagazineConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Stats|Consumable")
	FConsumablesStats ConsumablesStats;

	// Чтобы в PrimaryAsset системе было красиво
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		// Type = "MasterItem", Name = AssetName
		return FPrimaryAssetId(TEXT("MasterItem"), GetFName());
	}
};