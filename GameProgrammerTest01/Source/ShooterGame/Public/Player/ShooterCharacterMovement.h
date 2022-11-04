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

	/** Custom Saved Move containing variables used for the network replication of our custom movement abilities */
	class FSavedMove_Custom : public FSavedMove_Character
	{
		typedef FSavedMove_Character Super;

		uint8 Saved_bWantsToTeleport : 1; //we need to save a variable telling the server wether or not we want to Teleport
		uint8 Saved_bWantsToJetpack : 1;
		uint8 Saved_bWantsToWalljump : 1;

		virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
		virtual void Clear() override;
		virtual uint8 GetCompressedFlags() const override;
		virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override;
		virtual void PrepMoveFor(ACharacter* C) override;
	};



	/** helper class used to tell the CMC that we'll use our FSavedMove_Custom */
	class FNetworkPredictionData_Client_Custom : public FNetworkPredictionData_Client_Character
	{
	public:
		FNetworkPredictionData_Client_Custom(const UCharacterMovementComponent& ClientMovement);

		typedef FNetworkPredictionData_Client_Character Super;

		virtual FSavedMovePtr AllocateNewMove() override;
	};



	/** time delta between frames */
	float DeltaTime; 

	/** flag toggled when performing teleport ability */
	bool Safe_bWantsToTeleport;

	/** flag toggled when performing jetpack ability */
	bool Safe_bWantsToJetpack;

	/** flag toggled when performing wall jump ability */
	bool Safe_bWantsToWalljump;

	/** maximum distance from starting position to position after teleport */
	UPROPERTY(EditDefaultsOnly, Category = "Movement Abilities")
	float TeleportDistance;

	/** distance to offset teleport to avoid clipping in case of a teleport near map geometry, a bit larger than the CapsuleComponent's radius */
	float TeleportClipSafetyDistance = 70.0f;

	/** maximum vertical velocity increase value */
	UPROPERTY(EditDefaultsOnly, Category = "Movement Abilities")
	float JetpackMaxForce;

	/** rate at which JetpackForce is increased */
	UPROPERTY(EditDefaultsOnly, Category = "Movement Abilities")
	float JetpackForceIncreaseRate;

	/** Starting vertical velocity at jetpack activation */
	UPROPERTY(EditDefaultsOnly, Category = "Movement Abilities")
	float JetpackInitialForce;

	/** vertical velocity increase value */
	float JetpackForce;

	/** WallJump lateral force pushing chatracter away from the wall */
	UPROPERTY(EditDefaultsOnly, Category = "Movement Abilities")
	float WallJumpLateralForce;

	/** normal vector to the wall that character is jumping from */
	FVector WallNormal;

protected:

	/** set state of CharacterMovementController on the server based on the flags */
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;

	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;

public:

	/** used to indicate that we want to use our NetworkPredictionData_Client_Custom class */
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

	/** DeltaTime variable setter */
	void SetDeltaTime(float DeltaSeconds);

	/** activate the Safe_bWantsToTeleport flag */
	void TeleportPressed();

	/** toggle Safe_bWantsToJetpack flag to activate jetpack ability */
	void JetpackPressed();

	/** toggle Safe_bWantsToJetpack flag to stop jetpack ability */
	void JetpackReleased();

	/** activate the Safe_bWantsToWalljump flag */
	void WalljumpPressed(FVector Normal);

private:

	/** Teleport ability implementation */
	void Teleport();

	/** Jetpack ability implementation */
	void Jetpack();

	/** Wall jump ability implementation */
	void Walljump();

	/** DeltaTime variable setter via RPC to enable correct client prediction/server replication */
	UFUNCTION(Server, Reliable)
	void Server_SetDeltaTime(float DeltaSeconds);

	/** WallNormal variable setter via RPC to enable correct client prediction/server replication */
	UFUNCTION(Server, Reliable)
	void Server_SetWallNormal(FVector Normal);
};

