// Pavel Penkov 2025 All Rights Reserved.

#include "MetaSoundFMODProxyNode.h"

#include "MetasoundFacade.h"
#include "MetasoundExecutableOperator.h"
#include "MetasoundBuilderInterface.h"
#include "MetasoundParamHelper.h"
#include "MetasoundPrimitives.h"
#include "MetasoundTrigger.h"
#include "MetasoundVertex.h"
#include "MetasoundDataReference.h"
#include "MetasoundFrontendRegistries.h"
#include "MetasoundNodeRegistrationMacro.h"

#include "Async/Async.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
//
// using namespace Metasound;
//
// namespace Metasound
// {
// namespace FMODProxy
// {
// METASOUND_PARAM(InPlay, "Play", "Trigger to start the FMOD event");
// METASOUND_PARAM(InStop, "Stop", "Trigger to stop the FMOD event");
// METASOUND_PARAM(InEventPath, "Event Path", "FMOD event path or soft object path to UFMODEvent");
// METASOUND_PARAM(OutFinished, "Finished", "Emitted when playback finishes");
// METASOUND_PARAM(OutIsPlaying, "Is Playing", "True while the FMOD event is playing");
//
// class FFMODProxyOperator : public TExecutableOperator<FFMODProxyOperator>
// {
// public:
//     FFMODProxyOperator(const FCreateOperatorParams& InParams)
//     {
//         const FDataReferenceCollection& Inputs = InParams.InputData;
//         const FDataReferenceCollection& Outputs = InParams.OutputData;
//
//         PlayTrigger = Inputs.GetDataReadReferenceOrConstruct<FTrigger>(METASOUND_GET_PARAM_NAME(InPlay), InParams.OperatorSettings);
//         StopTrigger = Inputs.GetDataReadReferenceOrConstruct<FTrigger>(METASOUND_GET_PARAM_NAME(InStop), InParams.OperatorSettings);
//         EventPathRef = Inputs.GetDataReadReferenceOrConstruct<FString>(METASOUND_GET_PARAM_NAME(InEventPath), InParams.OperatorSettings, FString());
//
//         OutFinishedTrigger = Outputs.GetDataWriteReferenceOrConstruct<FTrigger>(METASOUND_GET_PARAM_NAME(OutFinished), InParams.OperatorSettings);
//         OutIsPlaying = Outputs.GetDataWriteReferenceOrConstruct<bool>(METASOUND_GET_PARAM_NAME(OutIsPlaying), InParams.OperatorSettings, false);
//     }
//
//     void Execute()
//     {
//         if (PlayTrigger->PopTrigger())
//         {
//             const FString EventPath = *EventPathRef;
//             CurrentInstance = FGuid::NewGuid();
//             OutIsPlaying->Set(true);
//
//             AsyncTask(ENamedThreads::GameThread, [EventPath]() {
//                 if (GEngine)
//                 {
//                     UWorld* World = GEngine->GetWorldContexts().Num() ? GEngine->GetWorldContexts()[0].World() : nullptr;
//                     if (World)
//                     {
//                         if (UGameInstance* GI = World->GetGameInstance())
//                         {
//                             if (UFMODProxySubsystem* Proxy = GI->GetSubsystem<UFMODProxySubsystem>())
//                             {
//                                 Proxy->PlayEventAtLocationByPath(World, EventPath, FTransform::Identity, true);
//                             }
//                         }
//                     }
//                 }
//             });
//         }
//
//         if (StopTrigger->PopTrigger())
//         {
//             const FGuid ToStop = CurrentInstance;
//             if (ToStop.IsValid())
//             {
//                 AsyncTask(ENamedThreads::GameThread, [ToStop]() {
//                     if (GEngine)
//                     {
//                         UWorld* World = GEngine->GetWorldContexts().Num() ? GEngine->GetWorldContexts()[0].World() : nullptr;
//                         if (World)
//                         {
//                             if (UGameInstance* GI = World->GetGameInstance())
//                             {
//                                 if (UFMODProxySubsystem* Proxy = GI->GetSubsystem<UFMODProxySubsystem>())
//                                 {
//                                     Proxy->StopEvent(ToStop, EFMOD_STUDIO_STOP_MODE::ALLOWFADEOUT);
//                                 }
//                             }
//                         }
//                     }
//                 });
//             }
//         }
//
//         if (CurrentInstance.IsValid())
//         {
//             bool bStillPlaying = false;
//             if (GEngine)
//             {
//                 UWorld* World = GEngine->GetWorldContexts().Num() ? GEngine->GetWorldContexts()[0].World() : nullptr;
//                 if (World)
//                 {
//                     if (UGameInstance* GI = World->GetGameInstance())
//                     {
//                         if (UFMODProxySubsystem* Proxy = GI->GetSubsystem<UFMODProxySubsystem>())
//                         {
//                             bStillPlaying = Proxy->IsPlaying(CurrentInstance);
//                         }
//                     }
//                 }
//             }
//
//             if (!bStillPlaying)
//             {
//                 OutFinishedTrigger->PushTrigger();
//                 OutIsPlaying->Set(false);
//                 CurrentInstance.Invalidate();
//             }
//         }
//     }
//
//     static const FVertexInterface& GetVertexInterface()
//     {
//         static const FVertexInterface Interface(
//             FInputVertexInterface(
//                 TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME(InPlay), METASOUND_GET_PARAM_DESCRIPTION(InPlay)),
//                 TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME(InStop), METASOUND_GET_PARAM_DESCRIPTION(InStop)),
//                 TInputDataVertex<FString>(METASOUND_GET_PARAM_NAME(InEventPath), METASOUND_GET_PARAM_DESCRIPTION(InEventPath))
//             ),
//             FOutputVertexInterface(
//                 TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME(OutFinished), METASOUND_GET_PARAM_DESCRIPTION(OutFinished)),
//                 TOutputDataVertex<bool>(METASOUND_GET_PARAM_NAME(OutIsPlaying), METASOUND_GET_PARAM_DESCRIPTION(OutIsPlaying))
//             )
//         );
//         return Interface;
//     }
//
//     static const FNodeClassMetadata& GetNodeInfo()
//     {
//         static const FNodeClassMetadata Info = []() -> FNodeClassMetadata
//         {
//             FNodeClassMetadata Metadata;
//             Metadata.ClassName = FNodeClassName{TEXT("FMOD"), TEXT("ProxyPlayer"), TEXT("Audio")};
//             Metadata.MajorVersion = 1;
//             Metadata.MinorVersion = 0;
//             Metadata.DisplayName = METASOUND_LOCTEXT("FMODProxyNodeDisplayName", "FMOD Proxy Player");
//             Metadata.Description = METASOUND_LOCTEXT("FMODProxyNodeDesc", "Plays an FMOD event and emits Finished when the event stops");
//             Metadata.Author = TEXT("MetaSoundFMODProxy");
//             Metadata.DefaultInterface = GetVertexInterface();
//             return Metadata;
//         }();
//         return Info;
//     }
//
//     static FOperatorFactorySharedRef CreateOperatorFactory()
//     {
//         return MakeOperatorFactoryRef<FFMODProxyOperator>();
//     }
//
// private:
//     FTriggerReadRef PlayTrigger;
//     FTriggerReadRef StopTrigger;
//     FStringReadRef EventPathRef;
//
//     FTriggerWriteRef OutFinishedTrigger;
//     FBoolWriteRef OutIsPlaying;
//
//     FGuid CurrentInstance;
// };
//
// class FFMODProxyNode : public FNodeFacade
// {
// public:
//     FFMODProxyNode(const FNodeInitData& Init)
//         : FNodeFacade(FFMODProxyOperator::GetNodeInfo(), FFMODProxyOperator::CreateOperatorFactory())
//     {
//     }
// };
//
// } // namespace FMODProxy
// } // namespace Metasound
//
// REGISTER_METASOUND_NODE(Metasound::FMODProxy::FFMODProxyNode)