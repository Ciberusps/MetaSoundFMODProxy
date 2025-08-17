#include "FMODProxySubsystem.h"
#include "Engine/World.h"
#include "Async/Async.h"
#include "UObject/SoftObjectPath.h"

// Define the static callback (C-style types). Not using F_CALLBACK here — this avoids macro collisions with Unreal.
FMOD_RESULT UFMODProxySubsystem::StaticFmodCallback(FMOD_STUDIO_EVENT_CALLBACK_TYPE Type, FMOD_STUDIO_EVENTINSTANCE* Event, void* /*Parameters*/)
{
    if (Type & FMOD_STUDIO_EVENT_CALLBACK_STOPPED)
    {
        void* userData = nullptr;
        FMOD_RESULT r = FMOD_Studio_EventInstance_GetUserData(Event, &userData);
        if (r == FMOD_OK && userData)
        {
            FInstanceWatch* Watch = reinterpret_cast<FInstanceWatch*>(userData);
            if (Watch)
            {
                Watch->bStopped.store(true);
            }
        }
    }
    return FMOD_OK;
}

FGuid UFMODProxySubsystem::PlayEventAtLocationByAsset(UWorld* World, UFMODEvent* Event, const FTransform& Transform, bool bAutoDestroy)
{
    if (!World || !Event)
    {
        return FGuid();
    }

    // Use the FMOD UE BlueprintStatics to play an event (returns FFMODEventInstance)
    FFMODEventInstance UEInstance = UFMODBlueprintStatics::PlayEventAtLocation(World, Event, Transform, bAutoDestroy);

    // Try to extract the native pointer via the wrapper - this depends on FMOD UE internals. If unavailable,
    // you'd need to create the instance via the FMOD Studio API directly.
    FMOD::Studio::EventInstance* NativeInst = nullptr;

#if 0
    // If you know how to extract native pointer from FFMODEventInstance in your FMOD UE plugin version do that here.
    NativeInst = /* extract native pointer */ nullptr;
#endif

    FGuid Guid = FGuid::NewGuid();

    if (NativeInst)
    {
        RegisterInstanceWatch(NativeInst, Guid);

        FStoredInstance Stored;
        Stored.Instance = NativeInst;
        Stored.Watch = MakeUnique<FInstanceWatch>(Guid);

        FScopeLock Lock(&MapLock);
        InstanceMap.Add(Guid, MoveTemp(Stored));

        return Guid;
    }
    else
    {
        // Fallback: store a watch so IsPlaying can be polled if you implement polling logic.
        FStoredInstance Stored;
        Stored.Instance = nullptr;
        Stored.Watch = MakeUnique<FInstanceWatch>(Guid);
        FScopeLock Lock(&MapLock);
        InstanceMap.Add(Guid, MoveTemp(Stored));
        return Guid;
    }
}

