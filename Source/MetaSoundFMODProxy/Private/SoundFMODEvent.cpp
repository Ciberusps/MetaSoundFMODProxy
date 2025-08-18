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

#if WITH_EDITOR
	if (GIsEditor && (!GEditor || !GEditor->PlayWorld))
	{
		UFMODWaitingWave* WaitingWave = CreateWaitingWave(AudioDevice, NodeWaveInstanceHash, ActiveSound, ParseParams, WaveInstances);
		if (WaitingWave)
		{
			UFMODEvent* EventCopy = FMODEvent;
			AsyncTask(ENamedThreads::GameThread, [WaitingWave, EventCopy]()
			{
				if (!EventCopy)
				{
					return;
				}
				FMOD::Studio::EventInstance* PreviewInstance = IFMODStudioModule::Get().CreateAuditioningInstance(EventCopy);
				if (PreviewInstance)
				{
					PreviewInstance->start();
					WaitingWave->InitWaitingPreview(PreviewInstance);
				}
			});
		}
		return;
	}
#endif

	UWorld* World = GWorld;
	if (!World)
	{
		return;
	}

	UFMODWaitingWave* WaitingWave = CreateWaitingWave(AudioDevice, NodeWaveInstanceHash, ActiveSound, ParseParams, WaveInstances);
	if (!WaitingWave)
	{
		return;
	}

	FTransform TransformCopy = ParseParams.Transform;
	UFMODEvent* EventCopy = FMODEvent;
	TMap<FName, float> ParamsCopy = EventParameters;
	bool bAutoDestroyCopy = bAutoDestroy;

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


