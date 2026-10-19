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

#include "Interfaces/Sensor/SensorConfigurationData.h"
#include "Interfaces/Sensor/SensorConfigurationDispatcher.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RadarSensorTester.generated.h"

class AStaticMeshActor;

UCLASS()
class ARadarSensorTester : public AActor,
	public ISensorSpawnerDispatcher,
	public ISensorDataUpdate
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARadarSensorTester();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void RegisterSensorSpawner_Implementation(AActor* spawner) override;

	void UnregisterSensorSpawner_Implementation(AActor* spawner) override;

	void SensorViewDataUpdate_Implementation(FSensorView& sensorViewData, int TargetIndex, UTextureRenderTarget2D* cameraRenderTarget) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere)
	UStaticMesh* CubeMesh;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* CubeMaterial;

	UPROPERTY(EditAnywhere)
	AStaticMeshActor* CubeActor;
	AStaticMeshActor* TargetCubeActor;
	AStaticMeshActor* TargetCube2Actor;

	UPROPERTY(EditAnywhere)
	float speedMultiplier = 1;

	UPROPERTY(EditAnywhere)
	int rotationTimer = 1000;

private:
	int counter = 0;




	UPROPERTY(EditAnywhere)
	AActor* TestActorToInform;

private:
	TArray<AActor*> ActorsToInform;
	bool bSensorSpawned = false;
	bool bReportedWaiting = false;

	AActor* FindSensorSpawner() const;
};
