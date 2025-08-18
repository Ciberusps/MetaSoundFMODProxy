// Pavel Penkov 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR
#include "Factories/Factory.h"
#include "SoundFMODEventFactory.generated.h"

UCLASS(hidecategories=Object)
class USoundFMODEventFactory : public UFactory
{
	GENERATED_BODY()

public:
	USoundFMODEventFactory(const FObjectInitializer& ObjectInitializer);

	// UFactory
	virtual UObject* FactoryCreateNew(
		UClass* InClass,
		UObject* InParent,
		FName InName,
		EObjectFlags Flags,
		UObject* Context,
		FFeedbackContext* Warn
	) override;
};

#endif // WITH_EDITOR


