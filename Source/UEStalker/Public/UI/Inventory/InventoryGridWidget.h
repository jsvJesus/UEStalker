#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/MasterItemStructs.h"
#include "Blueprint/DragDropOperation.h"
#include "Input/Reply.h"
#include "InventoryGridWidget.generated.h"

class UInventoryItemWidget;
class UInventoryComponent;
class UItemObject;
class USizeBox;
class UBorder;
class UCanvasPanel;
class UInventoryWidget;

/** Payload для Drag&Drop */
USTRUCT(BlueprintType)
struct FInventoryItemPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Payload")
	TObjectPtr<UItemObject> ItemObject = nullptr;
};

UCLASS()
class UESTALKER_API UInventoryGridWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ===== BindWidget =====
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly)
	TObjectPtr<USizeBox> GridSizeBox = nullptr;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly)
	TObjectPtr<UBorder> GridBorder = nullptr;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly)
	TObjectPtr<UCanvasPanel> GridCanvasPanel = nullptr;

	// InventoryComponent
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Inventory", meta=(ExposeOnSpawn="true"))
	TObjectPtr<UInventoryComponent> InventoryComponent = nullptr;

	// Родительский виджет инвентаря (нужен для ItemWidget)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Inventory", meta=(ExposeOnSpawn="true"))
	TObjectPtr<UInventoryWidget> WBInventory = nullptr;

	// Класс виджета предмета, который кладём в CanvasPanel
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Grid")
	TSubclassOf<UInventoryItemWidget> ItemWidgetClass;

	// 1x1 = 64x64
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Grid")
	float TileSize = 64.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Grid|Design")
	int32 PreviewColumns = 8;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Grid|Design")
	int32 PreviewRows = 11;

	// Рисовать подсветку места дропа (включается на DragEnter, выключается на DragLeave)
	UPROPERTY(BlueprintReadOnly, Category="Drag")
	bool bDrawDropLocation = false;

	// Последняя локальная позиция мыши (для отладки/визуала)
	UPROPERTY(BlueprintReadOnly, Category="Drag")
	FVector2D MousePosition = FVector2D::ZeroVector;

	// Текущий TopLeft (Dragged Item Top Left Tile X/Y)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Drag")
	int32 DraggedItemTopLeftTileX = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Drag")
	int32 DraggedItemTopLeftTileY = 0;

	// Линии сетки (для отрисовки)
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Grid")
	TArray<FLine2D> Lines;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Grid")
	float LineThickness = 1.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Grid|Design")
	FLinearColor GridLineColor = FLinearColor(1.f, 1.f, 1.f, 0.12f);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Grid|Design")
	bool bGridLinesAntialias = true;

	/** Достать ItemObject из DragDropOperation->Payload */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Grid|DragDrop")
	UItemObject* GetPayload(UDragDropOperation* DragDropOperation) const;

	/** Просто вернуть Handled (вешается на GridBorder->OnMouseButtonDownEvent) */
	UFUNCTION(BlueprintCallable, Category="Grid|Input")
	FEventReply HandleGridBorderMouseDown(FGeometry MyGeometry, const FPointerEvent& MouseEvent);

	/** Проверка места под Payload */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Grid")
	bool IsRoomAvailableForPayload(const FInventoryItemPayload& Payload, const UDragDropOperation* Operation) const;

	/**
	 * Позиция мыши в текущем тайле + флаги Right/Down
	 * LocalPos — позиция мыши в локальных координатах GridCanvasPanel
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Grid")
	void GetMousePositionInTile(FVector2D& LocalPos, bool& bRight, bool& bDown) const;

	/** Использовать предмет (просто прокидываем в InventoryComponent->UseItem) */
	UFUNCTION(BlueprintCallable, Category="Grid")
	void OnItemUsed(UItemObject* ItemObject);

	/** Удалить предмет (просто прокидываем в InventoryComponent->RemoveItem) */
	UFUNCTION(BlueprintCallable, Category="Grid")
	void OnItemRemoved(UItemObject* ItemObject);

	/** Создать линии сетки по Rows/Columns */
	UFUNCTION(BlueprintCallable, Category="Grid")
	void CreateLineSegments();

	/** Initialize: сохранить ссылки, выставить TileSize, линии, Refresh, подписка на OnInventoryChanged */
	UFUNCTION(BlueprintCallable, Category="Grid")
	void InitializeGrid(UInventoryComponent* InInventoryComponent, float InTileSize = 64.f, UInventoryWidget* InWBInventory = nullptr);

	/** Refresh: пересоздать ItemWidgets на CanvasPanel */
	UFUNCTION(BlueprintCallable, Category="Grid")
	void Refresh();

	/** Утилита для макроса ForEachItem: найти TopLeftTile предмета (по первой ячейке в массиве Items) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Grid")
	bool GetTopLeftTileForItem(UItemObject* ItemObject, FTile& OutTile) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Grid")
	void GetItemTileMap(TMap<UItemObject*, FTile>& OutMap) const;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	// Drop
	virtual bool NativeOnDrop(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation
	) override;

	// Drag Enter/Leave/Over
	virtual void NativeOnDragEnter(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation
	) override;

	virtual void NativeOnDragLeave(
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation
	) override;

	virtual bool NativeOnDragOver(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation
	) override;

	// Paint
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled
	) const override;
};