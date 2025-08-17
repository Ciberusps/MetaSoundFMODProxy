// Pavel Penkov 2025 All Rights Reserved.

#include "MetaSoundFMODProxyNodeNew.h"

#include "MetasoundBuilderInterface.h"
#include "MetasoundParamHelper.h"
#include "MetasoundTrigger.h"
#include "MetasoundNodeRegistrationMacro.h"
#include "FMODProxySubsystem.h"
#include "Async/Async.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

#define LOCTEXT_NAMESPACE "MetaSoundFMODProxyNodeTest_LFSRNode"

namespace Metasound
{
    namespace FMODProxyVertexNames
    {
        METASOUND_PARAM(InputPlay, "Play", "Trigger to start the FMOD event");
        METASOUND_PARAM(InputStop, "Stop", "Trigger to stop the FMOD event");
        METASOUND_PARAM(InputEventPath, "Event Path", "FMOD event path or soft object path to UFMODEvent");
        METASOUND_PARAM(OutputFinished, "Finished", "Emitted when playback finishes");
        METASOUND_PARAM(OutputIsPlaying, "Is Playing", "True while the FMOD event is playing");
    }

    //------------------------------------------------------------------------------

    FMetaSoundFMODProxyNewOperator::FMetaSoundFMODProxyNewOperator(
        const FBuildOperatorParams& InParams,
        const FTriggerReadRef& InPlayTrigger,
        const FTriggerReadRef& InStopTrigger,
        const FStringReadRef& InEventPath)
        : PlayTrigger(InPlayTrigger)
        , StopTrigger(InStopTrigger)
        , EventPathRef(InEventPath)
        , OutFinishedTrigger(FTriggerWriteRef::CreateNew(InParams.OperatorSettings))
        , OutIsPlaying(TDataWriteReferenceFactory<bool>::CreateAny(InParams.OperatorSettings, false))
    {
    }
    
    //------------------------------------------------------------------------------

    const FNodeClassMetadata& FMetaSoundFMODProxyNewOperator::GetNodeInfo()
    {
        auto InitNodeMetaData = []() -> FNodeClassMetadata {
            FNodeClassMetadata NodeMetaData {
                .ClassName = { TEXT("FMOD"), TEXT("Proxy Player (New)"), TEXT("Audio") },
                .MajorVersion = 1,
                .MinorVersion = 0,
                .DisplayName = LOCTEXT("Metasound_FMODProxyNewDisplayName", "FMOD Proxy Player (New)"),
                .Description = LOCTEXT("Metasound_FMODProxyNewDescription", "Plays an FMOD event and emits Finished when the event stops"),
                .Author = "MetaSoundFMODProxy",
                .PromptIfMissing = PluginNodeMissingPrompt,
                .DefaultInterface = GetVertexInterface(),
                .CategoryHierarchy = { LOCTEXT("Metasound_FMODNodeCategory", "Audio") }
            };
            
            return NodeMetaData;
        };

        static const FNodeClassMetadata NodeMetaData = InitNodeMetaData();
        return NodeMetaData;
    }

    //------------------------------------------------------------------------------

