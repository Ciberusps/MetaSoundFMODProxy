// Pavel Penkov 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundWaveProcedural.h"
#include "FMODWaitingWave.generated.h"

class UFMODProxySubsystem;
namespace FMOD { namespace Studio { class EventInstance; } }

/**
 * Procedural silent wave that keeps a SoundCue alive until an FMOD event finishes.
 */
UCLASS()
class METASOUNDFMODPROXY_API UFMODWaitingWave : public USoundWaveProcedural
{
	GENERATED_BODY()

public:
	UFMODWaitingWave(const FObjectInitializer& ObjectInitializer);

	/** Initialize with FMOD instance and subsystem used to poll IsPlaying */
	void InitWaiting(const FGuid& InInstanceId, UFMODProxySubsystem* InSubsystem);

	/** Update instance id later when it becomes available (game thread safe) */
	void SetInstanceId(const FGuid& InInstanceId);

	/** Initialize for editor auditioning with a native FMOD preview instance */
	void InitWaitingPreview(FMOD::Studio::EventInstance* InPreviewInstance);

	// USoundWaveProcedural interface
	virtual int32 OnGeneratePCMAudio(TArray<uint8>& OutAudio, int32 NumSamples) override;
	virtual void OnBeginGenerate() override;
	virtual void OnEndGenerate() override;

	/** Returns true if FMOD finished and no more audio should be generated */
	bool HasFinished() const;

private:
	FGuid InstanceId;
	TWeakObjectPtr<UFMODProxySubsystem> Subsystem;
	bool bFinished;
	FMOD::Studio::EventInstance* PreviewInstance = nullptr;
	/** While we are waiting for InstanceId/SubSystem or Editor preview instance to be assigned, keep generating silence */
	bool bPending = false;
};


