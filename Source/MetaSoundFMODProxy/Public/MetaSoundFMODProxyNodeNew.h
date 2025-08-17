// Pavel Penkov 2025 All Rights Reserved.

#include "MetasoundNodeInterface.h"
#include "MetasoundParamHelper.h"
#include "MetasoundEnumRegistrationMacro.h"
#include "Math/LFSR.h"

using namespace Metasound;

namespace Metasound
{
    class FMetaSoundFMODProxyNewOperator : public TExecutableOperator<FMetaSoundFMODProxyNewOperator>
    {
    public:
        FMetaSoundFMODProxyNewOperator(const FBuildOperatorParams& InParams,
            TDataReadReference<FTrigger> InputTriggerNext,
            TDataReadReference<int32> InNumBits);

        static const FNodeClassMetadata& GetNodeInfo();
        static const FVertexInterface& GetVertexInterface();
        static TUniquePtr<IOperator> CreateOperator(const FBuildOperatorParams& InParams, FBuildResults& OutBuildResults);

        virtual void BindInputs(FInputVertexInterfaceData& InOutVertexInterfaceData) override;
        virtual void BindOutputs(FOutputVertexInterfaceData& InOutVertexInterfaceData) override;

        void Execute();

    private:
        FTriggerReadRef TriggerNext;
        FInt32ReadRef NumBits;
        FTriggerWriteRef TriggerOnNext;
        FInt32WriteRef OutValue;


    	// FMOD
    	FTriggerReadRef PlayTrigger;
    	FTriggerReadRef StopTrigger;
    	FStringReadRef EventPathRef;

    	FTriggerWriteRef OutFinishedTrigger;
    	FBoolWriteRef OutIsPlaying;

    	FGuid CurrentInstance;

        UE::Math::FLinearFeedbackShiftRegister LFSR;
        
        void Reset();
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