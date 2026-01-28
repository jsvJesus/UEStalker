#include "UI/Inventory/Context/DropAreaWidget.h"
#include "UI/Inventory/Context/EquipmentDragDropOperation.h"
#include "Blueprint/DragDropOperation.h"
#include "Components/SizeBox.h"
#include "Components/InventoryComponent.h"
#include "Components/EquipmentComponent.h"
#include "Items/ItemObject.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

void UDropAreaWidget::SetInventoryRef(UInventoryComponent* InInventoryComponent)
{
	InventoryComponent = InInventoryComponent;
}

void UDropAreaWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	// SetWidthOverride / SetHeightOverride
	if (IsValid(DropAreaSizeBox))
	{
		DropAreaSizeBox->SetWidthOverride(Width);
		DropAreaSizeBox->SetHeightOverride(Height);
	}
}

bool UDropAreaWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
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

	// Play drop sound
	if (USoundBase* DropSound = ItemObject->ItemDetails.ItemDropSound)
	{
		AActor* OwnerActor = InventoryComponent->GetOwner();
		const FVector Loc = IsValid(OwnerActor) ? OwnerActor->GetActorLocation() : FVector::ZeroVector;
		UGameplayStatics::PlaySoundAtLocation(this, DropSound, Loc);
	}

	// InventoryComponent -> DropItem(Target=InventoryComponent, Actor=Owner, ItemObject, GroundClamp=true)
	AActor* OwnerActor = InventoryComponent->GetOwner();
	InventoryComponent->DropItem(OwnerActor, ItemObject, true);

	if (UEquipmentDragDropOperation* EquipOp = Cast<UEquipmentDragDropOperation>(InOperation))
	{
		if (IsValid(EquipOp->SourceEquipment))
		{
			EquipOp->SourceEquipment->UnequipSlot(EquipOp->SourceSlotId, false);
		}
	}

	return true;
}
