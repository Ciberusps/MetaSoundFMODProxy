// Pavel Penkov 2025 All Rights Reserved.

#include "SoundWaveFMODEvent.h"
#include "FMODProxySubsystem.h"
#include "FMODBlueprintStatics.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

#if WITH_EDITOR
#include "Editor.h"
#include "FMODStudioModule.h"
#include <fmod_studio.h>
#endif

USoundWaveFMODEvent::USoundWaveFMODEvent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NumChannels = 1;
	SetSampleRate(48000);
	bLooping = false;
	bStreaming = false;
	Duration = INDEFINITELY_LOOPING_DURATION;
	bStartRequested = false;
	bStartDispatched = false;
	bStarted = false;
	bFinished = false;
	bAutoDestroy = true;
	RuntimeInstanceRaw = nullptr;
	bRuntimeObservedPlaying = false;
}

void USoundWaveFMODEvent::OnBeginGenerate()
{
	bStartRequested = true;
	bStartDispatched = false;
	bStarted = false;
	bFinished = false;
#if WITH_EDITOR
	PreviewInstanceRaw = nullptr;
	bHasObservedPlaying = false;
	bStopSignaled = false;
#endif
	RuntimeInstanceRaw = nullptr;
	bRuntimeObservedPlaying = false;
}

void USoundWaveFMODEvent::OnEndGenerate()
{
}

#if WITH_EDITOR
void USoundWaveFMODEvent::MarkPreviewStopped()
{
	bStopSignaled = true;
}

static FMOD_RESULT StaticPreviewStoppedCallback_SW(
	FMOD_STUDIO_EVENT_CALLBACK_TYPE Type,
	FMOD_STUDIO_EVENTINSTANCE* Event,
	void* /*Parameters*/)
{
	if (Type & FMOD_STUDIO_EVENT_CALLBACK_STOPPED)
	{
		void* User = nullptr;
		if (FMOD_Studio_EventInstance_GetUserData(Event, &User) == FMOD_OK && User)
		{
			USoundWaveFMODEvent* Self = reinterpret_cast<USoundWaveFMODEvent*>(User);
			Self->MarkPreviewStopped();
		}
	}
	return FMOD_OK;
}
#endif

