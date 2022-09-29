#include "ShooterGame.h"
#include "Player/ShooterCharacterMovement.h"
#include "GameFramework/Character.h"



//----------------------------------------------------------------------//
// UPawnMovementComponent
//----------------------------------------------------------------------//
UShooterCharacterMovement::UShooterCharacterMovement(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	//set variable based on capsule component's radius
	//ClipSafetyDistance = GetOwner()->FindComponentByClass<UCapsuleComponent>()->GetUnscaledCapsuleRadius(); //CRASH
}


float UShooterCharacterMovement::GetMaxSpeed() const
{
	float MaxSpeed = Super::GetMaxSpeed();

	const AShooterCharacter* ShooterCharacterOwner = Cast<AShooterCharacter>(PawnOwner);
	if (ShooterCharacterOwner)
	{
		if (ShooterCharacterOwner->IsTargeting())
		{
			MaxSpeed *= ShooterCharacterOwner->GetTargetingSpeedModifier();
		}
		if (ShooterCharacterOwner->IsRunning())
		{
			MaxSpeed *= ShooterCharacterOwner->GetRunningSpeedModifier();
		}
	}

	return MaxSpeed;
}



FNetworkPredictionData_Client* UShooterCharacterMovement::GetPredictionData_Client() const
{
	check(PawnOwner != nullptr)

	if (ClientPredictionData == nullptr) //ClientPredictionData is a cached value
	{
		// set client prediction data type inside charactermovementcomponent

		UShooterCharacterMovement* MutableThis = const_cast<UShooterCharacterMovement*>(this);

		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Custom(*this); //set networkpredictiondata type to our custom type
		MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = 92.f;
		MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = 140.f;
	}

	return ClientPredictionData;
}



void UShooterCharacterMovement::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	//set variable on server based on custom flag value
	Safe_bWantsToTeleport = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
}



// custom movement abilities implementation
void UShooterCharacterMovement::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity)
{
	// TELEPORT
	if (Safe_bWantsToTeleport)
	{
		Teleport();
	}
}



//toggle flag on client in response to player input
void UShooterCharacterMovement::TeleportPressed()
{
	Safe_bWantsToTeleport = true;
}



//Set new location to teleport character to
void UShooterCharacterMovement::Teleport()
{
	FVector CharacterLocation = CharacterOwner->GetActorLocation();
	FVector NewLocation = CharacterLocation + CharacterOwner->GetActorForwardVector() * TeleportDistance;
	FVector LinetraceStop = NewLocation + CharacterOwner->GetActorForwardVector() * ClipSafetyDistance; //trace line a bit further than teleport distance to avoid edge-case clipping

	FHitResult OutHit;

	//check for objects in front of the character (don't want to teleport through or into solid objects)
	if (GetWorld()->LineTraceSingleByChannel(OutHit, CharacterLocation, LinetraceStop, ECC_Camera))
	{
		//if trace hit something, set different teleport Location 
		if ((CharacterLocation - OutHit.ImpactPoint).Size() > ClipSafetyDistance)
		{
			// line trace hit, but not near character -> we don't want to set the new location directly to the impact point otherwise the character may clip into map geometry
			NewLocation = OutHit.ImpactPoint - CharacterOwner->GetActorForwardVector() * ClipSafetyDistance;
		}
		else
		{
			// travel distance is too small (e.g: teleporting towards a wall while very close to it), teleport to current location
			NewLocation = CharacterLocation;
		}
	}

	UE_LOG(LogTemp, Display, TEXT("Teleport distance = %f"), (CharacterLocation - OutHit.ImpactPoint).Size());

	//teleport player to new location
	CharacterOwner->SetActorLocation(NewLocation, false, 0, ETeleportType::TeleportPhysics);

	Safe_bWantsToTeleport = false;
}



//----------------------------------------------------------------------------------------------------- SavedMove_Custom



//checks current move with new move to see if they can be combined and save bandwidth
bool UShooterCharacterMovement::FSavedMove_Custom::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
{
	//cast to custom SavedMove
	FSavedMove_Custom* NewCustomMove = static_cast<FSavedMove_Custom*>(NewMove.Get());

	//check if variable is different, if so no combination can be done
	if (Saved_bWantsToTeleport != NewCustomMove->Saved_bWantsToTeleport)
	{
		return false;
	}

	return FSavedMove_Character::CanCombineWith(NewMove, InCharacter, MaxDelta);
}



//reset savedmove object to be emptied
void UShooterCharacterMovement::FSavedMove_Custom::Clear()
{
	FSavedMove_Character::Clear();

	Saved_bWantsToTeleport = 0;
}



uint8 UShooterCharacterMovement::FSavedMove_Custom::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();

	//if wants to Teleport then activate custom flag 0 -> custom flag 0 is assigned to Teleportation movement functionality
	if (Saved_bWantsToTeleport) Result |= FLAG_Custom_0;

	return Result;
}



// setting the savedmove for the current snapshot of the CMC (capture the state data of the CMC: look through all Safe variables of our custom CMC and set their respective Saved variables) 
void UShooterCharacterMovement::FSavedMove_Custom::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	//super method
	FSavedMove_Character::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);

	UShooterCharacterMovement* CharacterMovement = Cast<UShooterCharacterMovement>(C->GetCharacterMovement());

	//save variable state in the SavedMove
	Saved_bWantsToTeleport = CharacterMovement->Safe_bWantsToTeleport;
}



// Retrieve data from the savedmove and apply it to the current state of the CMC
void UShooterCharacterMovement::FSavedMove_Custom::PrepMoveFor(ACharacter* C)
{
	Super::PrepMoveFor(C);

	UShooterCharacterMovement* CharacterMovement = Cast<UShooterCharacterMovement>(C->GetCharacterMovement());

	CharacterMovement->Safe_bWantsToTeleport = Saved_bWantsToTeleport;
}



//----------------------------------------------------------------------------------------------------- FNetworkPredictionData_Client_Custom



UShooterCharacterMovement::FNetworkPredictionData_Client_Custom::FNetworkPredictionData_Client_Custom(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{  //just calls superclass constructor
}



FSavedMovePtr UShooterCharacterMovement::FNetworkPredictionData_Client_Custom::AllocateNewMove()
{
	//allocate custom savedmove type
	return FSavedMovePtr(new FSavedMove_Custom());
}
