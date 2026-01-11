#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Items/MasterItemEnums.h"
#include "Items/MasterItemStructs.h"
#include "MasterItemBlueprintLibrary.generated.h"

class UItemObject;
class UInventoryComponent;
class UTexture2D;

UCLASS()
class UMasterItemBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="MasterItem|Enums")
	static FText GetCategoryText(EItemCategory Value);

	UFUNCTION(BlueprintPure, Category="MasterItem|Enums")
	static FText GetSubCategoryText(EItemSubCategory Value);

	UFUNCTION(BlueprintPure, Category="MasterItem|Enums")
	static FText GetAmmoTypeText(EAmmoType Value);

	UFUNCTION(BlueprintPure, Category="MasterItem|Enums")
	static FText GetWeaponTypeText(EWeaponType Value);

	UFUNCTION(BlueprintPure, Category="MasterItem|Enums")
	static FText GetGrenadeTypeText(EGrenadeType Value);

	UFUNCTION(BlueprintPure, Category="MasterItem|Enums")
	static FText GetMagazineTypeText(EMagazineType Value);

	UFUNCTION(BlueprintPure, Category="MasterItem|Enums")
	static FText GetWeaponStateText(EWeaponState Value);

	UFUNCTION(BlueprintPure, Category="MasterItem|Ammo")
	static TArray<EAmmoType> GetAllAmmoTypes();

	/** Размер иконки в пикселях для UMG (по умолчанию 64px клетка) */
	UFUNCTION(BlueprintPure, Category="MasterItem|UI")
	static FVector2D GetItemPixelSize(const FItemSize& Size, float TilePx = 64.f);

	/** Подбор текстуры состояния по проценту (0..1) */
	UFUNCTION(BlueprintPure, Category="MasterItem|Condition")
	static UTexture2D* GetConditionTextureByPercent(const FItemColorsCondition& Cond, float Normalized01);

	// ============================
	// Drag&Drop: Weapon/Mag/Ammo
	// ============================

	/** Можно ли применить Payload к Target (Mag->Weapon, Ammo->Mag, Ammo->Weapon(через вставленный магазин)) */
	UFUNCTION(BlueprintPure, Category="MasterItem|DragDrop")
	static bool CanApplyPayloadToTarget(const UItemObject* Payload, const UItemObject* Target);

	/** Применить Payload к Target через InventoryComponent (изменит стак/удалит предмет при необходимости) */
	UFUNCTION(BlueprintCallable, Category="MasterItem|DragDrop")
	static bool ApplyPayloadToTarget(UInventoryComponent* Inventory, UItemObject* Payload, UItemObject* Target, int32 RequestedCount, int32& OutAppliedCount);
};