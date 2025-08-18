// Pavel Penkov 2025 All Rights Reserved.

#include "SoundFMODEventFactory.h"
#include "SoundFMODEvent.h"

#if WITH_EDITOR

USoundFMODEventFactory::USoundFMODEventFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = USoundFMODEvent::StaticClass();
	 bCreateNew = true;
	 bEditAfterNew = true;
}

UObject* USoundFMODEventFactory::FactoryCreateNew(
	UClass* InClass,
	UObject* InParent,
	FName InName,
	EObjectFlags Flags,
	UObject* Context,
	FFeedbackContext* Warn)
{
	return NewObject<USoundFMODEvent>(InParent, InClass, InName, Flags);
}

#endif // WITH_EDITOR


