// Pavel Penkov 2025 All Rights Reserved.

#include "SoundFMODEvent.h"

#include "ActiveSound.h"
#include "AudioDevice.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

#include "FMODProxySubsystem.h"
#include "FMODWaitingWave.h"
#include "FMODBlueprintStatics.h"

#if WITH_EDITOR
#include "Editor.h"
#include "FMODStudioModule.h"
#endif

USoundFMODEvent::USoundFMODEvent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoDestroy = true;
#if WITH_EDITORONLY_DATA
	bPreviewAs2D = true;
#endif
}

UFMODWaitingWave* USoundFMODEvent::CreateWaitingWave(
	FAudioDevice* AudioDevice,
	const UPTRINT NodeWaveInstanceHash,
	FActiveSound& ActiveSound,
	const FSoundParseParameters& ParseParams,
	TArray<FWaveInstance*>& WaveInstances
) const
{
	UFMODWaitingWave* WaitingWave = NewObject<UFMODWaitingWave>(GetTransientPackage());
	if (WaitingWave)
	{
		FSoundParseParameters UpdatedParams = ParseParams;
		UpdatedParams.bLooping = true;
		WaitingWave->InitWaiting(FGuid(), nullptr);
		WaitingWave->Parse(AudioDevice, NodeWaveInstanceHash, ActiveSound, UpdatedParams, WaveInstances);
	}
	return WaitingWave;
}

