/*
 * Copyright (c) 2026 DLR e.V.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 */

#include "RadarSensorManager.h"

// Sets default values
ARadarSensorManager::ARadarSensorManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	AcceptChannel(GemstarInterfaceType::SENSORVIEW,
		[this](FDataTarget& Target, const FString& Producer)
		{
			for (AActor* RadarActor : SpawnedActors)
			{
				if (!IsValid(RadarActor) || RadarActor->GetName() != Producer) { continue; }
				ASimpleRadar* SimpleRadar = Cast<ASimpleRadar>(RadarActor);
				if (SimpleRadar == nullptr) { return false; }
				SimpleRadar->AddDataTarget(Target);
				Target.Actor->AddTickPrerequisiteActor(RadarActor);
				return true;
			}
			return false;
		},
		[this](FDataTarget& Target, const FString&)
		{
			bool bRemoved = false;
			for (AActor* RadarActor : SpawnedActors)
			{
				if (ASimpleRadar* SimpleRadar = Cast<ASimpleRadar>(RadarActor))
				{
					SimpleRadar->RemoveDataTarget(Target);
					Target.Actor->RemoveTickPrerequisiteActor(SimpleRadar);
					bRemoved = true;
				}
			}
			return bRemoved;
		});
}

// Called when the game starts or when spawned
void ARadarSensorManager::BeginPlay()
{
	Super::BeginPlay();
	
}

void ARadarSensorManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(GemstarSimulationSubsystem)) {
		GemstarSimulationSubsystem->UnregisterManager(this);
		GemstarSimulationSubsystem = nullptr;
	}

	for (int32 i = SpawnedActors.Num() - 1; i >= 0; --i){
		AActor* Sensor = SpawnedActors[i];
		
		if (!IsValid(Sensor)) {
			continue;
		}

		RemoveTickPrerequisiteActor(Sensor);

		Sensor->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors);
		Sensor->Destroy();
	}

	SpawnedActors.Reset();
	SensorConfiguration.Reset();

	Super::EndPlay(EndPlayReason);
}

// Called every frame
void ARadarSensorManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ARadarSensorManager::Configure(TMap<FString, FInnerConfig> Configurations, UGemstarSimulationSubsystem* SimulationSubsystem) {
	GemstarSimulationSubsystem = SimulationSubsystem;

	for (const TPair<FString, FInnerConfig>& Pair : Configurations)
	{
		FSensorConfigData config;
		config.ID = FName(Pair.Key);
		if (const FString* ValueStr = Pair.Value.Entries.Find("Mount")) {
			config.Mount = *ValueStr;
		}
		double x = 0.0;
		if (const FString* ValueStr = Pair.Value.Entries.Find("MountingPositionX"))
		{
			FDefaultValueHelper::ParseDouble(*ValueStr, x);
		}
		double y = 0.0;
		if (const FString* ValueStr = Pair.Value.Entries.Find("MountingPositionY"))
		{
			FDefaultValueHelper::ParseDouble(*ValueStr, y);
		}
		double z = 0.0;
		if (const FString* ValueStr = Pair.Value.Entries.Find("MountingPositionZ"))
		{
			FDefaultValueHelper::ParseDouble(*ValueStr, z);
		}
		config.Offset.SetLocation(FVector(x, y, z));

		double roll = 0.0;
		if (const FString* ValueStr = Pair.Value.Entries.Find("MountingOrientationRoll"))
		{
			FDefaultValueHelper::ParseDouble(*ValueStr, roll);
		}
		double pitch = 0.0;
		if (const FString* ValueStr = Pair.Value.Entries.Find("MountingOrientationPitch"))
		{
			FDefaultValueHelper::ParseDouble(*ValueStr, pitch);
		}
		double yaw = 0.0;
		if (const FString* ValueStr = Pair.Value.Entries.Find("MountingOrientationYaw"))
		{
			FDefaultValueHelper::ParseDouble(*ValueStr, yaw);
		}
		config.Offset.SetRotation(FRotator(pitch, yaw, roll).Quaternion());

		double width = 0.0;
		if (const FString* ValueStr = Pair.Value.Entries.Find("NumberOfRaysHorizontal"))
		{
			FDefaultValueHelper::ParseDouble(*ValueStr, width);
		}
		double height = 0.0;
		if (const FString* ValueStr = Pair.Value.Entries.Find("NumberOfRaysVertical"))
		{
			FDefaultValueHelper::ParseDouble(*ValueStr, height);
		}
		double FoVHorizontal = 0.0;
		if (const FString* ValueStr = Pair.Value.Entries.Find("FoVHorizontal"))
		{
			FDefaultValueHelper::ParseDouble(*ValueStr, FoVHorizontal);
		}
		double FoVVertical = 0.0;
		if (const FString* ValueStr = Pair.Value.Entries.Find("FoVVertical"))
		{
			FDefaultValueHelper::ParseDouble(*ValueStr, FoVVertical);
		}
		float Range = 0.0;
		if (const FString* ValueStr = Pair.Value.Entries.Find("Range"))
		{
			FDefaultValueHelper::ParseFloat(*ValueStr, Range);
		}
		bool NoFalloff = false;
		if (const FString* ValueStr = Pair.Value.Entries.Find("NoFalloff"))
		{
			FDefaultValueHelper::ParseBool(*ValueStr, NoFalloff);
		}
		float distanceHalfFalloff = 0.0;
		if (const FString* ValueStr = Pair.Value.Entries.Find("DistanceHalfFalloff"))
		{
			FDefaultValueHelper::ParseFloat(*ValueStr, distanceHalfFalloff);
		}
		config.Radar.NumberOfRaysHorizontal = width;
		config.Radar.NumberOfRaysVertical = height;
		config.Radar.FieldOfViewHorizontal = FMath::DegreesToRadians(FoVHorizontal);
		config.Radar.FieldOfViewVertical = FMath::DegreesToRadians(FoVVertical);
		// DistanceHalfFalloff in Meter
		config.Radar.DistanceHalfFalloff = distanceHalfFalloff * 100.0f;
		SensorConfiguration.Add(config);
	}
	for (auto& config : SensorConfiguration)
	{
		SpawnSensor(config);
	}
}

