// Pavel Penkov 2025 All Rights Reserved.

#include "MetaSoundFMODProxyNodeTest.h"

#include "MetasoundAudioBuffer.h"
#include "MetasoundExecutableOperator.h"
#include "MetasoundFacade.h"
#include "MetasoundNodeRegistrationMacro.h"
#include "MetasoundParamHelper.h"
#include "MetasoundPrimitives.h"
#include "MetasoundVertex.h"

#include "DSP/FloatArrayMath.h"

#define LOCTEXT_NAMESPACE "MetaSoundFMODProxy"

namespace MetaSoundFMODProxy::Nodes::Peak
{
	const Metasound::FNodeClassName& GetClassName()
	{
		static Metasound::FNodeClassName ClassName
		{
			"MetaSoundFMODProxy",
			"Peak FMOD TEST",
			""
		};
		return ClassName;
	}

	namespace Inputs
	{
		DEFINE_METASOUND_PARAM(Enable, "", "");
		DEFINE_METASOUND_PARAM(AudioMono, "", "");
	}

	namespace Outputs
	{
		DEFINE_METASOUND_PARAM(Peak, "Peak FMOD Test", "The peak for the latest block");
	}

	class FOp final : public Metasound::TExecutableOperator<FOp>
	{
	public:
		static const Metasound::FVertexInterface& GetVertexInterface()
		{
			auto InitVertexInterface = []() -> Metasound::FVertexInterface
			{
				using namespace Metasound;
				
				return {
					FInputVertexInterface
					{
						TInputDataVertex<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::Enable), true),
						TInputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(Inputs::AudioMono))
					},
					Metasound::FOutputVertexInterface
					{
						TOutputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(Outputs::Peak))
					}
				};
			};
			
			static const Metasound::FVertexInterface Interface = InitVertexInterface();
			return Interface;
		}
		
		static const Metasound::FNodeClassMetadata& GetNodeInfo()
		{
			auto InitNodeInfo = []() -> Metasound::FNodeClassMetadata
			{
				Metasound::FNodeClassMetadata Info;
				Info.ClassName = GetClassName();
				Info.MajorVersion = 0;
				Info.MinorVersion = 1;
				Info.DisplayName = METASOUND_LOCTEXT("Peak_DisplayName", "Peak FMOD Test");
				Info.Description = METASOUND_LOCTEXT("Peak_Description", "Reports the peak for an audio signal");
				Info.Author = Metasound::PluginAuthor;
				Info.PromptIfMissing = Metasound::PluginNodeMissingPrompt;
				Info.DefaultInterface = GetVertexInterface();
				Info.CategoryHierarchy = { METASOUND_LOCTEXT("Metasound_FMODTESTCategory", "FMOD TEST") };
				return Info;
			};

			static const Metasound::FNodeClassMetadata Info = InitNodeInfo();

			return Info;
		}

		struct FInputs
		{
			Metasound::FAudioBufferReadRef Audio;
			Metasound::FBoolReadRef Enable;
		};

		struct FOutputs
		{
			Metasound::FFloatWriteRef Peak;
		};

		static TUniquePtr<IOperator> CreateOperator(const Metasound::FBuildOperatorParams& InParams, Metasound::FBuildResults& OutResults)
		{
			FInputs Inputs
			{
				InParams.InputData.GetOrCreateDefaultDataReadReference<Metasound::FAudioBuffer>(
					Inputs::AudioMonoName,
					InParams.OperatorSettings),
				InParams.InputData.GetOrCreateDefaultDataReadReference<bool>(
					Inputs::EnableName,
					InParams.OperatorSettings)
			};

			FOutputs Outputs
			{
				Metasound::FFloatWriteRef::CreateNew()
			};

			return MakeUnique<FOp>(InParams, MoveTemp(Inputs), MoveTemp(Outputs));
		}

		FOp(const Metasound::FBuildOperatorParams& Params, FInputs&& InInputs, FOutputs&& InOutputs)
		: Inputs(MoveTemp(InInputs))
		, Outputs(MoveTemp(InOutputs))
		{
			Reset(Params);
		}

		virtual void BindInputs(Metasound::FInputVertexInterfaceData& InVertexData) override
		{
			InVertexData.BindReadVertex(Inputs::AudioMonoName, Inputs.Audio);
			InVertexData.BindReadVertex(Inputs::EnableName, Inputs.Enable);

			UpdatePeak();
		}

		virtual void BindOutputs(Metasound::FOutputVertexInterfaceData& InVertexData) override
		{
			InVertexData.BindReadVertex(Outputs::PeakName, Outputs.Peak);
		}

		void Reset(const FResetParams&)
		{
			*Outputs.Peak = 0.0f;
		}

		void Execute()
		{
			UpdatePeak();
		}

	private:
		void UpdatePeak()
		{
			if (*Inputs.Enable)
			{
				*Outputs.Peak = Audio::ArrayMaxAbsValue(*Inputs.Audio);
			}
			else
			{
				*Outputs.Peak = 0.0f;
			}
		}
		
		FInputs Inputs;
		FOutputs Outputs;
	};

	using FPeakNode = Metasound::TNodeFacade<FOp>;
	METASOUND_REGISTER_NODE(FPeakNode);
}

#undef LOCTEXT_NAMESPACE
