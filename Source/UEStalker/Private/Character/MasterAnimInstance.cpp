#include "Character/MasterAnimInstance.h"
#include "Character/MasterCharacter.h"
#include "Items/MasterWeaponActor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "KismetAnimationLibrary.h"

void UMasterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	APawn* P = TryGetPawnOwner();
	CachedCharacter = Cast<AMasterCharacter>(P);
}

void UMasterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	APawn* P = TryGetPawnOwner();
	if (!IsValid(P))
	{
		Velocity = FVector::ZeroVector;
		Speed = 0.f;
		Direction = 0.f;
		bIsFalling = false;
		WeaponState = EWeaponState::Unarmed;
		CachedCharacter = nullptr;
		return;
	}

	if (!IsValid(CachedCharacter))
	{
		CachedCharacter = Cast<AMasterCharacter>(P);
	}
	
	Velocity = P->GetVelocity();
	Speed = FVector(Velocity.X, Velocity.Y, 0.f).Size(); // VectorLengthXY

	// --- LocomotionState (Idle/Walk/Run) ---
	// Idle: Speed <= IdleSpeedThreshold
	// Walk: между Idle и RunEnter (или после выхода из Run)
	// Run : Speed >= RunEnterSpeed (и держим Run пока Speed >= RunExitSpeed)

	if (Speed <= IdleSpeedThreshold)
	{
		LocomotionState = ELocomotionState::Idle;
	}
	else
	{
		if (LocomotionState == ELocomotionState::Run)
		{
			// держим бег, пока не опустимся ниже RunExitSpeed
			if (Speed < RunExitSpeed)
			{
				LocomotionState = ELocomotionState::Walk;
			}
		}
		else
		{
			// вход в бег
			if (Speed >= RunEnterSpeed)
			{
				LocomotionState = ELocomotionState::Run;
			}
			else
			{
				LocomotionState = ELocomotionState::Walk;
			}
		}
	}

	const FRotator BaseRotation = IsValid(CachedCharacter) ? CachedCharacter->GetActorRotation() : P->GetActorRotation();
	Direction = UKismetAnimationLibrary::CalculateDirection(Velocity, BaseRotation);

	if (IsValid(CachedCharacter) && IsValid(CachedCharacter->GetCharacterMovement()))
	{
		bIsFalling = CachedCharacter->GetCharacterMovement()->IsFalling();
	}
	else
	{
		bIsFalling = false;
	}

	// WeaponState берём из персонажа (меняется при нажатии 1/2/3/4/5/X)
	WeaponState = IsValid(CachedCharacter) ? CachedCharacter->GetWeaponState() : EWeaponState::Unarmed;

	// WeaponState берём из персонажа (меняется при нажатии RMB)
	bIsAiming  = IsValid(CachedCharacter) ? CachedCharacter->IsAiming() : false;
	AimAlpha   = IsValid(CachedCharacter) ? CachedCharacter->GetAimAlpha() : 0.f;
	AimSequence = IsValid(CachedCharacter) ? CachedCharacter->GetAimSequence() : nullptr;

	// Left-hand IK transform (relative to right hand bone)
	bUseLeftHandIK = false;
	LeftHandIKTransform = FTransform::Identity;
	LeftHandIKTargetName = NAME_None;

	if (IsValid(CachedCharacter))
	{
		USkeletalMeshComponent* Mesh1P = CachedCharacter->GetMesh1P();
		if (IsValid(Mesh1P))
		{
			const FName RightHandBone = CachedCharacter->GetRightHandBoneName();
			if (Mesh1P->GetBoneIndex(RightHandBone) != INDEX_NONE)
			{
				AMasterWeaponActor* WeaponActor = CachedCharacter->GetActiveWeaponActor();
				if (IsValid(WeaponActor) && IsValid(WeaponActor->WeaponMesh))
				{
					const bool bReloadMag = (CachedCharacter->GetLeftHandIKMode() == ELeftHandIKMode::ReloadMag);
					const FName TargetName = WeaponActor->GetLeftHandTargetName(bReloadMag);
					LeftHandIKTargetName = TargetName;

					const bool bHasSocket = WeaponActor->WeaponMesh->DoesSocketExist(TargetName);
					const int32 BoneIndex = WeaponActor->WeaponMesh->GetBoneIndex(TargetName);
					if (!TargetName.IsNone() && (bHasSocket || BoneIndex != INDEX_NONE))
					{
						FTransform TargetWorld;
						if (bHasSocket)
						{
							TargetWorld = WeaponActor->WeaponMesh->GetSocketTransform(TargetName, RTS_World);
						}
						else
						{
							const FTransform BoneCS = WeaponActor->WeaponMesh->GetBoneTransform(BoneIndex);
							TargetWorld = BoneCS * WeaponActor->WeaponMesh->GetComponentTransform();
						}

						FVector OutLoc;
						FRotator OutRot;
						Mesh1P->TransformToBoneSpace(RightHandBone, TargetWorld.GetLocation(), TargetWorld.Rotator(), OutLoc, OutRot);

						LeftHandIKTransform = FTransform(OutRot, OutLoc, FVector::OneVector);
						bUseLeftHandIK = true;
					}
				}
			}
		}
	}
}
