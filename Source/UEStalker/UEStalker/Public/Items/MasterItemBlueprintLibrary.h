#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Items/MasterItemEnums.h"
#include "Items/MasterItemStructs.h"
#include "MasterItemBlueprintLibrary.generated.h"

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

	UFUNCTION(BlueprintPure, Category="MasterItem|Ammo")
	static TArray<EAmmoType> GetAllAmmoTypes();

	/** Размер иконки в пикселях для UMG (по умолчанию 64px клетка) */
	UFUNCTION(BlueprintPure, Category="MasterItem|UI")
	static FVector2D GetItemPixelSize(const FItemSize& Size, float TilePx = 64.f);

	/** Подбор текстуры состояния по проценту (0..1) */
	UFUNCTION(BlueprintPure, Category="MasterItem|Condition")
	static UTexture2D* GetConditionTextureByPercent(const FItemColorsCondition& Cond, float Normalized01);
};