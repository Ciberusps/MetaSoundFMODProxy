// Pavel Penkov 2025 All Rights Reserved.

#include "SoundNodeFMOD.h"
#include "ActiveSound.h"
#include "AudioDevice.h"
#include "Components/AudioComponent.h"
#include "FMODAudioComponent.h"
#include "FMODBlueprintStatics.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "FMODProxySubsystem.h"
#include "Sound/SoundBase.h"
#include "FMODWaitingWave.h"
#include "Async/Async.h"
#if WITH_EDITOR
#include "Editor.h"
#include "FMODStudioModule.h"
#endif



USoundNodeFMOD::USoundNodeFMOD()
{
	FMODEvent = nullptr;
	ProgrammerSoundName = "";
	bEnableTimelineCallbacks = false;
	bAutoDestroy = true;
}


void USoundNodeFMOD::ParseNodes(
	FAudioDevice* AudioDevice, 
	const UPTRINT NodeWaveInstanceHash, 
	FActiveSound& ActiveSound, 
	const FSoundParseParameters& ParseParams, 
	TArray<FWaveInstance*>& WaveInstances
)
{
	// Clean up any finished components first
	CleanupFinishedComponents();

	if (FMODEvent && AudioDevice)
	{
		uint32 SoundInstanceId = GetTypeHash(NodeWaveInstanceHash);
		if (ActiveFMODComponents.Contains(SoundInstanceId))
		{
			TWeakObjectPtr<UFMODAudioComponent> ExistingComponent = ActiveFMODComponents[SoundInstanceId];
			if (ExistingComponent.IsValid() && ExistingComponent->IsPlaying())
			{
				return;
			}
			else
			{
				ActiveFMODComponents.Remove(SoundInstanceId);
			}
		}

		#if WITH_EDITOR
		if (GIsEditor && (!GEditor || !GEditor->PlayWorld))
		{
			if (FMODEvent)
			{
				UFMODWaitingWave* WaitingWave = nullptr;
				if (TWeakObjectPtr<UFMODWaitingWave>* FoundWave = ActiveWaitingWaves.Find(NodeWaveInstanceHash))
				{
					if (FoundWave->IsValid())
					{
						WaitingWave = FoundWave->Get();
					}
					else
					{
						ActiveWaitingWaves.Remove(NodeWaveInstanceHash);
					}
				}
				bool bCreatedNow = false;
				if (!WaitingWave)
				{
					WaitingWave = NewObject<UFMODWaitingWave>(GetTransientPackage());
					if (WaitingWave)
					{
						ActiveWaitingWaves.Add(NodeWaveInstanceHash, WaitingWave);
						bCreatedNow = true;
					}
				}
				if (WaitingWave)
				{
					FSoundParseParameters UpdatedParams = ParseParams;
					UpdatedParams.bLooping = true;
					WaitingWave->Parse(AudioDevice, NodeWaveInstanceHash, ActiveSound, UpdatedParams, WaveInstances);
					if (bCreatedNow)
					{
						TWeakObjectPtr<UFMODWaitingWave> WeakWaiting = WaitingWave;
						UFMODEvent* EventCopy = FMODEvent;
						AsyncTask(ENamedThreads::GameThread, [WeakWaiting, EventCopy]()
						{
							if (!EventCopy)
							{
								return;
							}
							FMOD::Studio::EventInstance* PreviewInst = IFMODStudioModule::Get().CreateAuditioningInstance(EventCopy);
							if (PreviewInst)
							{
								PreviewInst->start();
								if (WeakWaiting.IsValid())
								{
									WeakWaiting->InitWaitingPreview(PreviewInst);
								}
							}
						});
					}
				}
			}
			return;
		}
		#endif

		UWorld* World = GWorld;

		if (World)
		{
			FGuid PlayedInstanceGuid;

			UFMODWaitingWave* WaitingWave = nullptr;
			if (TWeakObjectPtr<UFMODWaitingWave>* FoundWavePtr = ActiveWaitingWaves.Find(NodeWaveInstanceHash))
			{
				if (FoundWavePtr->IsValid())
				{
					WaitingWave = FoundWavePtr->Get();
				}
				else
				{
					ActiveWaitingWaves.Remove(NodeWaveInstanceHash);
				}
			}
			if (!WaitingWave)
			{
				WaitingWave = NewObject<UFMODWaitingWave>(GetTransientPackage());
				ActiveWaitingWaves.Add(NodeWaveInstanceHash, WaitingWave);
			}
			if (WaitingWave)
			{
				FSoundParseParameters UpdatedParams = ParseParams;
				UpdatedParams.bLooping = true;
				// If reused, it will continue generating; if new, will be initialized below when instance starts
				WaitingWave->Parse(AudioDevice, NodeWaveInstanceHash, ActiveSound, UpdatedParams, WaveInstances);
				TWeakObjectPtr<UFMODWaitingWave> WeakWaiting = WaitingWave;

				FTransform TransformCopy = ParseParams.Transform;
				UFMODEvent* EventCopy = FMODEvent;
				TMap<FName, float> ParamsCopy = EventParameters;
				bool bAutoDestroyCopy = bAutoDestroy;
				AsyncTask(ENamedThreads::GameThread, [World, WeakWaiting, TransformCopy, EventCopy, ParamsCopy, bAutoDestroyCopy]()
				{
					if (!EventCopy)
					{
						return;
					}
					FGuid InstanceGuid;
					bool bUsedProxy = false;
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
						}
					}
					if (WeakWaiting.IsValid())
					{
						if (bUsedProxy)
						{
							// Provide subsystem so the waiting wave can poll immediately
							UFMODProxySubsystem* ProxySubsystemLocal = nullptr;
							if (UGameInstance* InnerGI = World ? World->GetGameInstance() : nullptr)
							{
								ProxySubsystemLocal = InnerGI->GetSubsystem<UFMODProxySubsystem>();
							}
							WeakWaiting->InitWaiting(InstanceGuid, ProxySubsystemLocal);
						}
						else
						{
							// Fallback: use FMOD blueprint statics and bind native instance pointer
							UObject* WorldContext = World;
							FFMODEventInstance EventInstance = UFMODBlueprintStatics::PlayEventAtLocation(WorldContext, EventCopy, TransformCopy, true);
							for (const auto& KV : ParamsCopy)
							{
								UFMODBlueprintStatics::EventInstanceSetParameter(EventInstance, KV.Key, KV.Value);
							}
							// Bind to preview/native pointer to poll playback state
							WeakWaiting->InitWaitingPreview(EventInstance.Instance);
						}
					}
				});
			}
		}
	}
}




float USoundNodeFMOD::GetDuration()
{
	return INDEFINITELY_LOOPING_DURATION;
}


void USoundNodeFMOD::CleanupFinishedComponents()
{
	TArray<uint32> KeysToRemove;
	
	for (auto& ComponentPair : ActiveFMODComponents)
	{
		if (!ComponentPair.Value.IsValid() || !ComponentPair.Value->IsPlaying())
		{
			KeysToRemove.Add(ComponentPair.Key);
		}
	}
	
	for (uint32 Key : KeysToRemove)
	{
		ActiveFMODComponents.Remove(Key);
	}
}


void USoundNodeFMOD::OnFMODEventStopped()
{
}



