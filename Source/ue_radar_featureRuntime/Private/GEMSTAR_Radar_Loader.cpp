/*
 * Copyright (c) 2026 DLR e.V.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 */

#include "GEMSTAR_Radar_Loader.h"

#include "Subsystems/GemstarSimulationSubsystem.h"
#include "RadarSensorManager.h"
#include "RadarLog.h"

// Sets default values for this component's properties
UGEMSTAR_Radar_Loader::UGEMSTAR_Radar_Loader()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGEMSTAR_Radar_Loader::BeginPlay()
{
	Super::BeginPlay();

	UGemstarSimulationSubsystem* GemstarSimulationSubsystem = UGemstarSimulationSubsystem::Get(GetWorld());
	if (GemstarSimulationSubsystem)
	{
		FTransform SpawnTransform;

		FActorSpawnParameters SpawnParams;
		ARadarSensorManager *SpawnedRadarActor = GetWorld()->SpawnActor<ARadarSensorManager>(ARadarSensorManager::StaticClass(), SpawnTransform, SpawnParams);

		TMap<FString, FInnerConfig> config = GemstarSimulationSubsystem->RegisterManagerAndGetConfiguration(
			SpawnedRadarActor, "ue_radar_feature");
		SpawnedRadarActor->Configure(config, GemstarSimulationSubsystem);
		UE_LOG(Radar, Log, TEXT("RadarLoader: Registered and configured Radar"));
	}
	else
	{
		UE_LOG(Radar, Error, TEXT("RadarLoader: Cast of GemstarSimulationSubsystem was not successful"));
	}
}
