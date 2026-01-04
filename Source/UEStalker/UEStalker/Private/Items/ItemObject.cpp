#include "Items/ItemObject.h"
#include "Items/MasterItemDataAsset.h"
#include "Sound/SoundBase.h"

void UItemObject::InitializeFromAsset(UMasterItemDataAsset* InAsset, int32 InStackCount)
{
	SourceAsset = InAsset;

	if (!SourceAsset)
	{
		ItemDetails = FMasterItemDetails{};
		TradeConfig = FItemTradeConfig{};
		DurabilityConfig = FItemDurabilityConfig{};
		ChargeConfig = FItemChargeConfig{};
		OutfitStatsConfig = FItemOutfitStatsConfig{};
		WeaponStatsConfig = FItemWeaponsStatsConfig{};
		ConsumablesStats = FConsumablesStats{};
		Runtime = FItemRuntimeState{};
		return;
	}

	ItemDetails       = SourceAsset->ItemDetails;
	TradeConfig       = SourceAsset->TradeConfig;
	DurabilityConfig  = SourceAsset->DurabilityConfig;
	ChargeConfig      = SourceAsset->ChargeConfig;
	OutfitStatsConfig = SourceAsset->OutfitStatsConfig;
	WeaponStatsConfig = SourceAsset->WeaponStatsConfig;
	ConsumablesStats  = SourceAsset->ConsumablesStats;

	// Runtime init
	Runtime.StackCount = 1;
	SetStackCount(InStackCount);

	Runtime.CurrDurability = DurabilityConfig.bHasDurability ? DurabilityConfig.MaxDurability : 0.f;
	Runtime.CurrCharge     = ChargeConfig.bHasCharge ? ChargeConfig.MaxCharge : 0.f;
	Runtime.bIsRotated     = false;
}

void UItemObject::SetStackCount(int32 NewCount)
{
	NewCount = FMath::Max(1, NewCount);

	if (!ItemDetails.bIsStackable)
	{
		Runtime.StackCount = 1;
		return;
	}

	const int32 MaxStack = FMath::Max(1, ItemDetails.MaxStackCount);
	Runtime.StackCount = FMath::Clamp(NewCount, 1, MaxStack);
}

USoundBase* UItemObject::GetSoundOfUse() const
{
	return ItemDetails.ItemUseSound;
}

void UItemObject::GetDimensions(FItemSize& Dimensions) const
{
	Dimensions = ItemDetails.Size;

	// Учитываем поворот (как инвентарь раскладывает предмет)
	if (Runtime.bIsRotated)
	{
		Swap(Dimensions.X, Dimensions.Y);
	}

	// Safety
	Dimensions.X = FMath::Max(1, Dimensions.X);
	Dimensions.Y = FMath::Max(1, Dimensions.Y);
}

USoundBase* UItemObject::GetEquipmentSound() const
{
	return ItemDetails.ItemEquipSound;
}

USoundBase* UItemObject::GetDropSound() const
{
	return ItemDetails.ItemDropSound;
}

USoundBase* UItemObject::GetPickupSound() const
{
	return ItemDetails.ItemPickupSound;
}

FItemOutfitStatsConfig UItemObject::GetOutfitStats() const
{
	return OutfitStatsConfig;
}
