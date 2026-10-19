/*
 * Copyright (c) 2026 DLR e.V.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 */

#include "RadarSensorTester.h"

#include "EngineUtils.h"
#include "RadarSensorManager.h"
#include "Engine/StaticMeshActor.h"
#include "RadarLog.h"

// Sets default values
ARadarSensorTester::ARadarSensorTester()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeAsset.Succeeded())
	{
		CubeMesh = CubeAsset.Object;
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialAsset(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (MaterialAsset.Succeeded())
	{
		CubeMaterial = MaterialAsset.Object;
	}
}

// Called when the game starts or when spawned
void ARadarSensorTester::BeginPlay()
{
	Super::BeginPlay();

	if (CubeMesh)
	{
		FActorSpawnParameters SpawnParams;
		CubeActor = GetWorld()->SpawnActor<AStaticMeshActor>(GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
		//TargetCubeActor = GetWorld()->SpawnActor<AStaticMeshActor>(GetActorLocation() + FVector(1000,-200,50), FRotator::ZeroRotator, SpawnParams);
		//TargetCube2Actor = GetWorld()->SpawnActor<AStaticMeshActor>(GetActorLocation() + FVector(-500, 100, 50), FRotator::ZeroRotator, SpawnParams);


		if (CubeActor)// and TargetCubeActor and TargetCube2Actor)
		{
			CubeActor->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
			CubeActor->GetStaticMeshComponent()->SetMaterial(0, CubeMaterial);
			CubeActor->SetMobility(EComponentMobility::Movable);
			//CubeActor->SetActorLocation(CubeActor->GetActorLocation() + FVector(0, 0, 50));
			//TargetCubeActor->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
			//TargetCubeActor->GetStaticMeshComponent()->SetMaterial(0, CubeMaterial);
			//TargetCubeActor->SetMobility(EComponentMobility::Movable);
			//TargetCube2Actor->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
			//TargetCube2Actor->GetStaticMeshComponent()->SetMaterial(0, CubeMaterial);
			//TargetCube2Actor->SetMobility(EComponentMobility::Movable);
		}
	}
}

// Called every frame
void ARadarSensorTester::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (counter <= rotationTimer) {
		CubeActor->SetActorLocation(CubeActor->GetActorLocation() + FVector(1 * speedMultiplier, 0, 0));
		CubeActor->SetActorRotation(CubeActor->GetActorRotation() + FRotator(0, 0, 0));
	}
	else {
		CubeActor->SetActorLocation(CubeActor->GetActorLocation() + FVector(0, 0, 0));
		CubeActor->SetActorRotation(CubeActor->GetActorRotation() + FRotator(0, 1 * speedMultiplier, 0));
	}
	//TargetCubeActor->SetActorLocation(TargetCubeActor->GetActorLocation() + FVector(0, 1, 0));
	//TargetCubeActor->SetActorRotation(CubeActor->GetActorRotation() + FRotator(0, 0, 0));

	//TargetCube2Actor->SetActorLocation(TargetCube2Actor->GetActorLocation() + FVector(3, 0, 0));
	//TargetCube2Actor->SetActorRotation(CubeActor->GetActorRotation() + FRotator(0, 0, 0));


	if (bSensorSpawned) {
		return;
	}
	AActor* Spawner = FindSensorSpawner();

	if (Spawner == nullptr) {

		if (!bReportedWaiting) {
			bReportedWaiting = true;
			UE_LOG(Radar, Log, TEXT("RadarSensorTester: no radar manager yet, retrying every tick"));
		}
		return;
	}

	FSensorConfigData data;
	data.ID = FName(TEXT("radar:cube"));
	data.Type = SensorType::RADAR;
	data.Vehicle = CubeActor;
	data.Offset = FTransform(FRotator(0, 0, 0), FVector(0, 0, 100), FVector(1, 1, 1));
	data.Radar.FieldOfViewHorizontal = PI / 4;
	data.Radar.FieldOfViewVertical = PI / 4;
	data.Radar.NumberOfRaysHorizontal = 20;
	data.Radar.NumberOfRaysVertical = 20;
	if (TestActorToInform == nullptr)
	{
		data.DataTargets.Add({ this, 0 });
	}
	else {
		data.DataTargets.Add({ TestActorToInform, 0 });
	}

	ISensorConfigurationData::Execute_SpawnSensor(Spawner, data);
	bSensorSpawned = true;
	UE_LOG(Radar, Log, TEXT("RadarSensorTester: radar:cube spawned on %s"), *CubeActor->GetName());

}

AActor* ARadarSensorTester::FindSensorSpawner() const {
	
	if (ActorsToInform.Num()) {
		return ActorsToInform[0];
	}
	TActorIterator<ARadarSensorManager> Manager(GetWorld());
	return Manager ? *Manager : nullptr;
}

void ARadarSensorTester::RegisterSensorSpawner_Implementation(AActor* spawner)
{
	if (spawner && spawner->Implements<USensorConfigurationData>()) {
		ActorsToInform.AddUnique(spawner);
		UE_LOG(Radar, Log, TEXT("RadarSensorTester: tried to register an actor with USensorConfigurationData."));
	}
	else {
		UE_LOG(Radar, Log, TEXT("RadarSensorTester: tried to register an actor without USensorConfigurationData."));
	}
}

void ARadarSensorTester::UnregisterSensorSpawner_Implementation(AActor* spawner)
{
	if (spawner && spawner->Implements<USensorConfigurationData>()) {
		ActorsToInform.Remove(spawner);
	}
}

void ARadarSensorTester::SensorViewDataUpdate_Implementation(FSensorView& sensorViewData, int TargetIndex, UTextureRenderTarget2D* cameraRenderTarget)
{
	check(sensorViewData.sensorview->radar_sensor_view_size() == 1);
	UE_LOG(Radar, Log, TEXT("RadarSensorTester: OSI Radar data size in Gamethread: %d"),
		sensorViewData.sensorview->radar_sensor_view(0).reflection_size()
	);
}

