#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Input/Reply.h"
#include "InventoryWidget.generated.h"

class UInventorySlotWidget;
class UBorder;
class UTextBlock;
class UInventoryComponent;
class UEquipmentComponent;
class UInventoryGridWidget;
class UDropAreaWidget;
class UItemObject;

UCLASS()
class UESTALKER_API UInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ===== BindWidget =====
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly)
	TObjectPtr<UBorder> Border = nullptr;

	// ===== Runtime =====
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Inventory", meta=(ExposeOnSpawn="true"))
	TObjectPtr<UInventoryComponent> InventoryComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UEquipmentComponent> EquipmentComponent = nullptr;
	
	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UInventoryGridWidget> WBGridInventory = nullptr;
	
	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UDropAreaWidget> WBDropArea = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UTextBlock> TextWeight = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UTextBlock> TextMaxWeight = nullptr;

	// Equipment Slots
	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UInventorySlotWidget> WBS_Armor = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UInventorySlotWidget> WBS_Helmet = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UInventorySlotWidget> WBS_Backpack = nullptr;

	// Weapons Slots
	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UInventorySlotWidget> WBS_PrimaryWeaponSlot = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UInventorySlotWidget> WBS_SecondaryWeaponSlot = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UInventorySlotWidget> WBS_PistolSlot = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UInventorySlotWidget> WBS_KnifeSlot = nullptr;

	// Devices Slots
	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UInventorySlotWidget> WBS_DeviceSlot1 = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UInventorySlotWidget> WBS_DeviceSlot2 = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UInventorySlotWidget> WBS_DeviceSlot3 = nullptr;

	// Grenades Slots
	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UInventorySlotWidget> WBS_GrenadePrimarySlot = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UInventorySlotWidget> WBS_GrenadeSecondarySlot = nullptr;

	// Modules / Artefacts Slots
	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UInventorySlotWidget> WBS_ItemSlot1 = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UInventorySlotWidget> WBS_ItemSlot2 = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UInventorySlotWidget> WBS_ItemSlot3 = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UInventorySlotWidget> WBS_ItemSlot4 = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UInventorySlotWidget> WBS_ItemSlot5 = nullptr;

	// Quick Slots
	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UInventorySlotWidget> WBS_QuickSlot1 = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UInventorySlotWidget> WBS_QuickSlot2 = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UInventorySlotWidget> WBS_QuickSlot3 = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UInventorySlotWidget> WBS_QuickSlot4 = nullptr;

	////////////////////////////////////////////////////////////////////

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	float TileSize = 64.f;

	UFUNCTION(BlueprintCallable, Category="Inventory")
	void InitializeWidget(UInventoryComponent* InInventoryComponent, float InTileSize = 64.f);

	UFUNCTION(BlueprintCallable, Category="Inventory|Equipment")
	bool TryAutoEquipItem(UItemObject* ItemObject);

protected:
	virtual void NativeOnInitialized() override;
	
	// OnDrop
	virtual bool NativeOnDrop(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation
	) override;

private:
	bool bRuntimeInitialized = false;
	void SetupWithInventory();

	UFUNCTION()
	void OnEquipmentSlotChanged(EEquipmentSlotId SlotId, UItemObject* Item);

public:
	// HandleBorderMouseDown
	UFUNCTION(BlueprintCallable, Category="Inventory|Input")
	FEventReply HandleBorderMouseDown(FGeometry MyGeometry, const FPointerEvent& MouseEvent);

	UFUNCTION()
	void OnInventoryChangedEvent();
};