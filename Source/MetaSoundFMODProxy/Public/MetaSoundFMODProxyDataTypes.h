// Pavel Penkov 2025 All Rights Reserved.

#pragma once

#include "MetasoundDataReference.h"
#include "FMODEvent.h"
#include "MetasoundDataReferenceMacro.h"
#include "MetasoundParameterPack.h"
#include "MetasoundAudioBuffer.h"
#include "MetasoundDataReference.h"
#include "MetasoundParameterPack.h"
#include "MetasoundAudioBuffer.h"
#include "MetasoundDataReferenceMacro.h"
#include "MetasoundDataReferenceCollection.h"
#include "MetasoundParameterPack.h"
#include "MetasoundDataFactory.h"
#include "MetasoundDataTypeRegistrationMacro.h"
#include "MetasoundVertexData.h"
#include "MetasoundFrontendDataTypeRegistry.h"
#include "MetasoundFrontendDataTypeTraits.h"
#include "MetasoundOperatorData.h"

namespace Metasound
{
    // MetaSound data type wrapper for FMOD Event assets
    struct FFMODEventAsset
    {
        TObjectPtr<UFMODEvent> Event = nullptr;
    };

    // // Optional wrapper for FFMODEventAsset
    // struct FOptionalFFMODEventAsset
    // {
    //     bool bIsSet = false;
    //     FFMODEventAsset Value;
    // };

    // Register data ref types so these can be used as MetaSound pin types
	DECLARE_METASOUND_DATA_REFERENCE_ALIAS_TYPES(FFMODEventAsset, METASOUNDFMODPROXY_API, FFMODEventAssetTypeInfo, FFMODEventAssetReadRef, FFMODEventAssetWriteRef)

	// Plural: TYPES
	// DECLARE_METASOUND_DATA_REFERENCE_TYPES(FFMODEventAsset, METASOUNDFMODPROXY_API);
    // DECLARE_METASOUND_DATA_REFERENCE_TYPES(FOptionalFFMODEventAsset, METASOUNDFMODPROXY_API);
}

// DECLARE_METASOUND_DATA_REFERENCE_TYPES_NO_ALIASES(FFMODEventAsset, METASOUNDFMODPROXY_API)