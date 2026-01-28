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
	FItemMagazineConfig MagazineConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
	FConsumablesStats ConsumablesStats;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Runtime")
	FItemRuntimeState Runtime;

	// ==========================
	// Runtime: Weapon/Magazine
	// ==========================

	/** For weapons: inserted magazine object (removed from inventory). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Runtime|Weapon")
	TObjectPtr<UItemObject> InsertedMagazine = nullptr;

	/** For magazines: current bullets loaded */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Runtime|Magazine")
	int32 MagazineCurrentAmmo = 0;

	/** For magazines: ammo type currently loaded */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Runtime|Magazine")
	EAmmoType MagazineLoadedAmmoType = EAmmoType::AmmoType_None;

	/** For magazines: unit weight of loaded ammo (so weight is preserved when ammo item is consumed). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Runtime|Magazine")
	float MagazineAmmoUnitWeight = 0.f;

public:
	UFUNCTION(BlueprintCallable, Category="Item")
	void InitializeFromAsset(UMasterItemDataAsset* InAsset, int32 InStackCount = 1);

	UFUNCTION(BlueprintCallable, Category="Item")
	void SetStackCount(int32 NewCount);

	UFUNCTION(BlueprintPure, Category="Item")
	bool IsStackable() const { return ItemDetails.bIsStackable; }

	UFUNCTION(BlueprintPure, Category="Item")
	bool IsWeapon() const { return ItemDetails.ItemCategory == EItemCategory::ItemCat_Weapons; }

	UFUNCTION(BlueprintPure, Category="Item")
	bool IsAmmo() const { return ItemDetails.ItemCategory == EItemCategory::ItemCat_Ammo; }

	UFUNCTION(BlueprintPure, Category="Item")
	bool IsMagazine() const { return ItemDetails.ItemSubCategory == EItemSubCategory::ItemSubCat_Attachments_Magazine || ItemDetails.MagazineType != EMagazineType::Mag_None; }

	// ===== Weapon runtime =====
	UFUNCTION(BlueprintPure, Category="Item|Weapon")
	UItemObject* GetInsertedMagazine() const { return InsertedMagazine; }

	UFUNCTION(BlueprintCallable, Category="Item|Weapon")
	void SetInsertedMagazine(UItemObject* NewMag) { InsertedMagazine = NewMag; }

	UFUNCTION(BlueprintPure, Category="Item|Weapon")
	bool IsMagazineCompatibleForWeapon(const UItemObject* MagItem) const;

	// ===== Magazine runtime =====
	UFUNCTION(BlueprintPure, Category="Item|Magazine")
	int32 GetMagazineCapacity() const { return FMath::Max(0, MagazineConfig.Capacity); }

	UFUNCTION(BlueprintPure, Category="Item|Magazine")
	int32 GetMagazineCurrentAmmo() const { return FMath::Max(0, MagazineCurrentAmmo); }

	UFUNCTION(BlueprintCallable, Category="Item|Magazine")
	void SetMagazineCurrentAmmo(int32 NewValue) { MagazineCurrentAmmo = FMath::Max(0, NewValue); }

	UFUNCTION(BlueprintPure, Category="Item|Magazine")
	EAmmoType GetMagazineLoadedAmmoType() const { return MagazineLoadedAmmoType; }

	UFUNCTION(BlueprintCallable, Category="Item|Magazine")
	void SetMagazineLoadedAmmoType(EAmmoType NewType);

	UFUNCTION(BlueprintPure, Category="Item|Magazine")
	float GetMagazineAmmoUnitWeight() const { return MagazineAmmoUnitWeight; }

	UFUNCTION(BlueprintCallable, Category="Item|Magazine")
	void SetMagazineAmmoUnitWeight(float NewValue) { MagazineAmmoUnitWeight = FMath::Max(0.f, NewValue); }

	UFUNCTION(BlueprintPure, Category="Item|Magazine")
	bool IsAmmoCompatibleForMagazine(EAmmoType AmmoType) const;

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