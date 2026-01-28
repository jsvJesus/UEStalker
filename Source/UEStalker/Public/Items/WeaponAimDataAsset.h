#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeaponAimDataAsset.generated.h"

class UAnimSequence;

USTRUCT(BlueprintType)
struct FWeaponAimSettings
{
	GENERATED_BODY()

	// Можно ли вообще целиться этим оружием
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim")
	bool bEnableADS = true;

	// Имя сокета на SkeletalMesh оружия
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim")
	FName AimSocketName = TEXT("AimSocket");

	// FOV при прицеливании
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim")
	float AimFOV = 70.f;

	// Скорость интерполяции FOV
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim")
	float FOVInterpSpeed = 20.f;

	// Скорость интерполяции смещения/поворота рук (Mesh1P)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim")
	float MeshInterpSpeed = 18.f;

	// Доп. подстройка (в ПРОСТРАНСТВЕ КАМЕРЫ) после вычисления по сокету
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim")
	FVector AdditionalOffset = FVector::ZeroVector;

	// Доп. подстройка поворота (в ПРОСТРАНСТВЕ КАМЕРЫ)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim")
	FRotator AdditionalRotation = FRotator::ZeroRotator;
};

UCLASS(BlueprintType)
class UESTALKER_API UWeaponAimDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim")
	FWeaponAimSettings Settings;

	// Anim Sequence "Aim" (для Mesh1P/рук) — индивидуально для оружия
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim")
	TObjectPtr<UAnimSequence> AimAnim1P = nullptr;
};