    const FVertexInterface& FMetaSoundFMODProxyNewOperator::GetVertexInterface()
    {
        using namespace FMODProxyVertexNames;
        
        static const FVertexInterface VertexInterface(
            FInputVertexInterface(
                TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputPlay)),
                TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputStop)),
                TInputDataVertex<FString>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputEventPath))
            ),
            FOutputVertexInterface(
                TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputFinished)),
                TOutputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputIsPlaying))
            )
        );
        
        return VertexInterface;
    }

    TUniquePtr<IOperator> FMetaSoundFMODProxyNewOperator::CreateOperator(
        const FBuildOperatorParams& InParams, FBuildResults& OutBuildResults)
    {
        using namespace FMODProxyVertexNames;
        
        const FInputVertexInterfaceData& InputData = InParams.InputData;

        FTriggerReadRef Play = InputData.GetOrCreateDefaultDataReadReference<FTrigger>(METASOUND_GET_PARAM_NAME(InputPlay), InParams.OperatorSettings);
        FTriggerReadRef Stop = InputData.GetOrCreateDefaultDataReadReference<FTrigger>(METASOUND_GET_PARAM_NAME(InputStop), InParams.OperatorSettings);
        FStringReadRef EventPath = InputData.GetOrCreateDefaultDataReadReference<FString>(METASOUND_GET_PARAM_NAME(InputEventPath), InParams.OperatorSettings);

        return MakeUnique<FMetaSoundFMODProxyNewOperator>(InParams, Play, Stop, EventPath);
}

    //------------------------------------------------------------------------------

    void FMetaSoundFMODProxyNewOperator::BindInputs(
        FInputVertexInterfaceData& InOutVertexInterfaceData)
    {
        using namespace FMODProxyVertexNames;
        InOutVertexInterfaceData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputPlay), PlayTrigger);
        InOutVertexInterfaceData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputStop), StopTrigger);
        InOutVertexInterfaceData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputEventPath), EventPathRef);
    }

    //------------------------------------------------------------------------------

    void FMetaSoundFMODProxyNewOperator::BindOutputs(
        FOutputVertexInterfaceData& InOutVertexInterfaceData)
    {
        using namespace FMODProxyVertexNames;
        InOutVertexInterfaceData.BindWriteVertex(METASOUND_GET_PARAM_NAME(OutputFinished), OutFinishedTrigger);
        InOutVertexInterfaceData.BindWriteVertex(METASOUND_GET_PARAM_NAME(OutputIsPlaying), OutIsPlaying);
    }

    //------------------------------------------------------------------------------

    void FMetaSoundFMODProxyNewOperator::Execute()
    {
        OutFinishedTrigger->AdvanceBlock();

        PlayTrigger->ExecuteBlock(
            [](int32, int32) {},
            [this](int32 StartFrame, int32)
            {
                const FString EventPath = *EventPathRef;
                CurrentInstance = FGuid::NewGuid();
                *OutIsPlaying = true;

                AsyncTask(ENamedThreads::GameThread, [EventPath]()
                {
                    if (GEngine)
                    {
                        UWorld* World = GEngine->GetWorldContexts().Num() ? GEngine->GetWorldContexts()[0].World() : nullptr;
                        if (World)
                        {
                            if (UGameInstance* GI = World->GetGameInstance())
                            {
                                if (UFMODProxySubsystem* Proxy = GI->GetSubsystem<UFMODProxySubsystem>())
                                {
                                    Proxy->PlayEventAtLocationByPath(World, EventPath, FTransform::Identity, true);
                                }
                            }
                        }
                    }
                });
            }
        );

        StopTrigger->ExecuteBlock(
            [](int32, int32) {},
            [this](int32 StartFrame, int32)
            {
                const FGuid ToStop = CurrentInstance;
                if (ToStop.IsValid())
                {
                    AsyncTask(ENamedThreads::GameThread, [ToStop]()
                    {
                        if (GEngine)
                        {
                            UWorld* World = GEngine->GetWorldContexts().Num() ? GEngine->GetWorldContexts()[0].World() : nullptr;
                            if (World)
                            {
                                if (UGameInstance* GI = World->GetGameInstance())
                                {
                                    if (UFMODProxySubsystem* Proxy = GI->GetSubsystem<UFMODProxySubsystem>())
                                    {
                                        Proxy->StopEvent(ToStop, EFMOD_STUDIO_STOP_MODE::ALLOWFADEOUT);
                                    }
                                }
                            }
                        }
                    });
                }
            }
        );

        if (CurrentInstance.IsValid())
        {
            bool bStillPlaying = false;
            if (GEngine)
            {
                UWorld* World = GEngine->GetWorldContexts().Num() ? GEngine->GetWorldContexts()[0].World() : nullptr;
                if (World)
                {
                    if (UGameInstance* GI = World->GetGameInstance())
                    {
                        if (UFMODProxySubsystem* Proxy = GI->GetSubsystem<UFMODProxySubsystem>())
                        {
                            bStillPlaying = Proxy->IsPlaying(CurrentInstance);
                        }
                    }
                }
            }

            if (!bStillPlaying)
            {
                OutFinishedTrigger->TriggerFrame(0);
                *OutIsPlaying = false;
                CurrentInstance.Invalidate();
            }
        }

    }

    //------------------------------------------------------------------------------

    void FMetaSoundFMODProxyNewOperator::Reset(const IOperator::FResetParams& InParams)
    {
        if (CurrentInstance.IsValid())
        {
            const FGuid ToStop = CurrentInstance;
            AsyncTask(ENamedThreads::GameThread, [ToStop]()
            {
                if (GEngine)
                {
                    UWorld* World = GEngine->GetWorldContexts().Num() ? GEngine->GetWorldContexts()[0].World() : nullptr;
                    if (World)
                    {
                        if (UGameInstance* GI = World->GetGameInstance())
                        {
                            if (UFMODProxySubsystem* Proxy = GI->GetSubsystem<UFMODProxySubsystem>())
                            {
                                Proxy->StopEvent(ToStop, EFMOD_STUDIO_STOP_MODE::ALLOWFADEOUT);
                            }
                        }
                    }
                }
            });
        }

        CurrentInstance.Invalidate();
        *OutIsPlaying = false;
        OutFinishedTrigger->Reset();
    }
    
    //------------------------------------------------------------------------------
    
    METASOUND_REGISTER_NODE(FMetaSoundFMODProxyNodeNew)
}

#undef LOCTEXT_NAMESPACE
