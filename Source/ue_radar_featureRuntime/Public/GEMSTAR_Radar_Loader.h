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
#include "Components/ActorComponent.h"
#include "GEMSTAR_Radar_Loader.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UE_RADAR_FEATURERUNTIME_API UGEMSTAR_Radar_Loader : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGEMSTAR_Radar_Loader();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
};
