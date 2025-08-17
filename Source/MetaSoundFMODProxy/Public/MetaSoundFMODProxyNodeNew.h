// Pavel Penkov 2025 All Rights Reserved.

#pragma once

#include "MetasoundNodeInterface.h"
#include "MetasoundParamHelper.h"
#include "MetasoundEnumRegistrationMacro.h"
#include "MetasoundTrigger.h"
#include "MetasoundExecutableOperator.h"
#include "MetasoundOperatorInterface.h"

using namespace Metasound;

namespace Metasound
{
    class FMetaSoundFMODProxyNewOperator : public TExecutableOperator<FMetaSoundFMODProxyNewOperator>
    {
    public:
        FMetaSoundFMODProxyNewOperator(
            const FBuildOperatorParams& InParams,
            const FTriggerReadRef& InPlayTrigger,
            const FTriggerReadRef& InStopTrigger,
            const FStringReadRef& InEventPath);

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
        FStringReadRef EventPathRef;

        FTriggerWriteRef OutFinishedTrigger;
        FBoolWriteRef OutIsPlaying;

        FGuid CurrentInstance;
    	
    	void Reset(const IOperator::FResetParams& InParams);
    };

    //------------------------------------------------------------------------------

    class FMetaSoundFMODProxyNodeNew : public FNodeFacade
    {
    public:
        FMetaSoundFMODProxyNodeNew(const FNodeInitData& InitData)
            : FNodeFacade(InitData.InstanceName, InitData.InstanceID,
                TFacadeOperatorClass<FMetaSoundFMODProxyNewOperator>())
        {
        }

        virtual ~FMetaSoundFMODProxyNodeNew() = default;
    };
}