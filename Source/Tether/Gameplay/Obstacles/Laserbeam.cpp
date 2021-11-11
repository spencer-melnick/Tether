// Copyright (c) 2021 Spencer Melnick, Stephen Melnick


#include "Gameplay/Obstacles/Laserbeam.h"

// Sets default values
ALaserbeam::ALaserbeam()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ALaserbeam::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ALaserbeam::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

