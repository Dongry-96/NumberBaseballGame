#include "NBG_PlayerController.h"
#include "NBG_GameMode.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/VerticalBox.h"

ANBG_PlayerController::ANBG_PlayerController()
{
	PlayerWidget = nullptr;
	ServerText = nullptr;
	ResultText = nullptr;
	TriesText = nullptr;
	InputText = nullptr;
	HistoryBox = nullptr;
}

void ANBG_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController()) return;
	if (!PlayerWidgetClass) return;

	PlayerWidget = CreateWidget<UUserWidget>(this, PlayerWidgetClass);
	if (PlayerWidget)
	{
		PlayerWidget->AddToViewport();

		ServerText = Cast<UTextBlock>(PlayerWidget->GetWidgetFromName(TEXT("ServerText")));
		ResultText = Cast<UTextBlock>(PlayerWidget->GetWidgetFromName(TEXT("ResultText")));
		TriesText = Cast<UTextBlock>(PlayerWidget->GetWidgetFromName(TEXT("TriesText")));
		TimerText = Cast<UTextBlock>(PlayerWidget->GetWidgetFromName(TEXT("TimerText")));
		InputText = Cast<UEditableTextBox>(PlayerWidget->GetWidgetFromName(TEXT("InputText")));
		HistoryBox = Cast<UVerticalBox>(PlayerWidget->GetWidgetFromName(TEXT("HistoryBox")));

		if (InputText)
		{
			SetInputVisibility(false);
			InputText->OnTextCommitted.AddDynamic(this, &ANBG_PlayerController::OnInputCommitted);
		}
		if (TriesText)
		{
			UpdateTriesText(0);
		}
	}
}

void ANBG_PlayerController::OnInputCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter && !Text.IsEmpty())
	{
		SendGuessToServer(Text.ToString());
		InputText->SetText(FText::GetEmpty());
		SetInputVisibility(false);
	}
}

void ANBG_PlayerController::SendGuessToServer_Implementation(const FString& Input)
{
	ANBG_GameMode* GameMode = Cast<ANBG_GameMode>(GetWorld()->GetAuthGameMode());
	if (GameMode)
	{
		GameMode->ProcessPlayerGuess(Input, this);
	}
}

void ANBG_PlayerController::UpdateServerText_Implementation(const FString& NewText)
{
	if (ServerText)
	{
		ServerText->SetText(FText::FromString(NewText));
	}
}

void ANBG_PlayerController::UpdateResultText_Implementation(const FString& NewText)
{
	if (ResultText)
	{
		ResultText->SetText(FText::FromString(NewText));
	}
}

void ANBG_PlayerController::UpdateTriesText_Implementation(int32 TriesLeft)
{
	if (TriesText)
	{
		FString TriesMessage = FString::Printf(TEXT("남은 기회: %d"), 3 - TriesLeft);
		TriesText->SetText(FText::FromString(TriesMessage));
	}
}

void ANBG_PlayerController::UpdateTimerText_Implementation(int32 SecondsLeft)
{
	if (TimerText)
	{
		FString TimerMessage = FString::Printf(TEXT("%d"), SecondsLeft);
		TimerText->SetText(FText::FromString(TimerMessage));
	}
}

void ANBG_PlayerController::SetResultTextVisibility_Implementation(bool bVisible)
{
	if (ResultText)
	{
		ResultText->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

void ANBG_PlayerController::SetInputVisibility_Implementation(bool bVisible)
{
	if (InputText)
	{
		if (bVisible)
		{
			InputText->SetVisibility(ESlateVisibility::Visible);
			InputText->SetIsEnabled(bVisible);
		}
		else
		{
			InputText->SetVisibility(ESlateVisibility::Hidden);
			InputText->SetIsEnabled(!bVisible);
		}
	}
}

void ANBG_PlayerController::SetTriesTextVisibility_Implementation(bool bVisible)
{
	if (TriesText)
	{
		TriesText->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

void ANBG_PlayerController::SetTimerTextVisibility_Implementation(bool bVisible)
{
	if (TimerText)
	{
		TimerText->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
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

void ANBG_PlayerController::SetPlayerRole(bool bHost)
{
	bIsHost = bHost;
}
