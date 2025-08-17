// Pavel Penkov 2025 All Rights Reserved.

#include "FMODWaitingWave.h"
#include "FMODProxySubsystem.h"

UFMODWaitingWave::UFMODWaitingWave(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
	NumChannels = 1;
	SetSampleRate(48000);
	bLooping = false;
	bStreaming = false;
	Duration = INDEFINITELY_LOOPING_DURATION;
	bFinished = false;
}

void UFMODWaitingWave::InitWaiting(const FGuid& InInstanceId, UFMODProxySubsystem* InSubsystem)
{
	InstanceId = InInstanceId;
	Subsystem = InSubsystem;
	bFinished = false;
}

void UFMODWaitingWave::SetInstanceId(const FGuid& InInstanceId)
{
	InstanceId = InInstanceId;
}

void UFMODWaitingWave::InitWaitingPreview(FMOD::Studio::EventInstance* InPreviewInstance)
{
	PreviewInstance = InPreviewInstance;
	bFinished = false;
}

int32 UFMODWaitingWave::OnGeneratePCMAudio(TArray<uint8>& OutAudio, int32 NumSamples)
{
	// Check FMOD playing state
	if (PreviewInstance != nullptr)
	{
		FMOD_STUDIO_PLAYBACK_STATE State;
		if (PreviewInstance->getPlaybackState(&State) == FMOD_OK)
		{
			if (State == FMOD_STUDIO_PLAYBACK_STOPPED)
			{
				bFinished = true;
				return 0;
			}
		}
	}
	else if (Subsystem.IsValid())
	{
		if (!Subsystem->IsPlaying(InstanceId))
		{
			bFinished = true;
			return 0;
		}
	}
	else
	{
		bFinished = true;
		return 0;
	}

	// Zero-fill the provided buffer up to the expected size without changing its length
	const int32 Channels = FMath::Max(1, NumChannels);
	const int32 BytesPerSample = sizeof(int16);
	const int32 MaxExpectedBytes = NumSamples * Channels * BytesPerSample;
	const int32 BufferBytes = OutAudio.Num();
	const int32 BytesToWrite = FMath::Min(MaxExpectedBytes, BufferBytes);
	if (BytesToWrite > 0)
	{
		FMemory::Memset(OutAudio.GetData(), 0, BytesToWrite);
	}
	return BytesToWrite;
}

void UFMODWaitingWave::OnBeginGenerate()
{
}

void UFMODWaitingWave::OnEndGenerate()
{
}

bool UFMODWaitingWave::HasFinished() const
{
	return bFinished;
}


