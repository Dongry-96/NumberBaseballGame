#include "NBG_PlayerController.h"
#include "NBG_GameMode.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/VerticalBox.h"
#include "Net/UnrealNetwork.h" // Replication(네트워크 복제)를 위한 헤더

ANBG_PlayerController::ANBG_PlayerController()
{
	GameMode = nullptr;
	PlayerWidget = nullptr;
	ServerText = nullptr;
	ResultText = nullptr;
	TriesText = nullptr;
	TimerText = nullptr;
	PlayerText = nullptr;
	InputText = nullptr;
	HistoryBox = nullptr;
}

void ANBG_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController()) return; // 클라이언트에서만 실행

	if (!PlayerWidgetClass) return;

	PlayerWidget = CreateWidget<UUserWidget>(this, PlayerWidgetClass);
	if (PlayerWidget)
	{
		PlayerWidget->AddToViewport();

		ServerText = Cast<UTextBlock>(PlayerWidget->GetWidgetFromName(TEXT("ServerText")));
		ResultText = Cast<UTextBlock>(PlayerWidget->GetWidgetFromName(TEXT("ResultText")));
		TriesText = Cast<UTextBlock>(PlayerWidget->GetWidgetFromName(TEXT("TriesText")));
		TimerText = Cast<UTextBlock>(PlayerWidget->GetWidgetFromName(TEXT("TimerText")));
		PlayerText = Cast<UTextBlock>(PlayerWidget->GetWidgetFromName(TEXT("PlayerText")));
		InputText = Cast<UEditableTextBox>(PlayerWidget->GetWidgetFromName(TEXT("InputText")));
		HistoryBox = Cast<UVerticalBox>(PlayerWidget->GetWidgetFromName(TEXT("HistoryBox")));

		if (InputText)
		{
			SetInputVisibility(false);
			InputText->OnTextCommitted.AddDynamic(this, &ANBG_PlayerController::OnInputCommitted); // 사용자 입력 감지 이벤트 바인딩
		}
		if (TriesText)
		{
			UpdateTriesText(0);
		}
		if (PlayerText)
		{
			bIsHost ? PlayerText->SetText(FText::FromString(FString(TEXT("Host Player")))) :
				PlayerText->SetText(FText::FromString(FString(TEXT("Guest Player"))));
		}
	}
}

/***네트워크에서 변수를 복제할 수 있도록 설정하는 메서드***/
void ANBG_PlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANBG_PlayerController, TotalTries);
}

/***사용자 입력 감지 이벤트에 바인딩되는 메서드(Enter 감지)***/
void ANBG_PlayerController::OnInputCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (!IsLocalController()) return; // 클라이언트에서만 실행

	if (CommitMethod == ETextCommit::OnEnter && !Text.IsEmpty())
	{
		SendGuessToServer(Text.ToString()); // 서버에 클라이언트 입력 전달
		InputText->SetText(FText::GetEmpty());
		SetInputVisibility(false);
	}
}

/***Text Widget 업데이트***/
void ANBG_PlayerController::UpdateText(UTextBlock* TextBlock, const FString& NewText)
{
	if (TextBlock)
	{
		TextBlock->SetText(FText::FromString(NewText));
	}
}

/***Widget의 Visible 설정***/
void ANBG_PlayerController::SetWidgetVisibility(UWidget* Widget, bool bVisible)
{
	if (Widget)
	{
		Widget->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

/***서버에 클라이언트 입력 전달, Server RPC(클라이언트에서 호출)***/
void ANBG_PlayerController::SendGuessToServer_Implementation(const FString& Input)
{
	if (!GameMode)
	{
		GameMode = Cast<ANBG_GameMode>(GetWorld()->GetAuthGameMode());
	}
	if (GameMode)
	{
		GameMode->ProcessPlayerGuess(Input, this);
	}
}

/***UI 업데이트, Client RPC(서버에서 호출)***/
void ANBG_PlayerController::UpdateServerText_Implementation(const FString& NewText)
{
	UpdateText(ServerText, NewText);
}

void ANBG_PlayerController::UpdateResultText_Implementation(const FString& NewText)
{
	UpdateText(ResultText, NewText);
}

void ANBG_PlayerController::UpdateTriesText_Implementation(int32 TriesLeft)
{
	FString TriesMessage = FString::Printf(TEXT("남은 기회: %d"), TotalTries - TriesLeft);
	UpdateText(TriesText, TriesMessage);
}

void ANBG_PlayerController::UpdateTimerText_Implementation(int32 SecondsLeft)
{
	FString TimerMessage = FString::Printf(TEXT("%d"), SecondsLeft);
	UpdateText(TimerText, TimerMessage);
}

void ANBG_PlayerController::SetResultTextVisibility_Implementation(bool bVisible)
{
	SetWidgetVisibility(ResultText, bVisible);
}

void ANBG_PlayerController::SetInputVisibility_Implementation(bool bVisible)
{
	if (InputText)
	{
		SetWidgetVisibility(InputText, bVisible);
		InputText->SetIsEnabled(bVisible);
	}
}

void ANBG_PlayerController::SetTriesTextVisibility_Implementation(bool bVisible)
{
	SetWidgetVisibility(TriesText, bVisible);
}

void ANBG_PlayerController::SetTimerTextVisibility_Implementation(bool bVisible)
{
	SetWidgetVisibility(TimerText, bVisible);
}

void ANBG_PlayerController::SetPlayerTextVisibility_Implementation(bool bVisible)
{
	SetWidgetVisibility(PlayerText, bVisible);
}

void ANBG_PlayerController::AddHistoryEntry_Implementation(const FString& NewEntry)
{
	if (!HistoryBox) return;

	UTextBlock* NewText = NewObject<UTextBlock>(this);
	if (NewText)
	{
		FSlateFontInfo FontInfo = NewText->GetFont();
		FontInfo.Size = 17;
		NewText->SetFont(FontInfo);
		NewText->SetText(FText::FromString(NewEntry));
		HistoryBox->AddChild(NewText);
	}
}

void ANBG_PlayerController::ClearHistory_Implementation()
{
	if (HistoryBox)
	{
		HistoryBox->ClearChildren();
	}
}

/***TotalTries에 대한 동기화 후 실행될 함수, Property Replication***/
void ANBG_PlayerController::OnRep_TotalTries()
{
	UpdateTriesText(0);
}

/***Host 지정 메서드***/
void ANBG_PlayerController::SetPlayerRole(bool bHost)
{
	bIsHost = bHost;
}
