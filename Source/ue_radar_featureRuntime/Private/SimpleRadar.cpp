/*
 * Copyright (c) 2026 DLR e.V.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 */

#include "SimpleRadar.h"

// Sets default values
ASimpleRadar::ASimpleRadar()
{
}

void ASimpleRadar::BeginPlay()
{
	Super::BeginPlay();
	CreateTestConfig();
	
	//initialise internal speed calculation
	currentVelocity = FVector::ZeroVector;
	oldPosition = GetActorLocation();
	// complex collisions: true
	TraceParams = FCollisionQueryParams(TEXT("RadioTrace"), true, GetOwner());
	TraceParams.bReturnPhysicalMaterial = true;
	TraceParams.bTraceComplex = true;
}

// Called every frame
void ASimpleRadar::Tick(float DeltaTime){
	if (!TickActive) return;
	//internal speed calculation
	if (firstTick) {
		firstTick = false;
		currentVelocity = FVector::ZeroVector;
	} else {
		currentVelocity = (GetActorLocation() - oldPosition) / DeltaTime;
		oldPosition = GetActorLocation();
	}

	GetScanData();
	if (bDebugRayTraces || bDebugReflections || bDebugNormals)
		ShowSensorDebugInformation();
	if (PublishingActive)
	{
		GenerateOSIMessage();
		SendData(*Sensor.SensorView);
	}
}

void ASimpleRadar::GetScanData() {

	// Get owner's location and rotation
	const FVector RadarPosition = this->GetActorLocation();
	const FRotator RadarRotation = this->GetActorRotation();

	// Initialize array for raycast result
	Sensor.RecordedHits.Init(FRadarHitResultAndIntesity(), Sensor.Number_of_rays_horizontal * Sensor.Number_of_rays_vertical);

	// Calculate batch size for 'ParallelFor' based on workable thread
	const int32 ThreadNum = FPlatformMisc::NumberOfWorkerThreadsToSpawn();
	const int32 DivideEnd = FMath::FloorToInt((float)(Sensor.RecordedHits.Num() / ThreadNum));

	ParallelFor(
		ThreadNum,
		[&](int32 PFIndex)
		{
			int32 StartAt = PFIndex * DivideEnd;
			if (StartAt >= Sensor.RecordedHits.Num()) {
				return;
			}

			int32 EndAt = StartAt + DivideEnd;
			if (PFIndex == (ThreadNum - 1)) {
				EndAt = Sensor.RecordedHits.Num();
			}

			for (int32 Index = StartAt; Index < EndAt; ++Index) {
				const float HAngle = Sensor.AzimuthAngles[Index % Sensor.Number_of_rays_horizontal];
				const float VAngle = Sensor.ElevationAngles[Index / Sensor.Number_of_rays_horizontal];

				FRotator RadioRotation(FMath::RadiansToDegrees(VAngle), FMath::RadiansToDegrees(HAngle), 0.f);
				FRotator Rotation = UKismetMathLibrary::ComposeRotators(RadioRotation, RadarRotation);

				FVector EndPoint = RadarPosition + Sensor.SensorViewConfig->range() * 100 * UKismetMathLibrary::GetForwardVector(Rotation);

				GetWorld()->LineTraceSingleByChannel(
					Sensor.RecordedHits[Index].HitResult, RadarPosition, EndPoint, ECC_Visibility, TraceParams, FCollisionResponseParams::DefaultResponseParam);
				if (Sensor.RecordedHits[Index].HitResult.bBlockingHit) {
					Sensor.RecordedHits[Index].Intesity = ComputeRadarSignalStrength(Sensor.RecordedHits[Index].HitResult);
				}
			}
		},
		!SupportMultithread
	);

}

