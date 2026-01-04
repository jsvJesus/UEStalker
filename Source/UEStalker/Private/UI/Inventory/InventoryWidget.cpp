#include "UI/Inventory/InventoryWidget.h"
#include "UI/Inventory/InventoryGridWidget.h"
#include "UI/Inventory/Context/DropAreaWidget.h"
#include "Blueprint/DragDropOperation.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/InventoryComponent.h"
#include "Items/ItemObject.h"

void UInventoryWidget::InitializeWidget(UInventoryComponent* InInventoryComponent, float InTileSize)
{
	InventoryComponent = InInventoryComponent;
	TileSize = InTileSize;

	SetupWithInventory();
}

void UInventoryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetupWithInventory();
}

bool UInventoryWidget::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation
)
{
	if (!IsValid(InOperation) || !IsValid(InventoryComponent))
	{
		return false;
	}

	// Payload -> ItemObject
	UItemObject* ItemObject = Cast<UItemObject>(InOperation->Payload);
	if (!IsValid(ItemObject))
	{
		return false;
	}

	// TryAddItem
	const bool bAdded = InventoryComponent->TryAddItem(ItemObject);
	return bAdded;
}

void UInventoryWidget::SetupWithInventory()
{
	if (bRuntimeInitialized)
	{
		return;
	}

	if (!IsValid(InventoryComponent))
	{
		return;
	}

	bRuntimeInitialized = true;

	// Initialize Grid
	if (IsValid(WBGridInventory))
	{
		WBGridInventory->Initialize(InventoryComponent, TileSize, this);
	}

	// SetInventoryRef (DropArea)
	if (IsValid(WBDropArea))
	{
		WBDropArea->SetInventoryRef(InventoryComponent);
	}

	// Bind OnInventoryChanged -> OnInventoryChangedEvent
	InventoryComponent->OnInventoryChanged.AddDynamic(this, &UInventoryWidget::OnInventoryChangedEvent);

	// первичное обновление
	OnInventoryChangedEvent();
}

FEventReply UInventoryWidget::HandleBorderMouseDown(FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{
	return UWidgetBlueprintLibrary::Handled();
}

void UInventoryWidget::OnInventoryChangedEvent()
{
	if (IsValid(WBGridInventory))
	{
		WBGridInventory->Refresh();
	}

	// GetWeight/GetMaxWeight в InventoryComponent — TextWeight/TextMaxWeight
}
