// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/EquipmentComponent.h"
#include "InventorySlotWidget.generated.h"

class USizeBox;
class UBorder;
class UImage;
class UProgressBar;
class UItemObject;
class UMaterialInterface;
class UDragDropOperation;
class UEquipmentComponent;
class UInventoryComponent;
class UDragItemVisualWidget;

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

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UProgressBar> DurabilityBar = nullptr;

	// ===== Runtime refs =====
	UPROPERTY(BlueprintReadOnly, Category="Slot")
	TObjectPtr<UEquipmentComponent> EquipmentRef = nullptr;

	UPROPERTY()
	TObjectPtr<UInventoryComponent> InventoryRefCached = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Slot", meta=(ExposeOnSpawn="true"))
	EEquipmentSlotId SlotId = EEquipmentSlotId::None;

	// 1x1 = 64x64
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Slot")
	float TileSize = 64.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Slot|Design", meta=(ExposeOnSpawn="true"))
	float Width = 64.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Slot|Design", meta=(ExposeOnSpawn="true"))
	float Height = 64.f;

	UPROPERTY(BlueprintAssignable, Category="Slot|Events")
	FOnEquipmentRemoved OnEquipmentRemoved;

	// ===== API =====
	UFUNCTION(BlueprintPure, Category="Slot")
	bool ItemNone() const;

	UFUNCTION(BlueprintPure, Category="Slot")
	UItemObject* GetItem() const;

	UFUNCTION(BlueprintCallable, Category="Slot")
	void UnequipItem(bool bReturnToInventory = true);

	UFUNCTION(BlueprintCallable, Category="Slot")
	bool EquipItem(UItemObject* InItem);

	UFUNCTION(BlueprintCallable, Category="Slot")
	UItemObject* GetPayload(UDragDropOperation* Operation) const;

	UFUNCTION(BlueprintCallable, Category="Slot")
	void RefreshSlot(UMaterialInterface* Material, FLinearColor InColorAndOpacity);

	UFUNCTION(BlueprintPure, Category="Slot")
	bool CanAcceptItem(UItemObject* InItem) const;

	UFUNCTION(BlueprintCallable, Category="Slot")
	void SetEquipmentRef(UEquipmentComponent* InEquipment);

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	virtual void NativeOnDragDetected(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent,
		UDragDropOperation*& OutOperation
	) override;

	virtual bool NativeOnDrop(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation
	) override;

	virtual void NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	virtual void NativeOnDragCancelled(
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation
	) override;

private:
	UFUNCTION()
	void HandleEquipmentChanged(EEquipmentSlotId ChangedSlot, UItemObject* NewItem);

	UFUNCTION()
	void HandleActiveSlotChanged();

	void ApplySizeFromItem(UItemObject* Item);
	void SetIconFromItem(UItemObject* Item, FLinearColor InColorAndOpacity);
	void ClearVisual();
	void UpdateHighlightFromEquipment();
	void ApplyDesignSize();

	void RefreshDurabilityVisual(UItemObject* Item);
	static float CalcDurability(const UItemObject* Item);

	UFUNCTION()
	void HandleInventoryChanged();

	// чтобы слот не перекрывал другие drop-target'ы во время перетаскивания
	ESlateVisibility CachedVisibility = ESlateVisibility::Visible;
	bool bDragInProgress = false;
	void RestoreAfterDrag();

	UFUNCTION()
	void HandleSelectedSlotChanged(EEquipmentSlotId NewSelectedSlot);
};
