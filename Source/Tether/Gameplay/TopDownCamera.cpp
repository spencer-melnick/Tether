// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "TopDownCamera.h"

#include "TopDownCameraComponent.h"

// Sets default values
ATopDownCamera::ATopDownCamera()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraComponent = CreateDefaultSubobject<UTopDownCameraComponent>(TEXT("Camera"));
	RootComponent = CameraComponent;
}


void ATopDownCamera::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
	if (CameraComponent)
	{
		CameraComponent->GetCameraView(DeltaTime, OutResult);
	}
}


// Called when the game starts or when spawned
void ATopDownCamera::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATopDownCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

