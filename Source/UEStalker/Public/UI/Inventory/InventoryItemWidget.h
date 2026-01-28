#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/MasterItemStructs.h"
#include "InventoryItemWidget.generated.h"

class USizeBox;
class UBorder;
class UImage;
class UProgressBar;
class UTextBlock;
class UItemObject;
class UInventoryComponent;
class UDragDropOperation;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUseSelectedItem, UItemObject*, ItemObject);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeleteSelectedItem, UItemObject*, ItemObject);

UCLASS()
class UESTALKER_API UInventoryItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ===== BindWidget =====
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly)
	TObjectPtr<USizeBox> BackgroundSizeBox = nullptr;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly)
	TObjectPtr<UBorder> BackgroundBorder = nullptr;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly)
	TObjectPtr<UImage> ItemImage = nullptr;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly)
	TObjectPtr<UBorder> BorderCountText = nullptr;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly)
	TObjectPtr<UTextBlock> CountText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UProgressBar> DurabilityBar = nullptr;

	// ===== Runtime =====
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Item", meta=(ExposeOnSpawn="true"))
	TObjectPtr<UItemObject> ItemObject = nullptr;

	/** Cached inventory reference (set by InventoryGridWidget / InventorySlotWidget) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Item", meta=(ExposeOnSpawn="true"))
	TObjectPtr<UInventoryComponent> InventoryComponent = nullptr;

	// 1x1 = 64x64
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="UI")
	float TileSize = 64.f;

	// ===== Events (Call On Use Selected Item / Call On Delete Selected Item) =====
	UPROPERTY(BlueprintAssignable, Category="Item|Events")
	FOnUseSelectedItem OnUseSelectedItem;

	UPROPERTY(BlueprintAssignable, Category="Item|Events")
	FOnDeleteSelectedItem OnDeleteSelectedItem;

	// ===== Functions =====
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Item")
	FSlateBrush GetIconImage() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Item")
	FText GetCountItems() const;

	UFUNCTION(BlueprintCallable, Category="Item")
	bool RemoveItem();

	UFUNCTION(BlueprintCallable, Category="Item")
	void Refresh();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnInitialized() override;

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

	virtual bool NativeOnDragOver(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation
	) override;

	virtual void NativeOnDragCancelled(
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation
	) override;

	virtual FReply NativeOnMouseButtonDown(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent
	) override;

	virtual FReply NativeOnMouseButtonDoubleClick(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent
	) override;

	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

private:
	void ApplySizeFromItem();
	void RefreshCountVisual();

	void RefreshDurabilityVisual();
	static float CalcDurability(const UItemObject* Item);
};