#include "NBG_GameMode.h"
#include "Kismet/KismetMathLibrary.h"
#include "NBG_PlayerController.h"

ANBG_GameMode::ANBG_GameMode()
{
	TurnCountdown = 15;
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
		}
		else // 두 번째 플레이어는 Guest
		{
			GuestPlayer = PlayerController;
			PlayerController->SetPlayerRole(false);
		}
	}
}

void ANBG_GameMode::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		FTimerHandle StartGameTimer;
		GetWorldTimerManager().SetTimer(StartGameTimer, this, &ANBG_GameMode::StartNewGame, 0.5f, false);
	}
}

void ANBG_GameMode::StartNewGame_Implementation()
{
	if (!HasAuthority()) return;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ANBG_PlayerController* PlayerController = Cast<ANBG_PlayerController>(It->Get());
		if (PlayerController)
		{
			PlayerController->UpdateServerText(TEXT("숫자 야구 게임을 시작합니다"));
			PlayerController->SetInputVisibility(false);
			PlayerController->SetResultTextVisibility(false);
			PlayerController->SetTriesTextVisibility(false);
		}
	}
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

void ANBG_GameMode::StartPlayerTurn()
{
	ANBG_PlayerController* CurrentPlayer = (CurrentTurn == 0) ? HostPlayer : GuestPlayer;
	if (CurrentPlayer)
	{
		CurrentPlayer->UpdateServerText(TEXT("당신의 차례입니다. 숫자를 입력하세요!"));
		CurrentPlayer->UpdateResultText(TEXT("1 ~ 9까지의 숫자 중, 중복 없는 세 자리 숫자를 입력하세요.\n 예: /369"));
		CurrentPlayer->SetInputVisibility(true);
		CurrentPlayer->SetResultTextVisibility(true);
		CurrentPlayer->SetTriesTextVisibility(true);
		CurrentPlayer->SetTimerTextVisibility(true);

		CountdownTime = TurnCountdown;
		GetWorldTimerManager().SetTimer(TurnTimerHandle, this, &ANBG_GameMode::UpdateCountdown, 1.0f, true);
	}

	ANBG_PlayerController* OtherPlayer = (CurrentTurn == 0) ? GuestPlayer : HostPlayer;
	if (OtherPlayer)
	{
		OtherPlayer->UpdateServerText(TEXT("상대방의 차례를 기다리세요!"));
		OtherPlayer->SetInputVisibility(false);
		OtherPlayer->SetResultTextVisibility(false);
		OtherPlayer->SetTriesTextVisibility(true);
	}
}

void ANBG_GameMode::ProcessPlayerGuess_Implementation(const FString& PlayerGuess, ANBG_PlayerController* Player)
{
	if (!HasAuthority()) return;

	GetWorldTimerManager().ClearTimer(TurnTimerHandle);
	Player->SetTimerTextVisibility(false);
	Player->UpdateTimerText(TurnCountdown);

	bool bIsHost = (Player == HostPlayer);
	FString PlayerType = bIsHost ? TEXT("Host") : TEXT("Guest");

	// 유효하지 않은 입력 처리
	if (!IsValidInput(PlayerGuess))
	{
		FString OutMessage = FString::Printf(TEXT("[%s] 잘못된 입력 (OUT)"), *PlayerType);

		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			ANBG_PlayerController* PlayerController = Cast<ANBG_PlayerController>(It->Get());
			if (PlayerController)
			{
				PlayerController->UpdateResultText(OutMessage);
				PlayerController->SetResultTextVisibility(true);
				PlayerController->AddHistoryEntry(OutMessage);
			}
		}

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

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ANBG_PlayerController* PlayerController = Cast<ANBG_PlayerController>(It->Get());
		if (PlayerController)
		{
			PlayerController->UpdateResultText(ResultMessage);
			PlayerController->SetResultTextVisibility(true);
			PlayerController->AddHistoryEntry(ResultMessage);
		}
	}

	CheckGameStatus(PlayerType, Strike, Ball);
}