FGuid UFMODProxySubsystem::PlayEventAtLocationByPath(UWorld* World, const FString& EventPath, const FTransform& Transform, bool bAutoDestroy)
{
    if (!World || EventPath.IsEmpty())
    {
        return FGuid();
    }

    // Try to load as UFMODEvent asset first
    FSoftObjectPath SoftPath(EventPath);
    UFMODEvent* EventAsset = Cast<UFMODEvent>(SoftPath.TryLoad());
    if (EventAsset)
    {
        return PlayEventAtLocationByAsset(World, EventAsset, Transform, bAutoDestroy);
    }

    // Best: create native instance via FMOD Studio API directly. To do that you need to get the FMOD::Studio::System*
    // from the FMOD UE plugin. Common patterns:
    //  - Call IFMODStudioModule::Get().GetStudioSystem() or similar if the plugin exposes it
    //  - Use FMOD::Studio::System::createSound/ getEvent by path via the Studio API
    // Below is a guarded example skeleton showing the approach; update according to your FMOD plugin API.

    FMOD::Studio::System* StudioSystem = nullptr;

    // TODO: adapt to your FMOD UE plugin: the actual accessor name may differ.
#if 0
    // Example: if your plugin exposes a function to fetch the studio system pointer
    StudioSystem = IFMODStudioModule::Get().GetStudioSystem();
#endif

    if (StudioSystem)
    {
        // Create event instance by path using Studio API
        FMOD::Studio::EventDescription* Desc = nullptr;
        FMOD_RESULT Res = StudioSystem->getEvent(TCHAR_TO_ANSI(*EventPath), &Desc);
        if (Res == FMOD_OK && Desc)
        {
            FMOD::Studio::EventInstance* Inst = nullptr;
            Res = Desc->createInstance(&Inst);
            if (Res == FMOD_OK && Inst)
            {
                // Start the instance
                Inst->start();

                // Register watch and store
                FGuid Guid = FGuid::NewGuid();

                RegisterInstanceWatch(Inst, Guid);

                FStoredInstance Stored;
                Stored.Instance = Inst;
                Stored.Watch = MakeUnique<FInstanceWatch>(Guid);

                FScopeLock Lock(&MapLock);
                InstanceMap.Add(Guid, MoveTemp(Stored));

                return Guid;
            }
        }
    }

    return FGuid();
}

void UFMODProxySubsystem::RegisterInstanceWatch(FMOD::Studio::EventInstance* NativeInstance, const FGuid& InstanceId)
{
    if (!NativeInstance)
        return;

    // allocate watch
    FInstanceWatch* Watch = new FInstanceWatch(InstanceId);

    // The FMOD C API functions expect FMOD_STUDIO_EVENTINSTANCE*. Convert safely.
    FMOD_STUDIO_EVENTINSTANCE* CEvent = reinterpret_cast<FMOD_STUDIO_EVENTINSTANCE*>(NativeInstance);

    // Set user data & callback via C API (avoids calling-convention macro issues)
    FMOD_Studio_EventInstance_SetUserData(CEvent, (void*)Watch);
    FMOD_Studio_EventInstance_SetCallback(CEvent, &UFMODProxySubsystem::StaticFmodCallback, FMOD_STUDIO_EVENT_CALLBACK_STOPPED);

    FStoredInstance Stored;
    Stored.Instance = NativeInstance;
    Stored.Watch.Reset(Watch);

    FScopeLock Lock(&MapLock);
    InstanceMap.Add(InstanceId, MoveTemp(Stored));
}

void UFMODProxySubsystem::StopEvent(const FGuid& InstanceId, EFMOD_STUDIO_STOP_MODE StopMode)
{
    FScopeLock Lock(&MapLock);
    if (FStoredInstance* Found = InstanceMap.Find(InstanceId))
    {
        if (Found->Instance)
        {
            FMOD_STUDIO_EVENTINSTANCE* CEvent = reinterpret_cast<FMOD_STUDIO_EVENTINSTANCE*>(Found->Instance);
            FMOD_Studio_EventInstance_Stop(CEvent, static_cast<FMOD_STUDIO_STOP_MODE>(StopMode));
        }
        InstanceMap.Remove(InstanceId);
    }
}

void UFMODProxySubsystem::SetEventParameter(const FGuid& InstanceId, const FString& ParamName, float Value)
{
    FScopeLock Lock(&MapLock);
    if (FStoredInstance* Found = InstanceMap.Find(InstanceId))
    {
        if (Found->Instance)
        {
            Found->Instance->setParameterByName(TCHAR_TO_ANSI(*ParamName), Value);
        }
    }
}

bool UFMODProxySubsystem::IsPlaying(const FGuid& InstanceId) const
{
    FScopeLock Lock(&MapLock);
    if (const FStoredInstance* Found = InstanceMap.Find(InstanceId))
    {
        if (Found->Watch)
        {
            return !Found->Watch->bStopped.load();
        }
    }
    return false;
}
