// Pavel Penkov 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundNode.h"
#include "FMODEvent.h"
#include "SoundNodeFMOD.generated.h"

class UFMODAudioComponent;
namespace FMOD { namespace Studio { class EventInstance; } }

/**
 * Sound node that plays FMOD events within SoundCues
 * Integrates FMOD Studio events into Unreal's traditional audio pipeline
 */
UCLASS(BlueprintType, hidecategories = Object, editinlinenew, MinimalAPI, meta = (DisplayName = "FMOD Event"))
class USoundNodeFMOD : public USoundNode
{
	GENERATED_BODY()

public:
	USoundNodeFMOD();

	/** The FMOD event to play */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FMOD")
	TObjectPtr<UFMODEvent> FMODEvent;

	/** Sound name used for programmer sound. Will look up the name in any loaded audio table. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FMOD")
	FString ProgrammerSoundName;

	/** Enable timeline callbacks for this sound */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FMOD")
	bool bEnableTimelineCallbacks;

	/** Auto destroy the audio component when the event finishes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FMOD")
	bool bAutoDestroy;

	/** FMOD event parameters */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FMOD")
	TMap<FName, float> EventParameters;

#if WITH_EDITORONLY_DATA
	/** When previewing in the SoundCue editor, force playback as 2D */
	UPROPERTY(EditAnywhere, Category = "FMOD|Preview")
	bool bPreviewAs2D = true;
#endif

	//~ Begin USoundNode Interface
	virtual void ParseNodes(FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances) override;
	virtual int32 GetMaxChildNodes() const override { return 0; }
	virtual float GetDuration() override;
	virtual bool IsPlayWhenSilent() const override { return true; }

	//~ End USoundNode Interface



private:
	/** Track active FMOD components spawned by this node */
	UPROPERTY(Transient)
	TMap<uint32, TWeakObjectPtr<UFMODAudioComponent>> ActiveFMODComponents;

	/** Clean up finished FMOD components */
	void CleanupFinishedComponents();

	/** Callback for when FMOD event stops */
	UFUNCTION()
	void OnFMODEventStopped();

#if WITH_EDITORONLY_DATA
	/** Editor preview instance to prevent constant retriggering in SoundCue editor */
	FMOD::Studio::EventInstance* PreviewInstance = nullptr;
#endif
};