void ASimpleRadar::GenerateOSIMessage() {
	delete Sensor.SensorView;
	Sensor.SensorView = new osi3::SensorView();
	auto* radarSensorView = Sensor.SensorView->add_radar_sensor_view();
	auto* viewConfiguration = radarSensorView->mutable_view_configuration();
	if (Sensor.SensorViewConfig != nullptr && Sensor.SensorViewConfig->radar_sensor_view_configuration_size())
		viewConfiguration->CopyFrom(Sensor.SensorViewConfig->radar_sensor_view_configuration(0));

	
	for (int Index = 0; Index != Sensor.RecordedHits.Num(); Index++) {
		FRadarHitResultAndIntesity hit = Sensor.RecordedHits[Index];
		auto* reflection = radarSensorView->add_reflection();
		
		reflection->set_time_of_flight(2 * 0.01 * hit.HitResult.Distance / SPEED_OF_LIGHT);
		reflection->set_source_horizontal_angle(Sensor.AzimuthAngles[Index % Sensor.Number_of_rays_horizontal]);
		reflection->set_source_vertical_angle(Sensor.ElevationAngles[Index / Sensor.Number_of_rays_horizontal]);
		reflection->set_doppler_shift(0);
		reflection->set_signal_strength(hit.Intesity);
		FVector hitVelocity = FVector::ZeroVector;
		AActor* hitTarget = hit.HitResult.GetActor();
		if (hit.HitResult.bBlockingHit) {
			if (hitTarget->Implements<USensorReflectionData>()) {
				hitVelocity = Cast<ISensorReflectionData>(hitTarget)->GetVelocity();
			}
			else {
				hitVelocity = hitTarget->GetVelocity();
			}
			FString hitName = hit.HitResult.GetHitObjectHandle().GetName();
			FVector relativePostition = hit.HitResult.ImpactPoint - hit.HitResult.TraceStart;
			FVector relativeVelocity = hitVelocity - currentVelocity;
			float distance = relativePostition.Size();
			float radialVelocity = FVector::DotProduct(relativeVelocity, relativePostition / distance);
			double frequency = viewConfiguration->emitter_frequency();
			double doppler_shift = 2 * (radialVelocity * 0.01) * frequency / SPEED_OF_LIGHT;
			reflection->set_doppler_shift(doppler_shift);
			//Debug Data
			//if (hitName.Contains("BP_Simple")) {UE_LOG(Radar, Log, TEXT("Hit_%d: %s, vHero= %s, vTarget= %s, vR= %.2f, dS= %.2f, sS= %.2f" ), Index, *hitName, *currentVelocity.ToCompactString(), *hitVelocity.ToCompactString(), radialVelocity, doppler_shift, hit.Intesity);}
		}
	}
	
	Parent = GetAttachParentActor();

	if (Parent != nullptr) {
		auto* moving_object = Sensor.SensorView->mutable_global_ground_truth()->add_moving_object();
		uint32_t uniqueID = Parent->GetUniqueID();
		moving_object->mutable_id()->set_value(uniqueID);
		moving_object->mutable_base()->set_allocated_position(ToOSI::Position(Parent->GetActorLocation()));
		moving_object->mutable_base()->set_allocated_orientation(ToOSI::Orientation3d(Parent->GetActorQuat()));
		Sensor.SensorView->mutable_global_ground_truth()->mutable_host_vehicle_id()->set_value(uniqueID);
	}
}

float ASimpleRadar::ComputeRadarSignalStrength(const FHitResult& Hit)
{
	// Frequenz 77GHz, Wellenl�nge= 3.9mm, Sendeleistung 0.01W, Antennengewinn 30dB ca. 1000(linear), Radarziel R�ckstreuquerschnitt (Auto) 10m2, R = Entfernung zum Ziel
	// Pr = (7.5 * 10^-5 * Radarziel R�ckstreuquerschnitt) / R^4
	float backscatterValue = 10; // default for cars, need adjustment for bikes and pedestrians

	float distanceToTarget = (Hit.ImpactPoint - Hit.TraceStart).Size() * 0.01; //in meter
	distanceToTarget = FMath::Max(distanceToTarget, 0.01f); // prevent division by zero
	float signalStrengthInWatt = (7.5 * 0.00001 * backscatterValue) / FMath::Pow(distanceToTarget, 4); 

	float signalStrengthInDB = 10 * FMath::LogX(10, (signalStrengthInWatt / FMath::Pow(10.0, -12.0)));

	// normalise dB signal
	float scaledDBsignal = (signalStrengthInDB) / 100.0;

	return scaledDBsignal;
}

