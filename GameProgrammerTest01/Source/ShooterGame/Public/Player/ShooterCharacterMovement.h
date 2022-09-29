/**
 *  Character Movement Controller component with custom moves
 */

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ShooterCharacterMovement.generated.h"

UCLASS()
class UShooterCharacterMovement : public UCharacterMovementComponent
{
	GENERATED_UCLASS_BODY()

	virtual float GetMaxSpeed() const override;



	//--------------------------------------------------------------------helper classes

	/*
	* Custom Saved Move containing variables used for the network replication of our custom movement abilities
	*/
	class FSavedMove_Custom : public FSavedMove_Character
	{
		typedef FSavedMove_Character Super;

		uint8 Saved_bWantsToTeleport : 1; //we need to save a variable telling the server wether or not we want to Teleport

		virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
		virtual void Clear() override;
		virtual uint8 GetCompressedFlags() const override;
		virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override;
		virtual void PrepMoveFor(ACharacter* C) override;
	};



	//helper class used to tell the CMC that we'll use our FSavedMove_Custom
	class FNetworkPredictionData_Client_Custom : public FNetworkPredictionData_Client_Character
	{
	public:
		FNetworkPredictionData_Client_Custom(const UCharacterMovementComponent& ClientMovement);

		typedef FNetworkPredictionData_Client_Character Super;

		virtual FSavedMovePtr AllocateNewMove() override;
	};

	//-----------------------------------------------------------------------------------



	//flag set on the client when trying to teleport
	bool Safe_bWantsToTeleport;

	UPROPERTY(EditDefaultsOnly)
	float TeleportDistance;

	float ClipSafetyDistance = 70.0f; //a bit larger than the CapsuleComponent's radius

public:

	//used to indicate that we want to use our NetworkPredictionData_Client_Custom class
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

protected:

	// set state of CharacterMovementController on the server based on the flags
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;

	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;

public:

	//activate the Safe_bWantsToTeleport flag
	UFUNCTION(BlueprintCallable)
	void TeleportPressed();

private:
	//teleport ability implementation
	void Teleport();

};