void ANBG_GameMode::CheckGameStatus(const FString& PlayerType, const int& Strike, const int& Ball)
{
	// 플레이어 별 시도 횟수 증가
	if (PlayerType == "Host")
	{
		HostTries++;
		HostPlayer->UpdateTriesText(HostTries);
	}
	else
	{
		GuestTries++;
		GuestPlayer->UpdateTriesText(GuestTries);
	}

	// 승리 조건 체크
	if (Strike == 3)
	{
		FString WinMessage = FString::Printf(TEXT("[%s] 승리! 다시 게임을 시작합니다."), *PlayerType);
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			ANBG_PlayerController* PlayerController = Cast<ANBG_PlayerController>(It->Get());
			if (PlayerController)
			{
				PlayerController->UpdateServerText(WinMessage);
				PlayerController->UpdateResultText(CorrectAnswerMessage);
			}
		}

		FTimerHandle RestartTimer;
		GetWorldTimerManager().SetTimer(RestartTimer, this, &ANBG_GameMode::ResetGame, 5.0f, false);
		return;
	}

	// 무승부 조건 체크
	if (HostTries >= 3 && GuestTries >= 3)
	{
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			ANBG_PlayerController* PlayerController = Cast<ANBG_PlayerController>(It->Get());
			if (PlayerController)
			{
				PlayerController->UpdateServerText(TEXT("무승부! 다시 게임을 시작합니다."));
				PlayerController->UpdateResultText(CorrectAnswerMessage);
			}
		}
		FTimerHandle RestartTimer;
		GetWorldTimerManager().SetTimer(RestartTimer, this, &ANBG_GameMode::ResetGame, 5.0f, false);
		return;
	}

	// 턴 변경
	CurrentTurn = (PlayerType == "Host") ? 1 : 0;
	StartPlayerTurn();
}

void ANBG_GameMode::UpdateCountdown()
{
	ANBG_PlayerController* CurrentPlayer = (CurrentTurn == 0) ? HostPlayer : GuestPlayer;
	if (CurrentPlayer && CountdownTime > 0)
	{
		CurrentPlayer->UpdateTimerText(CountdownTime);
		CountdownTime--;
	}
	else
	{
		GetWorldTimerManager().ClearTimer(TurnTimerHandle);
		HandleTurnTimeOut();
	}
}

void ANBG_GameMode::HandleTurnTimeOut()
{
	ANBG_PlayerController* CurrentPlayer = (CurrentTurn == 0) ? HostPlayer : GuestPlayer;
	FString PlayerType = (CurrentTurn == 0) ? TEXT("Host") : TEXT("Guest");

	// 모든 플레이어에게 시간 초과 메시지 표시
	FString OutMessage = FString::Printf(TEXT("[%s] 시간 초과 (OUT)"), *PlayerType);
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ANBG_PlayerController* PlayerController = Cast<ANBG_PlayerController>(It->Get());
		if (PlayerController)
		{
			PlayerController->UpdateResultText(OutMessage);
			PlayerController->SetResultTextVisibility(true);
			PlayerController->AddHistoryEntry(OutMessage);
		}
	}

	// 현재 플레이어의 TimeText Widget 숨김
	if (CurrentPlayer)
	{
		CurrentPlayer->SetTimerTextVisibility(false);
		CurrentPlayer->UpdateTimerText(TurnCountdown);
	}

	CurrentTurn = (CurrentTurn == 0) ? 1 : 0;
	CheckGameStatus(PlayerType, 0, 0);
}

void ANBG_GameMode::ResetGame_Implementation()
{
	// 플레이어 UI 초기화
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ANBG_PlayerController* PlayerController = Cast<ANBG_PlayerController>(It->Get());
		if (PlayerController)
		{
			PlayerController->ClearHistory();
			PlayerController->UpdateTriesText(0);
		}
	}

	StartNewGame();
}

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