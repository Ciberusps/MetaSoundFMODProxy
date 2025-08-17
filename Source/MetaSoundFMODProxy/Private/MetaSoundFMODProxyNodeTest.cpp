// Pavel Penkov 2025 All Rights Reserved.

#include "MetaSoundFMODProxyNodeTest.h"

#include "MetasoundBuilderInterface.h"
#include "MetasoundParamHelper.h"
#include "MetasoundTrigger.h"
#include "Kismet/KismetMathLibrary.h"

#define LOCTEXT_NAMESPACE "MetaSoundFMODProxyNodeTest_LFSRNode"

namespace Metasound
{
    namespace LFSRVertexNames
    {
        METASOUND_PARAM(InputTriggerNextValue, "Next", "Trigger next value.");
        METASOUND_PARAM(InputNumBits, "Num Bits", "Number of Bits the LFSR operates on between [2-12] (clamped internally).");
        METASOUND_PARAM(OutputTriggerOnNext, "On Trigger", "Triggered when the input is triggered.");
        METASOUND_PARAM(OutputValue, "Output Value", "The output value.");
    }

    //------------------------------------------------------------------------------

    FMetaSoundFMODProxyOperator::FMetaSoundFMODProxyOperator(const FBuildOperatorParams& InParams,
        TDataReadReference<FTrigger> InputTriggerNext,
        TDataReadReference<int32> InNumBits)
        : TriggerNext(InputTriggerNext)
        , NumBits(InNumBits)
        , TriggerOnNext(FTriggerWriteRef::CreateNew(InParams.OperatorSettings))
        , OutValue(TDataWriteReferenceFactory<int32>::CreateAny(InParams.OperatorSettings))
    {
        Reset();
    }
    
    //------------------------------------------------------------------------------

    const FNodeClassMetadata& FMetaSoundFMODProxyOperator::GetNodeInfo()
    {
        auto InitNodeMetaData = []() -> FNodeClassMetadata {
            FNodeClassMetadata NodeMetaData {
                .ClassName = { TEXT("UE"), TEXT("LFSR FMOD TEST (Random)"), TEXT("") },
                .MajorVersion = 1,
                .MinorVersion = 0,
                .DisplayName = LOCTEXT("Metasound_LFSRDisplayName", "FMOD TEST LFSR"),
                .Description = LOCTEXT("Metasound_LFSRDescription", "Maximum Length Linear Feedback Shift Register"),
                .Author = "Michael Hartung <https://hartung.studio>",
                .PromptIfMissing = PluginNodeMissingPrompt,
                .DefaultInterface = GetVertexInterface(),
                .CategoryHierarchy = { LOCTEXT("Metasound_LFSRNodeCategory", "Random") }
            };
            
            return NodeMetaData;
        };

        static const FNodeClassMetadata NodeMetaData = InitNodeMetaData();
        return NodeMetaData;
    }

    //------------------------------------------------------------------------------

    const FVertexInterface& FMetaSoundFMODProxyOperator::GetVertexInterface()
    {
        using namespace LFSRVertexNames;;
        
        static const FVertexInterface VertexInterface(
            FInputVertexInterface(
                TInputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputTriggerNextValue)),
                TInputDataVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputNumBits), 3)
            ),
            FOutputVertexInterface(
                TOutputDataVertex<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputTriggerOnNext)),
                TOutputDataVertex<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputValue))
            )
        );
        
        return VertexInterface;
    }

    TUniquePtr<IOperator> FMetaSoundFMODProxyOperator::CreateOperator(
        const FBuildOperatorParams& InParams, FBuildResults& OutBuildResults)
    {
        using namespace LFSRVertexNames;
        
        const FInputVertexInterfaceData& InputData = InParams.InputData;

        FTriggerReadRef InTriggerNextValue = InputData.GetOrCreateDefaultDataReadReference<FTrigger>(METASOUND_GET_PARAM_NAME(InputTriggerNextValue), InParams.OperatorSettings);
        FInt32ReadRef MaxValue = InputData.GetOrCreateDefaultDataReadReference<int32>(METASOUND_GET_PARAM_NAME(InputNumBits), InParams.OperatorSettings);

        return MakeUnique<FMetaSoundFMODProxyOperator>(InParams, InTriggerNextValue, MaxValue);
}

    //------------------------------------------------------------------------------

    void FMetaSoundFMODProxyOperator::BindInputs(
        FInputVertexInterfaceData& InOutVertexInterfaceData)
    {
        using namespace LFSRVertexNames;
        InOutVertexInterfaceData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputTriggerNextValue), TriggerNext);
        InOutVertexInterfaceData.BindReadVertex(METASOUND_GET_PARAM_NAME(InputNumBits), NumBits);
    }

    //------------------------------------------------------------------------------

    void FMetaSoundFMODProxyOperator::BindOutputs(
        FOutputVertexInterfaceData& InOutVertexInterfaceData)
    {
        using namespace LFSRVertexNames;
        InOutVertexInterfaceData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputTriggerOnNext), TriggerNext);
        InOutVertexInterfaceData.BindReadVertex(METASOUND_GET_PARAM_NAME(OutputValue), OutValue);
    }

    //------------------------------------------------------------------------------

    void FMetaSoundFMODProxyOperator::Execute()
    {
        TriggerOnNext->AdvanceBlock();

        TriggerNext->ExecuteBlock(
            [&](int32 StartFrame, int32 EndFrame)
            {
            },
            [this](int32 StartFrame, int32 EndFrame)
            {
                int32 NumBitsClamped = UKismetMathLibrary::Clamp(*NumBits, 2, 12);
                *OutValue = LFSR.GetNextValueWithLast(NumBitsClamped);
                TriggerOnNext->TriggerFrame(StartFrame);
            }
        );

    }

    //------------------------------------------------------------------------------

    void FMetaSoundFMODProxyOperator::Reset()
    {
        TriggerOnNext->Reset();
    }
    
    //------------------------------------------------------------------------------
    
    METASOUND_REGISTER_NODE(FMetaSoundFMODProxyNode)
}

#undef LOCTEXT_NAMESPACE
