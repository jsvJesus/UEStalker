#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Items/MasterItemEnums.h"
#include "Animation/AnimSequence.h"
#include "MasterAnimInstance.generated.h"

class AMasterCharacter;

UENUM(BlueprintType)
enum class ELocomotionState : uint8
{
	Idle UMETA(DisplayName="Idle"),
	Walk UMETA(DisplayName="Walk"),
	Run  UMETA(DisplayName="Run"),
};

UCLASS()
class UESTALKER_API UMasterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	// Locomotion
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Anim|Locomotion")
	float Speed = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Anim|Locomotion")
	float Direction = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Anim|Locomotion")
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Anim|Locomotion")
	bool bIsFalling = false;

	// Готовое состояние для StateMachine
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Anim|Locomotion")
	ELocomotionState LocomotionState = ELocomotionState::Idle;
    
	// Пороги (чтобы не фликало на границе)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Anim|Locomotion")
	float IdleSpeedThreshold = 5.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Anim|Locomotion")
	float RunEnterSpeed = 450.f;
    
	// выход из бега чуть ниже входа (гистерезис)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Anim|Locomotion")
	float RunExitSpeed = 420;

	// Weapon stance
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Anim|Weapon")
	EWeaponState WeaponState = EWeaponState::Unarmed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Anim|Weapon")
	bool bIsAiming = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Anim|Weapon")
	float AimAlpha = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Anim|Weapon")
	TObjectPtr<UAnimSequence> AimSequence = nullptr;

	// Left-hand IK (1P)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Anim|Weapon|IK")
	bool bUseLeftHandIK = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Anim|Weapon|IK")
	float LeftHandIKAlpha = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Anim|Weapon|IK")
	FTransform LeftHandIKTransform = FTransform::Identity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Anim|Weapon|IK")
	FName LeftHandIKTargetName = NAME_None;

protected:
	UPROPERTY(Transient)
	TObjectPtr<AMasterCharacter> CachedCharacter = nullptr;

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
};