int32 USoundWaveFMODEvent::OnGeneratePCMAudio(TArray<uint8>& OutAudio, int32 NumSamples)
{
	if (bFinished)
	{
		return 0;
	}

#if WITH_EDITOR
	if (GIsEditor && (!GEditor || !GEditor->PlayWorld))
	{
		if (!FMODEvent)
		{
			return 0;
		}
		// Start preview instance once
		if (bStartRequested && !bStarted && !bStartDispatched)
		{
			bStartRequested = false;
			bStartDispatched = true;
			bStarted = true;
			FMOD::Studio::EventInstance* PreviewInst = IFMODStudioModule::Get().CreateAuditioningInstance(FMODEvent);
			if (PreviewInst)
			{
				PreviewInst->start();
				PreviewInstanceRaw = reinterpret_cast<void*>(PreviewInst);
				FMOD_STUDIO_EVENTINSTANCE* CEvent = reinterpret_cast<FMOD_STUDIO_EVENTINSTANCE*>(PreviewInst);
				FMOD_Studio_EventInstance_SetUserData(CEvent, this);
				FMOD_Studio_EventInstance_SetCallback(CEvent, &StaticPreviewStoppedCallback_SW, FMOD_STUDIO_EVENT_CALLBACK_STOPPED);
			}
		}
		if (PreviewInstanceRaw)
		{
			FMOD_STUDIO_EVENTINSTANCE* CEvent = reinterpret_cast<FMOD_STUDIO_EVENTINSTANCE*>(PreviewInstanceRaw);
			FMOD_STUDIO_PLAYBACK_STATE State;
			if (FMOD_Studio_EventInstance_GetPlaybackState(CEvent, &State) == FMOD_OK)
			{
				if (State == FMOD_STUDIO_PLAYBACK_STARTING || State == FMOD_STUDIO_PLAYBACK_PLAYING)
				{
					bHasObservedPlaying = true;
				}
				if ((State == FMOD_STUDIO_PLAYBACK_STOPPED && bHasObservedPlaying) || bStopSignaled)
				{
					bFinished = true;
					return 0;
				}
			}
		}
		// Zero-fill provided buffer and return exactly its size
		const int32 BufferBytes = OutAudio.Num();
		if (BufferBytes > 0)
		{
			FMemory::Memset(OutAudio.GetData(), 0, BufferBytes);
			return BufferBytes;
		}
		return 0;
	}
#endif

	// Runtime path
	if (bStartRequested && !bStarted && !bStartDispatched && FMODEvent)
	{
		UWorld* World = GetWorld();
		if (!World && GEngine && GEngine->GetWorldContexts().Num() > 0)
		{
			World = GEngine->GetWorldContexts()[0].World();
		}
		if (World)
		{
			bStartRequested = false;
			bStartDispatched = true;
			bStarted = true;
			if (UGameInstance* GI = World->GetGameInstance())
			{
				Subsystem = GI->GetSubsystem<UFMODProxySubsystem>();
			}
			if (Subsystem.IsValid())
			{
				UWorld* WorldCtx = World;
				TWeakObjectPtr<USoundWaveFMODEvent> Self = this;
				AsyncTask(ENamedThreads::GameThread, [Self, WorldCtx]()
				{
					if (!Self.IsValid() || !Self->FMODEvent)
					{
						return;
					}
					FGuid NewId = Self->Subsystem.IsValid() ? Self->Subsystem->PlayEventAtLocationByAsset(WorldCtx, Self->FMODEvent, FTransform::Identity, Self->bAutoDestroy) : FGuid();
					for (const auto& KV : Self->EventParameters)
					{
						if (Self->Subsystem.IsValid())
						{
							Self->Subsystem->SetEventParameter(NewId, KV.Key.ToString(), KV.Value);
						}
					}
					Self->InstanceId = NewId;
				});
			}
			else
			{
				UWorld* WorldCtx = World;
				TWeakObjectPtr<USoundWaveFMODEvent> Self = this;
				AsyncTask(ENamedThreads::GameThread, [Self, WorldCtx]()
				{
					if (!Self.IsValid() || !Self->FMODEvent)
					{
						return;
					}
					FFMODEventInstance EI = UFMODBlueprintStatics::PlayEventAtLocation(WorldCtx, Self->FMODEvent, FTransform::Identity, true);
					for (const auto& KV : Self->EventParameters)
					{
						UFMODBlueprintStatics::EventInstanceSetParameter(EI, KV.Key, KV.Value);
					}
					Self->RuntimeInstanceRaw = EI.Instance;
				});
			}
		}
		// If no world yet, keep returning silence; we'll retry next buffer
	}

	bool bIsPlaying = true;
	if (Subsystem.IsValid() && InstanceId.IsValid())
	{
		bIsPlaying = Subsystem->IsPlaying(InstanceId);
	}
	else if (RuntimeInstanceRaw)
	{
		FMOD_STUDIO_EVENTINSTANCE* CEvent = reinterpret_cast<FMOD_STUDIO_EVENTINSTANCE*>(RuntimeInstanceRaw);
		FMOD_STUDIO_PLAYBACK_STATE State;
		if (FMOD_Studio_EventInstance_GetPlaybackState(CEvent, &State) == FMOD_OK)
		{
			if (State == FMOD_STUDIO_PLAYBACK_STARTING || State == FMOD_STUDIO_PLAYBACK_PLAYING)
			{
				bRuntimeObservedPlaying = true;
			}
			if (State == FMOD_STUDIO_PLAYBACK_STOPPED && bRuntimeObservedPlaying)
			{
				bFinished = true;
				return 0;
			}
		}
	}
	if (!bIsPlaying && bStarted)
	{
		bFinished = true;
		return 0;
	}

	const int32 BufferBytes = OutAudio.Num();
	if (BufferBytes > 0)
	{
		FMemory::Memset(OutAudio.GetData(), 0, BufferBytes);
		return BufferBytes;
	}
	return 0;
}