void ASimpleRadar::SetSensorConfiguration(FSensorConfigData NewSensorConfiguration) {
	Super::SetSensorConfiguration(NewSensorConfiguration);
	Sensor.SensorViewConfig->clear_radar_sensor_view_configuration();
	auto* conf = Sensor.SensorViewConfig->add_radar_sensor_view_configuration();
	conf->set_field_of_view_horizontal(SensorConfiguration.Radar.FieldOfViewHorizontal);
	conf->set_field_of_view_vertical(SensorConfiguration.Radar.FieldOfViewVertical);
	conf->set_number_of_rays_horizontal(SensorConfiguration.Radar.NumberOfRaysHorizontal);
	conf->set_number_of_rays_vertical(SensorConfiguration.Radar.NumberOfRaysVertical);
	conf->mutable_mounting_position()->set_allocated_position(
		ToOSI::Position(SensorConfiguration.Offset.GetLocation()));
	conf->mutable_mounting_position()->set_allocated_orientation(
		ToOSI::Orientation3d(SensorConfiguration.Offset.GetRotation()));
	conf->set_emitter_frequency(77000000000.0); //in Hz
	//calculate angles

	Sensor.AzimuthAngles.Empty();
	Sensor.ElevationAngles.Empty();
	Sensor.Number_of_rays_horizontal = Sensor.SensorViewConfig->radar_sensor_view_configuration(0).number_of_rays_horizontal();
	for (uint32_t i = 0; i < Sensor.Number_of_rays_horizontal; i++) {
		Sensor.AzimuthAngles.Add(-0.5 * Sensor.SensorViewConfig->radar_sensor_view_configuration(0).field_of_view_horizontal()
			+ Sensor.SensorViewConfig->radar_sensor_view_configuration(0).field_of_view_horizontal() / (Sensor.Number_of_rays_horizontal - 1) * i);
	}
	Sensor.Number_of_rays_vertical = Sensor.SensorViewConfig->radar_sensor_view_configuration(0).number_of_rays_vertical();
	for (uint32_t i = 0; i < Sensor.Number_of_rays_vertical; i++) {
		Sensor.ElevationAngles.Add(-0.5 * Sensor.SensorViewConfig->radar_sensor_view_configuration(0).field_of_view_vertical()
			+ Sensor.SensorViewConfig->radar_sensor_view_configuration(0).field_of_view_vertical() / (Sensor.Number_of_rays_vertical - 1) * i);
	}
}

void ASimpleRadar::CreateTestConfig() {
	Sensor.SensorViewConfig = new osi3::SensorViewConfiguration();
	Sensor.SensorViewConfig->set_range(10);
}

void ASimpleRadar::ShowSensorDebugInformation() {
	int noHitCounter = 0;
	for (auto& hit : Sensor.RecordedHits) {
		if (hit.HitResult.bBlockingHit) {
			if (bDebugRayTraces)
				DrawDebugLine(GetWorld(), hit.HitResult.TraceStart, hit.HitResult.ImpactPoint, FColor::Green, false, 0.0f, 0, 1.0f);
			if (bDebugReflections)
				DrawDebugPoint(GetWorld(), hit.HitResult.ImpactPoint, 10.0f, FColor::Red, false, 0.0f);
			if (bDebugNormals)
				DrawDebugLine(GetWorld(), hit.HitResult.ImpactPoint, hit.HitResult.ImpactPoint + hit.HitResult.ImpactNormal * 10, FColor::Blue, false, 0.0f, 0, 1.0f);
		}
		else {
			noHitCounter++;
		}
	}
	UE_LOG(Radar, Log, TEXT("SimpleRadar: No reflection from radar: %d of %d"), noHitCounter, Sensor.RecordedHits.Num());
}