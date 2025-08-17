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

	// Don't create wave instances - we handle FMOD playback directly
	// Instead, trigger FMOD event creation
	if (FMODEvent && AudioDevice)
	{
		// Create a unique ID for this sound instance
		uint32 SoundInstanceId = GetTypeHash(NodeWaveInstanceHash);
		
		// Check if we already have an active component for this instance
		if (ActiveFMODComponents.Contains(SoundInstanceId))
		{
			TWeakObjectPtr<UFMODAudioComponent> ExistingComponent = ActiveFMODComponents[SoundInstanceId];
			if (ExistingComponent.IsValid() && ExistingComponent->IsPlaying())
			{
				// Component is still playing, don't create another one
				return;
			}
			else
			{
				// Component finished or was destroyed, remove from tracking
				ActiveFMODComponents.Remove(SoundInstanceId);
			}
		}

		// Get world context for SoundNode - use GEngine approach
		UWorld* World = nullptr;
		if (GEngine)
		{
			// For SoundCues, we typically want the game world
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE)
				{
					World = Context.World();
					break;
				}
			}
		}

		if (World)
		{
			UFMODAudioComponent* FMODComponent = nullptr;

			// Try using the proxy subsystem first if available
			if (UGameInstance* GameInstance = World->GetGameInstance())
			{
				if (UFMODProxySubsystem* ProxySubsystem = GameInstance->GetSubsystem<UFMODProxySubsystem>())
				{
					// Use proxy subsystem to play the event
					FVector Location = ParseParams.Transform.GetLocation();
					
					if (FMODEvent)
					{
						FGuid InstanceGuid = ProxySubsystem->PlayEventAtLocationByAsset(World, FMODEvent, ParseParams.Transform, bAutoDestroy);
						
						// Set parameters if any
						for (const auto& Param : EventParameters)
						{
							ProxySubsystem->SetEventParameter(InstanceGuid, Param.Key.ToString(), Param.Value);
						}
						
						// Note: ProgrammerSound is handled by the FMOD component callback system
						// We can't set it directly via parameters in the proxy subsystem
					}
				}
			}

			// Fallback to direct FMOD Blueprint Statics
			if (!FMODComponent)
			{
				// Play at location (since we can't access the audio component directly from ActiveSound)
				FFMODEventInstance EventInstance = UFMODBlueprintStatics::PlayEventAtLocation(
					World,
					FMODEvent,
					ParseParams.Transform,
					true // bAutoPlay
				);
				
				// Note: We get an EventInstance struct, not a component
				// The FMOD plugin handles the component internally
				// For SoundCue integration, this basic playback should be sufficient
			}
		}
	}

	// Note: We don't add any WaveInstances because FMOD handles audio playback directly
	// The SoundCue system will see no wave instances and won't create traditional audio components
}





float USoundNodeFMOD::GetDuration()
{
	// FMOD events have dynamic durations, return indefinite
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
	// This will be called when any tracked FMOD component stops
	// The cleanup will happen in the next ParseNodes call
}



