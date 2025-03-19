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

	/***새 게임 시작***/
	UFUNCTION(Server, Reliable)
	void StartNewGame();
	void StartNewGame_Implementation();

	/***게임 종료 및 재시작***/
	UFUNCTION(Server, Reliable)
	void ResetGame();
	void ResetGame_Implementation();

	/***플레이어 입력 처리***/
	void ProcessPlayerGuess(const FString& PlayerGuess, class ANBG_PlayerController* Player);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameRule")
	int32 TurnCountdown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameRule")
	int32 TotalTries;

protected:
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void BeginPlay() override;

	/***턴 시작 및 플레이어 UI업데이트***/
	void StartPlayerTurn();

	/***현재 입력 플레이어 시간제한***/
	void UpdateCountdown();

	

	/***타임 아웃 적용***/
	void HandleTurnTimeOut();

	/***사용자 입력 유효성 검증***/
	bool IsValidInput(const FString& Input);

	/***게임 규칙에 따른 결과 적용***/
	void CheckGameStatus(const FString& PlayerType, const int& Strike, const int& Ball);

	/***UI 업데이트***/
	void BroadcastMessageToAllPlayers(const FString& Message);
	void BroadcastMessageToAllPlayers(const FString& Message, bool bVisible);
	void BroadCastMessageToPlayer(class ANBG_PlayerController* Player, const FString& ServerMessage, const FString& ResultMessage, bool bVisible);
	void EndGameMessageToAllPlayers(const FString& Message);

private:
	/***PlayerController 레퍼런스***/
	class ANBG_PlayerController* HostPlayer;
	class ANBG_PlayerController* GuestPlayer;

	/***Game Rule 관련 변수***/
	int32 CountdownTime;
	int32 CurrentTurn;
	int32 HostTries;
	int32 GuestTries;

	FString SecretNumber;
	FString CorrectAnswerMessage;

	FTimerHandle TurnTimerHandle;
};
