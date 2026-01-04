#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MasterItemActor.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class UMasterItemDataAsset;

UCLASS()
class AMasterItemActor : public AActor
{
	GENERATED_BODY()

public:
	AMasterItemActor();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> DefaultSceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> StaticMeshComp = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> SphereCollision = nullptr;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	TObjectPtr<UMasterItemDataAsset> ItemData = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item", meta=(ClampMin="1", UIMin="1"))
	int32 WorldStackCount = 1;
};