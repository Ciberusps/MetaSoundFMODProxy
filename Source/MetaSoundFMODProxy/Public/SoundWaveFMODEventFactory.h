// Pavel Penkov 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR
#include "Factories/Factory.h"
#include "SoundWaveFMODEventFactory.generated.h"

UCLASS(hidecategories=Object)
class USoundWaveFMODEventFactory : public UFactory
{
	GENERATED_BODY()

public:
	USoundWaveFMODEventFactory(const FObjectInitializer& ObjectInitializer);

	virtual UObject* FactoryCreateNew(
		UClass* InClass,
		UObject* InParent,
		FName InName,
		EObjectFlags Flags,
		UObject* Context,
		FFeedbackContext* Warn
	) override;

	virtual uint32 GetMenuCategories() const override;
};
#endif // WITH_EDITOR


