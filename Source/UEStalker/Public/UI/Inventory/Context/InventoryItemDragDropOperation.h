#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "InventoryItemDragDropOperation.generated.h"

class UInventoryComponent;

/**
 * Drag&Drop operation for items dragged from InventoryGrid.
 * Stores the source inventory + top-left index to allow safe move/rollback.
 */
UCLASS()
class UESTALKER_API UInventoryItemDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	/** Inventory the item was dragged from. */
	UPROPERTY(BlueprintReadOnly, Category="Drag")
	TObjectPtr<UInventoryComponent> SourceInventory = nullptr;

	/** Top-left index of the item in the source inventory grid (INDEX_NONE if unknown). */
	UPROPERTY(BlueprintReadOnly, Category="Drag")
	int32 SourceTopLeftIndex = INDEX_NONE;
};