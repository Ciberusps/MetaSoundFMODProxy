// Pavel Penkov 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundBase.h"
#include "FMODEvent.h"
#include "SoundFMODEvent.generated.h"

class UFMODProxySubsystem;
class UFMODWaitingWave;

/**
 * USoundBase-derived asset that plays an FMOD Studio Event when parsed by UE audio.
 * Produces a silent procedural wave to keep the audio graph alive until the FMOD event ends.
 */
UCLASS(BlueprintType, meta=(DisplayName="FMOD Sound (Event)"))
class METASOUNDFMODPROXY_API USoundFMODEvent : public USoundBase
{
	GENERATED_BODY()

public:
	USoundFMODEvent(const FObjectInitializer& ObjectInitializer);

	/** The FMOD event to play */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FMOD")
	TObjectPtr<UFMODEvent> FMODEvent;

	/** Auto destroy the native instance when it finishes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FMOD")
	bool bAutoDestroy;

	/** Optional parameters to set after starting */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FMOD")
	TMap<FName, float> EventParameters;

#if WITH_EDITORONLY_DATA
	/** Preview as 2D when auditioning in editor */
	UPROPERTY(EditAnywhere, Category = "FMOD|Preview")
	bool bPreviewAs2D;
#endif

	// USoundBase interface
	virtual void Parse(
		FAudioDevice* AudioDevice,
		const UPTRINT NodeWaveInstanceHash,
		FActiveSound& ActiveSound,
		const FSoundParseParameters& ParseParams,
		TArray<FWaveInstance*>& WaveInstances
	) override;

protected:
	/** Create and push a waiting wave to WaveInstances, returning the created object */
	UFMODWaitingWave* CreateWaitingWave(
		FAudioDevice* AudioDevice,
		const UPTRINT NodeWaveInstanceHash,
		FActiveSound& ActiveSound,
		const FSoundParseParameters& ParseParams,
		TArray<FWaveInstance*>& WaveInstances
	) const;
};


