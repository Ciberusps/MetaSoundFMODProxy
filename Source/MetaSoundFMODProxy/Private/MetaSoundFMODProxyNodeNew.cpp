// Pavel Penkov 2025 All Rights Reserved.

#include "MetaSoundFMODProxyNodeNew.h"
#include "MetaSoundFMODProxyDataTypes.h"

#include "CoreMinimal.h"
#include "MetasoundBuilderInterface.h"
#include "MetasoundParamHelper.h"
#include "MetasoundTrigger.h"
#include "MetasoundNodeRegistrationMacro.h"
#include "FMODProxySubsystem.h"
#include "Async/Async.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "MetasoundDataReference.h"
#include "MetasoundDataReferenceCollection.h"
#include "MetasoundDataReferenceMacro.h"
#include "MetasoundVertexData.h"


#define LOCTEXT_NAMESPACE "MetaSoundFMODProxyNodeTest_LFSRNode"

// namespace Metasound
// {
//     namespace FMODProxyVertexNames
//     {
//         METASOUND_PARAM(InputPlay, "Play", "Trigger to start the FMOD event");
//         METASOUND_PARAM(InputStop, "Stop", "Trigger to stop the FMOD event");
//         METASOUND_PARAM(InputEventAsset, "Event Asset", "FMOD Event asset to play");
//         METASOUND_PARAM(InputParam1Name, "Param1 Name", "FMOD parameter name (optional)");
//         METASOUND_PARAM(InputParam1Value, "Param1 Value", "FMOD parameter value (optional)");
//         METASOUND_PARAM(InputParam2Name, "Param2 Name", "FMOD parameter name (optional)");
//         METASOUND_PARAM(InputParam2Value, "Param2 Value", "FMOD parameter value (optional)");
//         METASOUND_PARAM(InputProgrammerSound, "Programmer Sound", "Programmer Sound name (optional)" );
//         METASOUND_PARAM(OutputFinished, "Finished", "Emitted when playback finishes");
//         METASOUND_PARAM(OutputIsPlaying, "Is Playing", "True while the FMOD event is playing");
//     }
//     
//
//     //------------------------------------------------------------------------------
//
//     FMetaSoundFMODProxyNewOperator::FMetaSoundFMODProxyNewOperator(
//         const FBuildOperatorParams& InParams,
//         const FTriggerReadRef& InPlayTrigger,
//         const FTriggerReadRef& InStopTrigger,
//         const TDataReadReference<FFMODEventAsset>& InEventAsset,
//         const FStringReadRef& InParam1Name,
//         const FFloatReadRef& InParam1Value,
//         const FStringReadRef& InParam2Name,
//         const FFloatReadRef& InParam2Value,
//         const FStringReadRef& InProgrammerSoundName)
//         : PlayTrigger(InPlayTrigger)
//         , StopTrigger(InStopTrigger)
//         , EventAssetRef(InEventAsset)
//         , Param1Name(InParam1Name)
//         , Param1Value(InParam1Value)
//         , Param2Name(InParam2Name)
//         , Param2Value(InParam2Value)
//         , ProgrammerSoundName(InProgrammerSoundName)
//         , OutFinishedTrigger(FTriggerWriteRef::CreateNew(InParams.OperatorSettings))
//         , OutIsPlaying(TDataWriteReferenceFactory<bool>::CreateAny(InParams.OperatorSettings, false))
//     {
//     }
//     
//     //------------------------------------------------------------------------------
//
//     const FNodeClassMetadata& FMetaSoundFMODProxyNewOperator::GetNodeInfo()
//     {
//         auto InitNodeMetaData = []() -> FNodeClassMetadata {
//             FNodeClassMetadata NodeMetaData {
//                 .ClassName = { TEXT("FMOD"), TEXT("Proxy Player (New)"), TEXT("Audio") },
//                 .MajorVersion = 1,
//                 .MinorVersion = 0,
//                 .DisplayName = LOCTEXT("Metasound_FMODProxyNewDisplayName", "FMOD Proxy Player (New)"),
//                 .Description = LOCTEXT("Metasound_FMODProxyNewDescription", "Plays an FMOD event asset and emits Finished when the event stops. Optional programmer sound name and up to two parameters."),
//                 .Author = "MetaSoundFMODProxy",
//                 .PromptIfMissing = PluginNodeMissingPrompt,
//                 .DefaultInterface = GetVertexInterface(),
//                 .CategoryHierarchy = { LOCTEXT("Metasound_FMODNodeCategory", "Audio") }
//             };
//             
//             return NodeMetaData;
//         };
//
//         static const FNodeClassMetadata NodeMetaData = InitNodeMetaData();
//         return NodeMetaData;
//     }
//
//     //------------------------------------------------------------------------------
//
//     const FVertexInterface& FMetaSoundFMODProxyNewOperator::GetVertexInterface()
//     {
//         using namespace FMODProxyVertexNames;
//         
//         static const FVertexInterface VertexInterface(
//             FInputVertexInterface(
//                 TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputPlay)),
//                 TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputStop)),
//                 TInputDataVertex<FFMODEventAsset>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputEventAsset)),
//                 TInputDataVertex<FString>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputParam1Name)),
//                 TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputParam1Value), 0.0f),
//                 TInputDataVertex<FString>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputParam2Name)),
//                 TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputParam2Value), 0.0f),
//                 TInputDataVertex<FString>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputProgrammerSound))
//             ),
//             FOutputVertexInterface(
//                 TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputFinished)),
//                 TOutputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputIsPlaying))
//             )
//         );
//         
//         return VertexInterface;
//     }
//
//     TUniquePtr<IOperator> FMetaSoundFMODProxyNewOperator::CreateOperator(
//         const FBuildOperatorParams& InParams, FBuildResults& OutBuildResults)
//     {
//         using namespace FMODProxyVertexNames;
//         
//         const FInputVertexInterfaceData& InputData = InParams.InputData;
//
//         FTriggerReadRef Play = InputData.GetOrCreateDefaultDataReadReference<FTrigger>(METASOUND_GET_PARAM_NAME(InputPlay), InParams.OperatorSettings);
//         FTriggerReadRef Stop = InputData.GetOrCreateDefaultDataReadReference<FTrigger>(METASOUND_GET_PARAM_NAME(InputStop), InParams.OperatorSettings);
//         TDataReadReference<FFMODEventAsset> EventAsset = InputData.GetOrCreateDefaultDataReadReference<FFMODEventAsset>(METASOUND_GET_PARAM_NAME(InputEventAsset), InParams.OperatorSettings);
//         FStringReadRef Param1Name = InputData.GetOrCreateDefaultDataReadReference<FString>(METASOUND_GET_PARAM_NAME(InputParam1Name), InParams.OperatorSettings);
//         FFloatReadRef Param1Value = InputData.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputParam1Value), InParams.OperatorSettings);
//         FStringReadRef Param2Name = InputData.GetOrCreateDefaultDataReadReference<FString>(METASOUND_GET_PARAM_NAME(InputParam2Name), InParams.OperatorSettings);
//         FFloatReadRef Param2Value = InputData.GetOrCreateDefaultDataReadReference<float>(METASOUND_GET_PARAM_NAME(InputParam2Value), InParams.OperatorSettings);
//         FStringReadRef ProgrammerSound = InputData.GetOrCreateDefaultDataReadReference<FString>(METASOUND_GET_PARAM_NAME(InputProgrammerSound), InParams.OperatorSettings);
//
//         return MakeUnique<FMetaSoundFMODProxyNewOperator>(InParams, Play, Stop, EventAsset, Param1Name, Param1Value, Param2Name, Param2Value, ProgrammerSound);
// }
//
//     //------------------------------------------------------------------------------
//
//     void FMetaSoundFMODProxyNewOperator::BindInputs(
//         FInputVertexInterfaceData& InOutVertexInterfaceData)
//     {
//         using namespace FMODProxyVertexNames;
//         InOutVertexInterfaceData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputPlay), PlayTrigger);
//         InOutVertexInterfaceData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputStop), StopTrigger);
//         InOutVertexInterfaceData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputEventAsset), EventAssetRef);
//         InOutVertexInterfaceData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputParam1Name), Param1Name);
//         InOutVertexInterfaceData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputParam1Value), Param1Value);
//         InOutVertexInterfaceData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputParam2Name), Param2Name);
//         InOutVertexInterfaceData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputParam2Value), Param2Value);
//         InOutVertexInterfaceData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputProgrammerSound), ProgrammerSoundName);
//     }
//
//     //------------------------------------------------------------------------------
//
//     void FMetaSoundFMODProxyNewOperator::BindOutputs(
//         FOutputVertexInterfaceData& InOutVertexInterfaceData)
//     {
//         using namespace FMODProxyVertexNames;
//         InOutVertexInterfaceData.BindWriteVertex(METASOUND_GET_PARAM_NAME(OutputFinished), OutFinishedTrigger);
//         InOutVertexInterfaceData.BindWriteVertex(METASOUND_GET_PARAM_NAME(OutputIsPlaying), OutIsPlaying);
//     }
//
//     //------------------------------------------------------------------------------
//
//     void FMetaSoundFMODProxyNewOperator::Execute()
//     {
//         OutFinishedTrigger->AdvanceBlock();
//
//         PlayTrigger->ExecuteBlock(
//             [](int32, int32) {},
//             [this](int32 StartFrame, int32)
//             {
//                 TObjectPtr<UFMODEvent> EventAsset = EventAssetRef->Event;
//                 CurrentInstance = FGuid::NewGuid();
//                 *OutIsPlaying = true;
//
//                 const FString ProgrammerName = *ProgrammerSoundName;
//                 const FString P1Name = *Param1Name;
//                 const float P1Value = *Param1Value;
//                 const FString P2Name = *Param2Name;
//                 const float P2Value = *Param2Value;
//
//                 AsyncTask(ENamedThreads::GameThread, [EventAsset, ProgrammerName, P1Name, P1Value, P2Name, P2Value]()
//                 {
//                     if (GEngine)
//                     {
//                         UWorld* World = GEngine->GetWorldContexts().Num() ? GEngine->GetWorldContexts()[0].World() : nullptr;
//                         if (World)
//                         {
//                             if (UGameInstance* GI = World->GetGameInstance())
//                             {
//                                 if (UFMODProxySubsystem* Proxy = GI->GetSubsystem<UFMODProxySubsystem>())
//                                 {
//                                     // Play event by asset
//                                     const FGuid Guid = Proxy->PlayEventAtLocationByAsset(World, EventAsset, FTransform::Identity, true);
//                                     // Set optional parameters
//                                     if (!P1Name.IsEmpty()) { Proxy->SetEventParameter(Guid, P1Name, P1Value); }
//                                     if (!P2Name.IsEmpty()) { Proxy->SetEventParameter(Guid, P2Name, P2Value); }
//                                     // Programmer sound hook (if your FMOD setup uses a param or label for it)
//                                     if (!ProgrammerName.IsEmpty()) { Proxy->SetEventParameter(Guid, ProgrammerName, 1.0f); }
//                                 }
//                             }
//                         }
//                     }
//                 });
//             }
//         );
//
//         StopTrigger->ExecuteBlock(
//             [](int32, int32) {},
//             [this](int32 StartFrame, int32)
//             {
//                 const FGuid ToStop = CurrentInstance;
//                 if (ToStop.IsValid())
//                 {
//                     AsyncTask(ENamedThreads::GameThread, [ToStop]()
//                     {
//                         if (GEngine)
//                         {
//                             UWorld* World = GEngine->GetWorldContexts().Num() ? GEngine->GetWorldContexts()[0].World() : nullptr;
//                             if (World)
//                             {
//                                 if (UGameInstance* GI = World->GetGameInstance())
//                                 {
//                                     if (UFMODProxySubsystem* Proxy = GI->GetSubsystem<UFMODProxySubsystem>())
//                                     {
//                                         Proxy->StopEvent(ToStop, EFMOD_STUDIO_STOP_MODE::ALLOWFADEOUT);
//                                     }
//                                 }
//                             }
//                         }
//                     });
//                 }
//             }
//         );
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
//                 OutFinishedTrigger->TriggerFrame(0);
//                 *OutIsPlaying = false;
//                 CurrentInstance.Invalidate();
//             }
//         }
//
//     }
//
//     //------------------------------------------------------------------------------
//
//     void FMetaSoundFMODProxyNewOperator::Reset(const IOperator::FResetParams& InParams)
//     {
//         if (CurrentInstance.IsValid())
//         {
//             const FGuid ToStop = CurrentInstance;
//             AsyncTask(ENamedThreads::GameThread, [ToStop]()
//             {
//                 if (GEngine)
//                 {
//                     UWorld* World = GEngine->GetWorldContexts().Num() ? GEngine->GetWorldContexts()[0].World() : nullptr;
//                     if (World)
//                     {
//                         if (UGameInstance* GI = World->GetGameInstance())
//                         {
//                             if (UFMODProxySubsystem* Proxy = GI->GetSubsystem<UFMODProxySubsystem>())
//                             {
//                                 Proxy->StopEvent(ToStop, EFMOD_STUDIO_STOP_MODE::ALLOWFADEOUT);
//                             }
//                         }
//                     }
//                 }
//             });
//         }
//
//         CurrentInstance.Invalidate();
//         *OutIsPlaying = false;
//         OutFinishedTrigger->Reset();
//     }
//     
//     //------------------------------------------------------------------------------
//     
//     METASOUND_REGISTER_NODE(FMetaSoundFMODProxyNodeNew)
// }
//
#undef LOCTEXT_NAMESPACE
