#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Components/EquipmentComponent.h"
#include "EquipmentDragDropOperation.generated.h"

UCLASS()
class UESTALKER_API UEquipmentDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category="Drag")
	TObjectPtr<UEquipmentComponent> SourceEquipment = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Drag")
	EEquipmentSlotId SourceSlotId = EEquipmentSlotId::None;
};