void ARadarSensorManager::SpawnSensor(FSensorConfigData& sensorConfiguration) {
	UWorld* World = GetWorld();
	if (World)
	{
		FActorSpawnParameters params;
		params.Name = sensorConfiguration.ID;
		ASimpleRadar* SpawnedActor = World->SpawnActor<ASimpleRadar>(ASimpleRadar::StaticClass(), FTransform(), params);
		if (SpawnedActor == nullptr) {
			UE_LOG(Radar, Log, TEXT("RadarSensorManager: Radar spawn failed for %s"), *SpawnedActor->GetName());
			return;
		}

		SpawnedActor->SetSensorConfiguration(sensorConfiguration);

		UE_LOG(Radar, Log, TEXT("RadarSensorManager: Radar spawned %s"), *SpawnedActor->GetName());
		if (!sensorConfiguration.Mount.IsEmpty()) {
			IGemstarManager::Execute_MountOn(GemstarSimulationSubsystem, sensorConfiguration.Mount, SpawnedActor);
		}
		else {
			ILocalTransformUpdater::Execute_UpdateTransformAfterAttachment(SpawnedActor);
		}
		AddTickPrerequisiteActor(SpawnedActor);
		SpawnedActors.Add(SpawnedActor);
	}
}

void ARadarSensorManager::SpawnSensor_Implementation(FSensorConfigData& sensorConfiguration)
{
	if (sensorConfiguration.Type != SensorType::RADAR) {
		return;
	}

	UWorld* World = GetWorld();
	if (World)
	{
		FActorSpawnParameters params;
		if (sensorConfiguration.ID.IsNone()) {
			params.Name = "Unnamed_Radar_Sensor";
		}
		else {
			params.Name = sensorConfiguration.ID;
		}
		params.Owner = sensorConfiguration.Vehicle;

		ASimpleRadar* SpawnedActor = World->SpawnActor<ASimpleRadar>(ASimpleRadar::StaticClass(), FTransform(), params);
		SpawnedActor->AttachToActor(sensorConfiguration.Vehicle, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true));
		SpawnedActor->SetSensorConfiguration(sensorConfiguration);
		SpawnedActor->SetActorRelativeTransform(sensorConfiguration.Offset);

		if (SpawnedActor)
		{
			UE_LOG(Radar, Warning, TEXT("RadarSensorManager: spawned radar sensor: %s"), *SpawnedActor->GetName());
			SpawnedActors.Add(SpawnedActor);
		}
	}
}
