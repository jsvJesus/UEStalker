#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Items/MasterItemStructs.h"
#include "ItemObject.generated.h"

class UMasterItemDataAsset;
class USoundBase;

/**
 * Runtime-экземпляр предмета (в инвентаре).
 * Тут CurrDurability/CurrCharge/Stack/Rotation и т.п.
 */
UCLASS(BlueprintType)
class UItemObject : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
	TObjectPtr<UMasterItemDataAsset> SourceAsset = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
	FMasterItemDetails ItemDetails;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
	FItemTradeConfig TradeConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
	FItemDurabilityConfig DurabilityConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
	FItemChargeConfig ChargeConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
	FItemOutfitStatsConfig OutfitStatsConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
	FItemWeaponsStatsConfig WeaponStatsConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
	FConsumablesStats ConsumablesStats;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Runtime")
	FItemRuntimeState Runtime;

public:
	UFUNCTION(BlueprintCallable, Category="Item")
	void InitializeFromAsset(UMasterItemDataAsset* InAsset, int32 InStackCount = 1);

	UFUNCTION(BlueprintCallable, Category="Item")
	void SetStackCount(int32 NewCount);

	UFUNCTION(BlueprintPure, Category="Item")
	bool IsStackable() const { return ItemDetails.bIsStackable; }

	UFUNCTION(BlueprintPure, Category="Item")
	int32 GetMaxStack() const { return ItemDetails.bIsStackable ? ItemDetails.MaxStackCount : 1; }

	UFUNCTION(BlueprintPure, Category="Item", meta=(DisplayName="Get Sound Of Use"))
	USoundBase* GetSoundOfUse() const;

	UFUNCTION(BlueprintPure, Category="Item", meta=(DisplayName="Get Dimensions"))
	void GetDimensions(FItemSize& Dimensions) const;

	UFUNCTION(BlueprintPure, Category="Item", meta=(DisplayName="Get Equipment Sound"))
	USoundBase* GetEquipmentSound() const;

	UFUNCTION(BlueprintPure, Category="Item", meta=(DisplayName="Get Drop Sound"))
	USoundBase* GetDropSound() const;

	UFUNCTION(BlueprintPure, Category="Item", meta=(DisplayName="Get Pickup Sound"))
	USoundBase* GetPickupSound() const;

	UFUNCTION(BlueprintPure, Category="Item", meta=(DisplayName="Get Outfit Stats"))
	FItemOutfitStatsConfig GetOutfitStats() const;
};