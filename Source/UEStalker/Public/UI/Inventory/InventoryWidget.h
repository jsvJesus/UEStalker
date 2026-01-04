#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Input/Reply.h"
#include "InventoryWidget.generated.h"

class UBorder;
class UTextBlock;
class UInventoryComponent;
class UInventoryGridWidget;
class UDropAreaWidget;

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
	
	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UInventoryGridWidget> WBGridInventory = nullptr;
	
	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UDropAreaWidget> WBDropArea = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UTextBlock> TextWeight = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UTextBlock> TextMaxWeight = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	float TileSize = 64.f;

	UFUNCTION(BlueprintCallable, Category="Inventory")
	void InitializeWidget(UInventoryComponent* InInventoryComponent, float InTileSize = 64.f);

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

public:
	// HandleBorderMouseDown
	UFUNCTION(BlueprintCallable, Category="Inventory|Input")
	FEventReply HandleBorderMouseDown(FGeometry MyGeometry, const FPointerEvent& MouseEvent);

	UFUNCTION()
	void OnInventoryChangedEvent();
};