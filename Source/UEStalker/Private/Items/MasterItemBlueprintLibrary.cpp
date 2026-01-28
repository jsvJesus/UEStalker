#include "Items/MasterItemBlueprintLibrary.h"
#include "Engine/Texture2D.h"
#include "Items/ItemObject.h"
#include "Components/InventoryComponent.h"

FText UMasterItemBlueprintLibrary::GetCategoryText(EItemCategory Value)
{
	if (const UEnum* Enum = StaticEnum<EItemCategory>())
	{
		return Enum->GetDisplayNameTextByValue((int64)Value);
	}
	return FText::GetEmpty();
}

FText UMasterItemBlueprintLibrary::GetSubCategoryText(EItemSubCategory Value)
{
	if (const UEnum* Enum = StaticEnum<EItemSubCategory>())
	{
		return Enum->GetDisplayNameTextByValue((int64)Value);
	}
	return FText::GetEmpty();
}

FText UMasterItemBlueprintLibrary::GetAmmoTypeText(EAmmoType Value)
{
	if (const UEnum* Enum = StaticEnum<EAmmoType>())
	{
		return Enum->GetDisplayNameTextByValue((int64)Value);
	}
	return FText::GetEmpty();
}

FText UMasterItemBlueprintLibrary::GetWeaponTypeText(EWeaponType Value)
{
	if (const UEnum* Enum = StaticEnum<EWeaponType>())
	{
		return Enum->GetDisplayNameTextByValue((int64)Value);
	}
	return FText::GetEmpty();
}

FText UMasterItemBlueprintLibrary::GetGrenadeTypeText(EGrenadeType Value)
{
	if (const UEnum* Enum = StaticEnum<EGrenadeType>())
	{
		return Enum->GetDisplayNameTextByValue((int64)Value);
	}
	return FText::GetEmpty();
}

FText UMasterItemBlueprintLibrary::GetMagazineTypeText(EMagazineType Value)
{
	if (const UEnum* Enum = StaticEnum<EMagazineType>())
	{
		return Enum->GetDisplayNameTextByValue((int64)Value);
	}
	return FText::GetEmpty();
}

FText UMasterItemBlueprintLibrary::GetWeaponStateText(EWeaponState Value)
{
	if (const UEnum* Enum = StaticEnum<EWeaponState>())
	{
		return Enum->GetDisplayNameTextByValue((int64)Value);
	}
	return FText::GetEmpty();
}

TArray<EAmmoType> UMasterItemBlueprintLibrary::GetAllAmmoTypes()
{
	return {
		EAmmoType::AmmoType_45ACP,
		EAmmoType::AmmoType_223,
		EAmmoType::AmmoType_45ACP_Hydroshock,
		EAmmoType::AmmoType_12x70_Drob,
		EAmmoType::AmmoType_12x70_Drotik,
		EAmmoType::AmmoType_12x76_Zhekan,
		EAmmoType::AmmoType_545x39,
		EAmmoType::AmmoType_545x39_BP,
		EAmmoType::AmmoType_556x45,
		EAmmoType::AmmoType_556x45_AP,
		EAmmoType::AmmoType_57x28,
		EAmmoType::AmmoType_57x28_AP,
		EAmmoType::AmmoType_762x54R_7H14,
		EAmmoType::AmmoType_762x54_7H1,
		EAmmoType::AmmoType_762x54_BP,
		EAmmoType::AmmoType_762x54_PP,
		EAmmoType::AmmoType_9x18_PlusPPlus,
		EAmmoType::AmmoType_9x18_PBP,
		EAmmoType::AmmoType_9x19_FMJ,
		EAmmoType::AmmoType_9x19_JHP,
		EAmmoType::AmmoType_9x39_PAB9,
		EAmmoType::AmmoType_9x39_SP5,
		EAmmoType::AmmoType_9x39_SP6
	};
}

FVector2D UMasterItemBlueprintLibrary::GetItemPixelSize(const FItemSize& Size, float TilePx)
{
	return FVector2D((float)Size.X * TilePx, (float)Size.Y * TilePx);
}

UTexture2D* UMasterItemBlueprintLibrary::GetConditionTextureByPercent(const FItemColorsCondition& Cond, float Normalized01)
{
	const float P = FMath::Clamp(Normalized01, 0.f, 1.f);

	// 0..15
	if (P < 0.15f) return Cond.BaseCondition;

	// 15..50
	if (P < 0.50f) return Cond.GoodCondition;

	// 50..80
	if (P < 0.80f) return Cond.AverageCondition;

	// 80..100
	return Cond.PoorCondition;
}
