#include "NBG_GameMode.h"
#include "NBG_PlayerController.h"
#include "Kismet/KismetMathLibrary.h"

ANBG_GameMode::ANBG_GameMode()
{
	TurnCountdown = 15;
	TotalTries = 3;
	CurrentTurn = 0;
	HostTries = 0;
	GuestTries = 0;
	HostPlayer = nullptr;
	GuestPlayer = nullptr;
}

void ANBG_GameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	ANBG_PlayerController* PlayerController = Cast<ANBG_PlayerController>(NewPlayer);
	if (PlayerController)
	{
		if (!HostPlayer) // 첫 번째 플레이어는 Host
		{
			HostPlayer = PlayerController;
			PlayerController->SetPlayerRole(true);
			PlayerController->TotalTries = TotalTries; // 이 코드만으론 서버에 있는 PlayerController의 TotalTries만 값이 변경됨.
		}											   // 따라서 PlayerController에서 TotalTries를 Replication 속성을 추가하여 자동 동기화 하였음
		else // 두 번째 플레이어는 Guest
		{
			GuestPlayer = PlayerController;
			PlayerController->SetPlayerRole(false);
			PlayerController->TotalTries = TotalTries;
		}
	}
}

void ANBG_GameMode::BeginPlay()
{
	Super::BeginPlay();

	FTimerHandle StartGameTimer;
	GetWorldTimerManager().SetTimer(StartGameTimer, this, &ANBG_GameMode::StartNewGame, 0.5f, false);
}

/***새 게임 시작***/
void ANBG_GameMode::StartNewGame_Implementation()
{
	FString ServerMessage = FString(TEXT("숫자 야구 게임을 시작합니다!"));
	BroadcastMessageToAllPlayers(ServerMessage, false);

	// 랜덤 숫자 생성
	TArray<int32> Numbers;
	while (Numbers.Num() < 3)
	{
		int32 RandNum = UKismetMathLibrary::RandomIntegerInRange(1, 9);
		if (!Numbers.Contains(RandNum))
		{
			Numbers.Add(RandNum);
		}
	}

	SecretNumber = FString::Printf(TEXT("%d%d%d"), Numbers[0], Numbers[1], Numbers[2]);
	CorrectAnswerMessage = FString::Printf(TEXT("정답: %s"), *SecretNumber);

	// 게임 상태 초기화
	CurrentTurn = 0;
	HostTries = 0;
	GuestTries = 0;

	FTimerHandle StartTimer;
	GetWorldTimerManager().SetTimer(StartTimer, this, &ANBG_GameMode::StartPlayerTurn, 3.0f, false);
}

/***턴 시작 및 플레이어 UI업데이트***/
void ANBG_GameMode::StartPlayerTurn()
{
	ANBG_PlayerController* CurrentPlayer = (CurrentTurn == 0) ? HostPlayer : GuestPlayer;
	if (CurrentPlayer)
	{
		FString ServerMessage = FString(TEXT("당신의 차례입니다. 숫자를 입력하세요!"));
		FString ResultMessage = FString(TEXT("1 ~9까지의 숫자 중, 중복 없는 세 자리 숫자를 입력하세요.\n 예 : / 369"));
		BroadCastMessageToPlayer(CurrentPlayer, ServerMessage, ResultMessage, true);

		CountdownTime = TurnCountdown;
		GetWorldTimerManager().SetTimer(TurnTimerHandle, this, &ANBG_GameMode::UpdateCountdown, 1.0f, true);
	}

	ANBG_PlayerController* OtherPlayer = (CurrentTurn == 0) ? GuestPlayer : HostPlayer;
	if (OtherPlayer)
	{
		FString ServerMessage = FString(TEXT("상대방의 차례를 기다리세요!"));
		FString ResultMessage = FString(TEXT("1 ~9까지의 숫자 중, 중복 없는 세 자리 숫자를 입력하세요.\n 예 : / 369"));
		BroadCastMessageToPlayer(OtherPlayer, ServerMessage, ResultMessage, false);
	}
}

/***현재 입력 플레이어 시간제한***/
void ANBG_GameMode::UpdateCountdown()
{
	ANBG_PlayerController* CurrentPlayer = (CurrentTurn == 0) ? HostPlayer : GuestPlayer;
	if (CurrentPlayer && CountdownTime > 0)
	{
		CurrentPlayer->Client_UpdateTimerText(CountdownTime);
		CountdownTime--;
	}
	else
	{
		GetWorldTimerManager().ClearTimer(TurnTimerHandle);
		HandleTurnTimeOut();
	}
}

/***타임 아웃 적용***/
void ANBG_GameMode::HandleTurnTimeOut()
{
	ANBG_PlayerController* CurrentPlayer = (CurrentTurn == 0) ? HostPlayer : GuestPlayer;
	FString PlayerType = (CurrentTurn == 0) ? TEXT("Host") : TEXT("Guest");

	// 모든 플레이어에게 시간 초과 메시지 표시
	FString OutMessage = FString::Printf(TEXT("[%s] 시간 초과 (OUT)"), *PlayerType);
	BroadcastMessageToAllPlayers(OutMessage);

	CurrentTurn = (CurrentTurn == 0) ? 1 : 0;
	CheckGameStatus(PlayerType, 0, 0);
}

