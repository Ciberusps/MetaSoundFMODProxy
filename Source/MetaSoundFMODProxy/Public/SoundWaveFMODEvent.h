// Pavel Penkov 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundWaveProcedural.h"
#include "FMODEvent.h"
#include "SoundWaveFMODEvent.generated.h"

class UFMODProxySubsystem;

/**
 * Procedural SoundWave that plays an FMOD Studio Event and outputs silence until the event stops.
 * This avoids Parse-level retriggers and leverages the audio thread's generator lifecycle.
 */
UCLASS(BlueprintType, meta=(DisplayName="FMOD SoundWave (Event)"), editinlinenew)
class METASOUNDFMODPROXY_API USoundWaveFMODEvent : public USoundWaveProcedural
{
	GENERATED_BODY()

public:
	USoundWaveFMODEvent(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="FMOD")
	TObjectPtr<UFMODEvent> FMODEvent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FMOD")
	bool bAutoDestroy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FMOD")
	TMap<FName, float> EventParameters;

	// USoundWaveProcedural
	virtual int32 OnGeneratePCMAudio(TArray<uint8>& OutAudio, int32 NumSamples) override;
	virtual void OnBeginGenerate() override;
	virtual void OnEndGenerate() override;

#if WITH_EDITOR
	// Called by FMOD preview callback to signal STOPPED
	void MarkPreviewStopped();
#endif

private:
	FGuid InstanceId;
	TWeakObjectPtr<UFMODProxySubsystem> Subsystem;
	bool bStartRequested;
	bool bStartDispatched;
	bool bStarted;
	bool bFinished;

	// Runtime fallback polling (when no subsystem is available)
	void* RuntimeInstanceRaw = nullptr; // FMOD_STUDIO_EVENTINSTANCE* stored as void*
	bool bRuntimeObservedPlaying = false;

#if WITH_EDITOR
	void* PreviewInstanceRaw = nullptr; // FMOD_STUDIO_EVENTINSTANCE* stored as void*
	bool bHasObservedPlaying = false;
	bool bStopSignaled = false;
#endif
};


