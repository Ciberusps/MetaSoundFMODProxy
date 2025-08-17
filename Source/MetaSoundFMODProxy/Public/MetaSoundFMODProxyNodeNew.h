// Pavel Penkov 2025 All Rights Reserved.

#pragma once

#include "MetasoundFacade.h"
#include "MetasoundNodeInterface.h"
#include "MetasoundBuilderInterface.h"
#include "MetasoundVertex.h"
#include "MetasoundParamHelper.h"
#include "MetasoundTrigger.h"
#include "MetasoundExecutableOperator.h"
#include "MetasoundOperatorInterface.h"
#include "MetasoundPrimitives.h"
#include "MetasoundDataReference.h"
#include "FMODEvent.h"
#include "MetaSoundFMODProxyDataTypes.h"
#include "MetasoundDataReferenceMacro.h"
#include "MetasoundDataReferenceCollection.h"

namespace Metasound
{
    class FMetaSoundFMODProxyNewOperator : public TExecutableOperator<FMetaSoundFMODProxyNewOperator>
    {
    public:
        FMetaSoundFMODProxyNewOperator(
            const FBuildOperatorParams& InParams,
            const FTriggerReadRef& InPlayTrigger,
            const FTriggerReadRef& InStopTrigger,
            const TDataReadReference<FFMODEventAsset>& InEventAsset,
            const FStringReadRef& InParam1Name,
            const FFloatReadRef& InParam1Value,
            const FStringReadRef& InParam2Name,
            const FFloatReadRef& InParam2Value,
            const FStringReadRef& InProgrammerSoundName);

        static const FNodeClassMetadata& GetNodeInfo();
        static const FVertexInterface& GetVertexInterface();
        static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutBuildResults);

        virtual void BindInputs(FInputVertexInterfaceData& InOutVertexInterfaceData) override;
        virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexInterfaceData) override;

        void Execute();

    private:
        // FMOD
        FTriggerReadRef PlayTrigger;
        FTriggerReadRef StopTrigger;
        TDataReadReference<FFMODEventAsset> EventAssetRef;
        FStringReadRef Param1Name;
        FFloatReadRef Param1Value;
        FStringReadRef Param2Name;
        FFloatReadRef Param2Value;
        FStringReadRef ProgrammerSoundName;

        FTriggerWriteRef OutFinishedTrigger;
        FBoolWriteRef OutIsPlaying;

        FGuid CurrentInstance;
    	
    	void Reset(const IOperator::FResetParams& InParams);
    };

    //------------------------------------------------------------------------------

    class FMetaSoundFMODProxyNodeNew : public FNodeFacade
    {
    public:
        // UE5.6 constructor
        FMetaSoundFMODProxyNodeNew(FNodeData InNodeData, TSharedRef<const FNodeClassMetadata> InClassMetadata)
            : FNodeFacade(InNodeData.Name, InNodeData.ID,
                TFacadeOperatorClass<FMetaSoundFMODProxyNewOperator>())
        {
        }

        // Legacy constructor for compatibility
        FMetaSoundFMODProxyNodeNew(const FNodeInitData& InitData)
            : FNodeFacade(InitData.InstanceName, InitData.InstanceID,
                TFacadeOperatorClass<FMetaSoundFMODProxyNewOperator>())
        {
        }

        virtual ~FMetaSoundFMODProxyNodeNew() = default;

        // Required for UE5.6
        static FNodeClassMetadata CreateNodeClassMetadata()
        {
            return FMetaSoundFMODProxyNewOperator::GetNodeInfo();
        }
    };
}