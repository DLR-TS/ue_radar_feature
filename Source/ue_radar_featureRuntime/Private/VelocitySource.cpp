/*
 * Copyright (c) 2026 DLR e.V.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 */

#include "VelocitySource.h"

// Sets default values
AVelocitySource::AVelocitySource()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AVelocitySource::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AVelocitySource::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	currentVelocity = calculateVelocity(DeltaTime);
}

FVector AVelocitySource::GetVelocity() const
{
	return currentVelocity;
}

FString AVelocitySource::GetMaterialFilePath()
{
	return MaterialFilePath;
}

FVector AVelocitySource::calculateVelocity(float DeltaTime)
{
	return currentVelocity;
}


