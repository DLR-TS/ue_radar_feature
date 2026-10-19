/*
 * Copyright (c) 2026 DLR e.V.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 */

#pragma once

#include "Subsystems/GemstarSimulationSubsystem.h"
#include "Interfaces/Sensor/SensorConfigurationData.h"
#include "RadarLog.h"
#include "SimpleRadar.h"
#include "Engine/World.h"
#include "Misc/DefaultValueHelper.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RadarSensorManager.generated.h"

UCLASS()
class ARadarSensorManager : public AActor,
	public ISensorConfigurationData,
	public IGemstarManager
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARadarSensorManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;	
	
	public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void Configure(TMap<FString, FInnerConfig> Configurations, UGemstarSimulationSubsystem* SimulationSubsystem);

protected:
	virtual void SpawnSensor_Implementation(FSensorConfigData& sensorConfiguration) override;

private:
	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedActors;
	UPROPERTY()
	TObjectPtr<UGemstarSimulationSubsystem> GemstarSimulationSubsystem;
	UPROPERTY()
	TArray<FSensorConfigData> SensorConfiguration;

	void SpawnSensor(FSensorConfigData& sensorConfiguration);
};