void USoundFMODEvent::Parse(
	FAudioDevice* AudioDevice,
	const UPTRINT NodeWaveInstanceHash,
	FActiveSound& ActiveSound,
	const FSoundParseParameters& ParseParams,
	TArray<FWaveInstance*>& WaveInstances
)
{
	if (!FMODEvent || !AudioDevice)
	{
		return;
	}

	// Do not clean finished waves here; avoid immediate retrigger within same play

#if WITH_EDITOR
	if (GIsEditor && (!GEditor || !GEditor->PlayWorld))
	{
		// Use a stable non-zero key for preview caching
		uint32 Key = static_cast<uint32>(NodeWaveInstanceHash);
		if (Key == 0)
		{
			Key = 1u;
		}
		UFMODWaitingWave* WaitingWave = nullptr;
		if (TWeakObjectPtr<UFMODWaitingWave>* FoundPtr = ActiveWaitingWaves.Find(Key))
		{
			if (FoundPtr->IsValid())
			{
				WaitingWave = FoundPtr->Get();
			}
			else
			{
				ActiveWaitingWaves.Remove(Key);
			}
		}
		bool bCreatedNow = false;
		if (!WaitingWave)
		{
			WaitingWave = CreateWaitingWave(AudioDevice, Key, ActiveSound, ParseParams, WaveInstances);
			if (WaitingWave)
			{
				ActiveWaitingWaves.Add(Key, WaitingWave);
				bCreatedNow = true;
			}
		}
		else
		{
			FSoundParseParameters UpdatedParams = ParseParams;
			UpdatedParams.bLooping = true;
			WaitingWave->Parse(AudioDevice, Key, ActiveSound, UpdatedParams, WaveInstances);
		}
		if (!WaitingWave)
		{
			return;
		}
		if (WaitingWave->HasFinished())
		{
			// Do not remove here; keep the weak entry to block this tick and let GC invalidate it
			return;
		}
		if (bCreatedNow)
		{
			UFMODEvent* EventCopy = FMODEvent;
			TWeakObjectPtr<UFMODWaitingWave> WeakWaiting = WaitingWave;
			AsyncTask(ENamedThreads::GameThread, [WeakWaiting, EventCopy]()
			{
				if (!EventCopy)
				{
					return;
				}
				FMOD::Studio::EventInstance* PreviewInstance = IFMODStudioModule::Get().CreateAuditioningInstance(EventCopy);
				if (PreviewInstance)
				{
					PreviewInstance->start();
					if (WeakWaiting.IsValid())
					{
						WeakWaiting->InitWaitingPreview(PreviewInstance);
					}
				}
			});
		}
		return;
	}
#endif

	UWorld* World = GWorld;
	if (!World)
	{
		if (GEngine && GEngine->GetWorldContexts().Num() > 0)
		{
			World = GEngine->GetWorldContexts()[0].World();
		}
		if (!World)
		{
			return;
		}
	}

	UFMODWaitingWave* WaitingWave = nullptr;
	if (TWeakObjectPtr<UFMODWaitingWave>* FoundPtr2 = ActiveWaitingWaves.Find(NodeWaveInstanceHash))
	{
		if (FoundPtr2->IsValid())
		{
			WaitingWave = FoundPtr2->Get();
		}
		else
		{
			ActiveWaitingWaves.Remove(NodeWaveInstanceHash);
		}
	}
	bool bCreatedNowRuntime = false;
	if (!WaitingWave)
	{
		WaitingWave = CreateWaitingWave(AudioDevice, NodeWaveInstanceHash, ActiveSound, ParseParams, WaveInstances);
		if (WaitingWave)
		{
			ActiveWaitingWaves.Add(NodeWaveInstanceHash, WaitingWave);
			bCreatedNowRuntime = true;
		}
	}
	else
	{
		FSoundParseParameters UpdatedParams = ParseParams;
		UpdatedParams.bLooping = true;
		WaitingWave->Parse(AudioDevice, NodeWaveInstanceHash, ActiveSound, UpdatedParams, WaveInstances);
	}
	if (!WaitingWave)
	{
		return;
	}
	if (WaitingWave->HasFinished())
	{
		// Runtime: allow GC to clear and block this tick
		return;
	}

	FTransform TransformCopy = ParseParams.Transform;
	UFMODEvent* EventCopy = FMODEvent;
	TMap<FName, float> ParamsCopy = EventParameters;
	bool bAutoDestroyCopy = bAutoDestroy;

	if (bCreatedNowRuntime)
	{
		AsyncTask(ENamedThreads::GameThread, [World, WaitingWave, TransformCopy, EventCopy, ParamsCopy, bAutoDestroyCopy]()
		{
			if (!EventCopy)
			{
				return;
			}
			FGuid InstanceGuid;
			bool bUsedProxy = false;
			UFMODProxySubsystem* ProxySubsystemLocal = nullptr;
			if (UGameInstance* GI = World ? World->GetGameInstance() : nullptr)
			{
				if (UFMODProxySubsystem* ProxySubsystem = GI->GetSubsystem<UFMODProxySubsystem>())
				{
					InstanceGuid = ProxySubsystem->PlayEventAtLocationByAsset(World, EventCopy, TransformCopy, bAutoDestroyCopy);
					for (const auto& KV : ParamsCopy)
					{
						ProxySubsystem->SetEventParameter(InstanceGuid, KV.Key.ToString(), KV.Value);
					}
					bUsedProxy = true;
					ProxySubsystemLocal = ProxySubsystem;
				}
			}

			if (bUsedProxy)
			{
				WaitingWave->InitWaiting(InstanceGuid, ProxySubsystemLocal);
			}
			else
			{
				// Fallback: use FMOD blueprint statics
				UObject* WorldContext = World;
				FFMODEventInstance EventInstance = UFMODBlueprintStatics::PlayEventAtLocation(WorldContext, EventCopy, TransformCopy, true);
				for (const auto& KV : ParamsCopy)
				{
					UFMODBlueprintStatics::EventInstanceSetParameter(EventInstance, KV.Key, KV.Value);
				}
				WaitingWave->InitWaitingPreview(EventInstance.Instance);
			}
		});
	}
}


void USoundFMODEvent::CleanupFinishedWaitingWaves()
{
	TArray<uint32> KeysToRemove;
	for (auto& Pair : ActiveWaitingWaves)
	{
		if (!Pair.Value.IsValid())
		{
			KeysToRemove.Add(Pair.Key);
			continue;
		}
		UFMODWaitingWave* Wave = Pair.Value.Get();
		if (Wave && Wave->HasFinished())
		{
			KeysToRemove.Add(Pair.Key);
		}
	}
	for (uint32 Key : KeysToRemove)
	{
		ActiveWaitingWaves.Remove(Key);
	}
}

