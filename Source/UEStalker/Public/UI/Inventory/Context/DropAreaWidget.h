#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DropAreaWidget.generated.h"

class USizeBox;
class UInventoryComponent;
class UDragDropOperation;
class UItemObject;

UCLASS()
class UESTALKER_API UDropAreaWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ===== BindWidget =====
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly)
	TObjectPtr<USizeBox> DropAreaSizeBox = nullptr;

	// ===== Runtime =====
	UPROPERTY(BlueprintReadOnly, Category="DropArea")
	TObjectPtr<UInventoryComponent> InventoryComponent = nullptr;

	// как в BP (Width/Height)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DropArea|Design", meta=(ExposeOnSpawn="true"))
	float Width = 64.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DropArea|Design", meta=(ExposeOnSpawn="true"))
	float Height = 64.f;

	// SetInventoryRef
	UFUNCTION(BlueprintCallable, Category="DropArea")
	void SetInventoryRef(UInventoryComponent* InInventoryComponent);

protected:
	// EventPreConstruct
	virtual void NativePreConstruct() override;

	// OnDrop
	virtual bool NativeOnDrop(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation
	) override;
};