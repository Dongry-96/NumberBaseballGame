#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NBG_GameMode.generated.h"

UCLASS()
class NUMBERBASEBALLGAME_API ANBG_GameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	ANBG_GameMode();

	UFUNCTION(Server, Reliable)
	void StartNewGame();
	void StartNewGame_Implementation();
	UFUNCTION(Server, Reliable)
	void ProcessPlayerGuess(const FString& PlayerGuess, class ANBG_PlayerController* Player);
	void ProcessPlayerGuess_Implementation(const FString& PlayerGuess, class ANBG_PlayerController* Player);
	UFUNCTION(Server, Reliable)
	void ResetGame();
	void ResetGame_Implementation();

protected:
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void BeginPlay() override;

	void StartPlayerTurn();
	bool IsValidInput(const FString& Input);
	void CheckGameStatus(const FString& PlayerType, const int& Strike, const int& Ball);
	void UpdateCountdown();
	void HandleTurnTimeOut();

private:
	class ANBG_PlayerController* HostPlayer;
	class ANBG_PlayerController* GuestPlayer;

	FString SecretNumber;
	FString CorrectAnswerMessage;
	int32 CurrentTurn;
	int32 HostTries;
	int32 GuestTries;
	int32 CountdownTime;

	FTimerHandle TurnTimerHandle;
};