/***플레이어 입력 처리***/
void ANBG_GameMode::ProcessPlayerGuess(const FString& PlayerGuess, ANBG_PlayerController* Player)
{
	GetWorldTimerManager().ClearTimer(TurnTimerHandle);
	Player->Client_SetTimerTextVisibility(false);
	Player->Client_UpdateTimerText(TurnCountdown);

	// 유효하지 않은 입력 처리
	FString PlayerType = (Player == HostPlayer) ? TEXT("Host") : TEXT("Guest");

	if (!IsValidInput(PlayerGuess))
	{
		FString OutMessage = FString::Printf(TEXT("[%s] 잘못된 입력 (OUT)"), *PlayerType);
		BroadcastMessageToAllPlayers(OutMessage);

		CheckGameStatus(PlayerType, 0, 0);

		return;
	}

	// 결과 계산
	FString PureGuess = PlayerGuess.Right(3);
	int32 Strike = 0, Ball = 0;
	for (int32 i = 0; i < 3; ++i)
	{
		if (SecretNumber[i] == PureGuess[i])
		{
			Strike++;
		}
		else if (SecretNumber.Contains(FString(1, &PureGuess[i])))
		{
			Ball++;
		}
	}

	// 결과 메세지 출력
	FString ResultMessage = FString::Printf(TEXT("[%s] 입력: %s -> 결과: %dS / %dB"), *PlayerType, *PureGuess, Strike, Ball);
	BroadcastMessageToAllPlayers(ResultMessage);

	CheckGameStatus(PlayerType, Strike, Ball);
}

/***사용자 입력 유효성 검증***/
bool ANBG_GameMode::IsValidInput(const FString& Input)
{
	if (Input.Len() != 4 || !Input.StartsWith(TEXT("/"))) return false;

	FString PureInput = Input.Right(3);
	TSet<TCHAR> UniqueDigits;

	for (TCHAR Char : PureInput)
	{
		if (!FChar::IsDigit(Char) || Char == '0')
		{
			return false;
		}
		if (UniqueDigits.Contains(Char))
		{
			return false;
		}
		UniqueDigits.Add(Char);
	}
	return true;
}

/***게임 규칙에 따른 결과 적용***/
void ANBG_GameMode::CheckGameStatus(const FString& PlayerType, const int& Strike, const int& Ball)
{
	// 플레이어 별 시도 횟수 증가
	if (PlayerType == "Host")
	{
		HostTries++;
		HostPlayer->Client_UpdateTriesText(HostTries);
	}
	else
	{
		GuestTries++;
		GuestPlayer->Client_UpdateTriesText(GuestTries);
	}

	// 승리 조건 체크
	if (Strike == 3)
	{
		FString WinMessage = FString::Printf(TEXT("[%s] 승리! 다시 게임을 시작합니다."), *PlayerType);
		EndGameMessageToAllPlayers(WinMessage);
		return;
	}

	// 무승부 조건 체크
	if (HostTries >= TotalTries && GuestTries >= TotalTries)
	{
		FString DrawMessage = FString(TEXT("무승부! 다시 게임을 시작합니다."));
		EndGameMessageToAllPlayers(DrawMessage);
		return;
	}

	// 턴 변경
	CurrentTurn = (PlayerType == "Host") ? 1 : 0;
	StartPlayerTurn();
}

/***UI 업데이트***/
void ANBG_GameMode::BroadcastMessageToAllPlayers(const FString& Message)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ANBG_PlayerController* PlayerController = Cast<ANBG_PlayerController>(It->Get());
		if (PlayerController)
		{
			PlayerController->Client_UpdateResultText(Message);
			PlayerController->Client_SetResultTextVisibility(true);
			PlayerController->Client_AddHistoryEntry(Message);
		}
	}
}

void ANBG_GameMode::BroadcastMessageToAllPlayers(const FString& Message, bool bVisible)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ANBG_PlayerController* PlayerController = Cast<ANBG_PlayerController>(It->Get());
		if (PlayerController)
		{	
			PlayerController->Client_UpdateServerText(Message);
			PlayerController->Client_SetInputVisibility(bVisible);
			PlayerController->Client_SetResultTextVisibility(bVisible);
			PlayerController->Client_SetTriesTextVisibility(bVisible);
		}
	}
}

void ANBG_GameMode::BroadCastMessageToPlayer(ANBG_PlayerController* Player, const FString& ServerMessage, const FString& ResultMessage, bool bVisible)
{
	if (Player)
	{
		Player->Client_UpdateServerText(ServerMessage);
		Player->Client_UpdateResultText(ResultMessage);
		Player->Client_SetInputVisibility(bVisible);
		Player->Client_SetResultTextVisibility(bVisible);
		Player->Client_SetTimerTextVisibility(bVisible);
		Player->Client_SetTriesTextVisibility(true);
		Player->Client_SetPlayerTextVisibility(true);
	}
}

void ANBG_GameMode::EndGameMessageToAllPlayers(const FString& Message)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ANBG_PlayerController* PlayerController = Cast<ANBG_PlayerController>(It->Get());
		if (PlayerController)
		{
			PlayerController->Client_UpdateServerText(Message);
			PlayerController->Client_UpdateResultText(CorrectAnswerMessage);
		}
	}
	FTimerHandle RestartTimer;
	GetWorldTimerManager().SetTimer(RestartTimer, this, &ANBG_GameMode::ResetGame, 5.0f, false);
}

/***게임 종료 및 재시작***/
void ANBG_GameMode::ResetGame_Implementation()
{
	// 플레이어 UI 초기화
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ANBG_PlayerController* PlayerController = Cast<ANBG_PlayerController>(It->Get());
		if (PlayerController)
		{
			PlayerController->Client_ClearHistory();
			PlayerController->Client_UpdateTriesText(0);
		}
	}
	StartNewGame();
}

