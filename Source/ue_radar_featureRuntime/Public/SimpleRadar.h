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

#include "CoreMinimal.h"
#include "RadarLog.h"
#include "GameFramework/Actor.h"
#include "Kismet/KismetMathLibrary.h"
#include "Sensors/GemstarSensor.h"
#include "Kismet/GameplayStatics.h"
#include "VelocitySource.h"
#include "Utils/ToOSI.h"
#include "osi_sensorview.pb.h"
#include "osi_sensorviewconfiguration.pb.h"
#include "SimpleRadar.generated.h"

constexpr float SPEED_OF_LIGHT = 299705543.0f; //in m/s in air 299792458.0f in vakuum;

USTRUCT()
struct FRadarHitResultAndIntesity
{
	GENERATED_BODY()
public:
	FHitResult HitResult;
	float Intesity = 0.f;
};

USTRUCT()
struct FSimpleRadarConfiguration
{
	GENERATED_BODY()

public:
	//packed config
	osi3::SensorViewConfiguration* SensorViewConfig = nullptr;
	//prepared configuration
	TArray<float> AzimuthAngles;
	TArray<float> ElevationAngles;
	uint32_t Number_of_rays_horizontal = 0;
	uint32_t Number_of_rays_vertical = 0;
	//raw results
	TArray<FRadarHitResultAndIntesity> RecordedHits;
	//packed results
	osi3::SensorView* SensorView = nullptr;

};

UCLASS()
class UE_RADAR_FEATURERUNTIME_API ASimpleRadar : public AGemstarSensor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASimpleRadar();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GEMSTAR")
	bool bDebugRayTraces = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GEMSTAR")
	bool bDebugReflections = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GEMSTAR")
	bool bDebugNormals = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GEMSTAR")
	bool bDebugData = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GEMSTAR")
	bool SupportMultithread = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GEMSTAR")
	float DistanceHalfFalloff = 2000.0;

	virtual void SetSensorConfiguration(FSensorConfigData NewSensorConfiguration) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	void GetScanData();
	virtual void Tick(float DeltaTime) override;
	FVector currentVelocity;
private:
	UPROPERTY()
	AActor* Parent = nullptr;

	FSimpleRadarConfiguration Sensor;
	FCollisionQueryParams TraceParams;

	bool bReportedNoDataTarget = false;

	bool firstTick = true;
	FVector oldPosition;

	void CreateTestConfig();
	void ShowSensorDebugInformation();
	void GenerateOSIMessage();
	float ComputeRadarSignalStrength(
		const FHitResult& Hit
	);
};
