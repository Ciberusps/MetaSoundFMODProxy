// Pavel Penkov 2025 All Rights Reserved.

#include "MetasoundNodeInterface.h"
#include "MetasoundParamHelper.h"

// #include "HarmonixMetasound/Common.h"

namespace MetaSoundFMODProxy::Nodes::Peak
{
	const METASOUNDFMODPROXY_API Metasound::FNodeClassName& GetClassName();
	
	namespace Inputs
	{
		DECLARE_METASOUND_PARAM(METASOUNDFMODPROXY_API, Enable);
		DECLARE_METASOUND_PARAM(METASOUNDFMODPROXY_API, AudioMono);
	}

	namespace Outputs
	{
		DECLARE_METASOUND_PARAM(METASOUNDFMODPROXY_API, Peak);
	}
}
