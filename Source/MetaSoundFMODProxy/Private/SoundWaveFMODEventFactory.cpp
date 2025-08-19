// Pavel Penkov 2025 All Rights Reserved.

#include "SoundWaveFMODEventFactory.h"
#include "SoundWaveFMODEvent.h"
#include "AssetTypeCategories.h"

#if WITH_EDITOR

USoundWaveFMODEventFactory::USoundWaveFMODEventFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = USoundWaveFMODEvent::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* USoundWaveFMODEventFactory::FactoryCreateNew(
	UClass* InClass,
	UObject* InParent,
	FName InName,
	EObjectFlags Flags,
	UObject* Context,
	FFeedbackContext* Warn)
{
	return NewObject<USoundWaveFMODEvent>(InParent, InClass, InName, Flags);
}

uint32 USoundWaveFMODEventFactory::GetMenuCategories() const
{
	return EAssetTypeCategories::Sounds;
}

#endif // WITH_EDITOR


