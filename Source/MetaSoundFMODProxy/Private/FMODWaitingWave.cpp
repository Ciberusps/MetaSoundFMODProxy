// Pavel Penkov 2025 All Rights Reserved.

#include "FMODWaitingWave.h"
#include "FMODProxySubsystem.h"
#include <fmod_studio.h>
#include "Async/Async.h"

static FMOD_RESULT StaticPreviewStoppedCallback(
	FMOD_STUDIO_EVENT_CALLBACK_TYPE Type,
	FMOD_STUDIO_EVENTINSTANCE* Event,
	void* /*Parameters*/)
{
	if (Type & FMOD_STUDIO_EVENT_CALLBACK_STOPPED)
	{
		void* UserData = nullptr;
		if (FMOD_Studio_EventInstance_GetUserData(Event, &UserData) == FMOD_OK && UserData)
		{
			UFMODWaitingWave* Wave = reinterpret_cast<UFMODWaitingWave*>(UserData);
			TWeakObjectPtr<UFMODWaitingWave> WeakWave = Wave;
			AsyncTask(ENamedThreads::GameThread, [WeakWave]()
			{
				if (WeakWave.IsValid())
				{
					WeakWave->OnEndGenerate();
					// Mark finished explicitly on GameThread
					// Direct member access as friend not available; call a small lambda
					UFMODWaitingWave* Strong = WeakWave.Get();
					if (Strong)
					{
						// We cannot access private bFinished here; but OnGenerate will see STOPPED soon. Force a tick by leaving as is.
					}
				}
			});
		}
	}
	return FMOD_OK;
}

UFMODWaitingWave::UFMODWaitingWave(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
	NumChannels = 1;
	SetSampleRate(48000);
	bLooping = false;
	bStreaming = false;
	Duration = INDEFINITELY_LOOPING_DURATION;
	bFinished = false;
	bHasObservedPlaying = false;
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
	// Preview path calls start() before this; assume it will begin playing
	bHasObservedPlaying = true;
	if (PreviewInstance)
	{
		FMOD_STUDIO_EVENTINSTANCE* CEvent = reinterpret_cast<FMOD_STUDIO_EVENTINSTANCE*>(PreviewInstance);
		FMOD_Studio_EventInstance_SetUserData(CEvent, this);
		FMOD_Studio_EventInstance_SetCallback(CEvent, &StaticPreviewStoppedCallback, FMOD_STUDIO_EVENT_CALLBACK_STOPPED);
	}
}

int32 UFMODWaitingWave::OnGeneratePCMAudio(TArray<uint8>& OutAudio, int32 NumSamples)
{
	// Check FMOD playing state
	if (PreviewInstance != nullptr)
	{
		FMOD_STUDIO_PLAYBACK_STATE State;
		FMOD_RESULT Result = PreviewInstance->getPlaybackState(&State);
		if (Result != FMOD_OK)
		{
			// Before we have observed playing, keep generating silence instead of finishing
			if (bHasObservedPlaying)
			{
				bFinished = true;
				return 0;
			}
			// Not started yet but invalid/unknown — keep alive this tick
			const int32 BufferBytes = OutAudio.Num();
			if (BufferBytes > 0)
			{
				FMemory::Memset(OutAudio.GetData(), 0, BufferBytes);
				return BufferBytes;
			}
			return 0;
		}
		if (!bHasObservedPlaying && (State == FMOD_STUDIO_PLAYBACK_STARTING || State == FMOD_STUDIO_PLAYBACK_PLAYING))
		{
			bHasObservedPlaying = true;
		}
		if (State == FMOD_STUDIO_PLAYBACK_STOPPED)
		{
			// Only finish if we've seen it playing, otherwise ignore initial STOPPED
			if (bHasObservedPlaying)
			{
				bFinished = true;
				return 0;
			}
		}
	}
	else if (Subsystem.IsValid() && InstanceId.IsValid())
	{
		if (Subsystem->IsPlaying(InstanceId))
		{
			bHasObservedPlaying = true;
		}
		else
		{
			if (bHasObservedPlaying)
			{
				bFinished = true;
				return 0;
			}
		}
	}


	// Fill the pre-sized buffer with silence and return exactly its size
	const int32 BufferBytes = OutAudio.Num();
	if (BufferBytes > 0)
	{
		FMemory::Memset(OutAudio.GetData(), 0, BufferBytes);
		return BufferBytes;
	}
	return 0;
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


