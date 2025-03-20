#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ShooterPlayerController.generated.h"

class UInputMappingContext;

UCLASS()
class SHOOTER_API AShooterPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AShooterPlayerController();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shooter Input")
	UInputMappingContext* InputMappingContext;

protected:
	virtual void BeginPlay() override;
};