#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "ShooterPlayerCameraManager.generated.h"

class AShooterCharacter;

UCLASS()
class SHOOTER_API AShooterPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

	AShooterPlayerCameraManager();
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera FOV")
	float InitialFOV;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera FOV")
	float FOVSpeed;
	
	UPROPERTY(BlueprintReadWrite, Category = "Camera FOV")
	float TargetFOV;

private:
	UPROPERTY(BlueprintReadOnly, meta = (allowPrivateAccess = true))
	AShooterCharacter* ShooterCharacter;
	
protected:
	virtual void BeginPlay() override;
	virtual void UpdateCamera(float DeltaTime) override;

private:
	void UpdateCameraFOV(float DeltaTime);
	
public:
	UFUNCTION(BlueprintCallable, Category = "Camera FOV")
	void SetCameraFOV(float InFOV);

	UFUNCTION(BlueprintCallable, Category = "Camera FOV")
	void SetCameraFOVSpeed(float InFOVSpeed);

public:
	void SetShooterCharacter(AShooterCharacter* InShooterCharacter);
	
public:
	UFUNCTION(BlueprintCallable, Category = "Camera FOV")
	FORCEINLINE float GetInitialFOV() const { return InitialFOV; }
};

inline void AShooterPlayerCameraManager::SetShooterCharacter(AShooterCharacter* InShooterCharacter)
{
	ShooterCharacter = InShooterCharacter;
}
