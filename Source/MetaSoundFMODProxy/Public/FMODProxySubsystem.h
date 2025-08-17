#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Map.h"
#include "Misc/Guid.h"
#include "HAL/PlatformAtomics.h"
#include "Misc/ScopeLock.h"

// Use FMOD C and C++ headers. We include the C header for the callback C types.
#include <fmod.hpp>
#include <fmod_studio.h>

#include "FMODBlueprintStatics.h"
#include "FMODEvent.h"

#include "FMODProxySubsystem.generated.h"

// A tiny watch struct allocated per-played instance. It is referenced by FMOD user data.
struct FInstanceWatch
{
    std::atomic_bool bStopped{false};
    FGuid InstanceId;
    FInstanceWatch(const FGuid& InId) : InstanceId(InId) {}
};

UCLASS()
class METASOUNDFMODPROXY_API UFMODProxySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // Play an event by FMOD event asset (preferred) - returns an FGuid handle (empty if failed)
    UFUNCTION()
    FGuid PlayEventAtLocationByAsset(UWorld* World, UFMODEvent* Event, const FTransform& Transform, bool bAutoDestroy = true);

    // Play an event by path (e.g. "event:/SFX/Explosion") - useful if you prefer string identifiers
    UFUNCTION()
    FGuid PlayEventAtLocationByPath(UWorld* World, const FString& EventPath, const FTransform& Transform, bool bAutoDestroy = true);

    UFUNCTION()
    void StopEvent(const FGuid& InstanceId, EFMOD_STUDIO_STOP_MODE StopMode = EFMOD_STUDIO_STOP_MODE::ALLOWFADEOUT);

    UFUNCTION()
    void SetEventParameter(const FGuid& InstanceId, const FString& ParamName, float Value);

    UFUNCTION()
    bool IsPlaying(const FGuid& InstanceId) const;

private:
    // Map of instance id -> native FMOD instance pointer wrapper and watch struct
    struct FStoredInstance
    {
        FMOD::Studio::EventInstance* Instance = nullptr;
        TUniquePtr<FInstanceWatch> Watch;
    };

    mutable FCriticalSection MapLock;
    TMap<FGuid, FStoredInstance> InstanceMap;

    // Helper: called from GameThread after creating instance to set user data and callback
    void RegisterInstanceWatch(FMOD::Studio::EventInstance* NativeInstance, const FGuid& InstanceId);

    // Static FMOD callback (C-style signature). We avoid F_CALLBACK macro to prevent parser/calling-convention issues.
    static FMOD_RESULT StaticFmodCallback(FMOD_STUDIO_EVENT_CALLBACK_TYPE Type, FMOD_STUDIO_EVENTINSTANCE* Event, void* Parameters);
};
