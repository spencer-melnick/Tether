// Copyright (c) 2021 Spencer Melnick, Stephen Melnick

#pragma once

#include "CoreMinimal.h"
#include "BeamComponent.generated.h"

class ABeamController;
class UBeamComponent;


UINTERFACE(Blueprintable)
class UBeamTarget : public UInterface
{
	GENERATED_BODY()
};

/** Simple interface for actors that have a beam component */
class IBeamTarget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	UBeamComponent* GetBeamComponent() const;
	
	virtual UBeamComponent* GetBeamComponent_Implementation() const { return nullptr; }
};


/** Flags representing the current behavior of the beam component */
UENUM(meta=(Bitflags, UseEnumValuesAsMaskValuesInEditor="true"))
enum class EBeamComponentMode : uint8
{
	None					= 0			UMETA(Hidden),
	Required				= 1 << 0,
	Connectable				= 1 << 1,
};
ENUM_CLASS_FLAGS(EBeamComponentMode);


/** Flags representing the current status of the beam component */
UENUM(meta=(Bitflags, UseEnumValuesAsMaskValuesInEditor="true"))
enum class EBeamComponentStatus : uint8
{
	None					= 0			UMETA(Hidden),
	Tracked					= 1 << 0,
	Visible					= 1 << 1,
	Connected				= 1 << 2
};
ENUM_CLASS_FLAGS(EBeamComponentStatus);


/**
 * Component that indicates where a beam can be targeted as well as the active state of the beam
 */
UCLASS(meta=(BlueprintSpawnableComponent))
class UBeamComponent : public USceneComponent
{	
	GENERATED_BODY()

	friend class ABeamController;

public:

	UBeamComponent();


	// Actor component interface

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


	// Utility functions

	UFUNCTION(BlueprintCallable)
	static UBeamComponent* GetComponentFromActor(const AActor* Actor);
	

	// Accessors

	UFUNCTION(BlueprintPure)
	ABeamController* GetBeamController() const { return BeamController; }

	UFUNCTION(BlueprintPure)
	EBeamComponentMode GetMode() const { return Mode; }

	UFUNCTION(BlueprintPure)
	EBeamComponentStatus GetStatus() const { return Status; }

	UFUNCTION(BlueprintPure)
	FVector GetEffectLocation() const;

	UFUNCTION(BlueprintCallable)
	void SetMode(EBeamComponentMode NewMode);


	// Delegates

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStatusChanged, EBeamComponentStatus, OldStatus, EBeamComponentStatus, NewStatus);

	UPROPERTY(BlueprintAssignable)
	FOnStatusChanged OnStatusChanged;


	// Editor properties

	UPROPERTY(EditAnywhere, meta=(Bitmask, BitmaskEnum="EBeamComponentMode"))
	int32 DefaultMode = 0;

	UPROPERTY(EditAnywhere, meta=(GetOptions="GetAttachmentSockets"))
	FName EffectLocationSocket;

private:

	void SetStatus(EBeamComponentStatus NewStatus);

	UFUNCTION()
	TArray<FName> GetAttachmentSockets() const;
	

	UPROPERTY()
	ABeamController* BeamController;

	EBeamComponentMode Mode = EBeamComponentMode::None;

	EBeamComponentStatus Status = EBeamComponentStatus::None;
};
