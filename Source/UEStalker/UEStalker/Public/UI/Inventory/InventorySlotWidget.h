// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/MasterItemEnums.h"
#include "Items/MasterItemStructs.h"
#include "InventorySlotWidget.generated.h"

class USizeBox;
class UBorder;
class UImage;
class UInventoryComponent;
class UItemObject;
class UMaterialInterface;

USTRUCT(BlueprintType)
struct FInventorySlotRule
{
	GENERATED_BODY()

	// Если None -> пропускаем проверку категории
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rule")
	EItemCategory AllowedCategory = EItemCategory::ItemCat_None;

	// Если None -> пропускаем проверку субкатегории
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rule")
	EItemSubCategory AllowedSubCategory = EItemSubCategory::ItemSubCat_None;

	// Если None -> пропускаем проверку типа патронов
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rule")
	EAmmoType AllowedAmmoType = EAmmoType::AmmoType_None;

	// true = слот “универсальный” (принимает всё, игнорит правила выше)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rule")
	bool bAcceptAnyItem = true;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquipmentRemoved);

UCLASS()
class UESTALKER_API UInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ===== BindWidget =====
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly)
	TObjectPtr<USizeBox> BackGroundSizeBox = nullptr;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly)
	TObjectPtr<UBorder> BackgroundSlot = nullptr;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly)
	TObjectPtr<UImage> Icon = nullptr;

	// ===== Runtime data =====
	UPROPERTY(BlueprintReadOnly, Category="Slot")
	TObjectPtr<UItemObject> ItemObject = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Slot")
	TObjectPtr<UInventoryComponent> InventoryRef = nullptr;

	// 1x1 = 64x64
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Slot")
	float TileSize = 64.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Slot|Design", meta=(ExposeOnSpawn="true"))
	float Width = 64.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Slot|Design", meta=(ExposeOnSpawn="true"))
	float Height = 64.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Slot|Design", meta=(ExposeOnSpawn="true"))
	bool bRotated = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Slot")
	bool bLocked = false;

	// Правило слота (по категориям/субкатегориям/патронам)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Slot")
	FInventorySlotRule SlotRule;
	
	UPROPERTY(BlueprintAssignable, Category="Slot|Events")
	FOnEquipmentRemoved OnEquipmentRemoved;

	UFUNCTION(BlueprintPure, Category="Slot")
	bool ItemNone() const;

	UFUNCTION(BlueprintPure, Category="Slot")
	UItemObject* GetItem() const;

	UFUNCTION(BlueprintCallable, Category="Slot")
	void UnequipItem(int32 Operation = 0);

	UFUNCTION(BlueprintCallable, Category="Slot")
	bool EquipItem(UItemObject* InItem);

	UFUNCTION(BlueprintCallable, Category="Slot")
	UItemObject* GetPayload(UDragDropOperation* Operation) const;

	UFUNCTION(BlueprintCallable, Category="Slot")
	void RefreshSlot(UMaterialInterface* Material, FLinearColor InColorAndOpacity);

	UFUNCTION(BlueprintPure, Category="Slot")
	FItemOutfitStatsConfig GetOutfitStats() const;

	UFUNCTION(BlueprintCallable, Category="Slot")
	void SetInventoryRef(UInventoryComponent* InventoryComponent);

	// SlotType = SlotRule
	UFUNCTION(BlueprintPure, Category="Slot")
	FInventorySlotRule GetSlotType() const;

	UFUNCTION(BlueprintCallable, Category="Slot")
	void SetSlotType(const FInventorySlotRule& InSlotRule);

	// Полезно для экип/dragdrop
	UFUNCTION(BlueprintPure, Category="Slot")
	bool CanAcceptItem(UItemObject* InItem) const;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
		UDragDropOperation*& OutOperation) override;
	
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation) override;

private:
	UFUNCTION()
	void HandleOperationDrop(UDragDropOperation* Operation);
	
	void ApplySizeFromItem();
	void SetIconFromItem(FLinearColor InColorAndOpacity);
	void ClearVisual();
};
