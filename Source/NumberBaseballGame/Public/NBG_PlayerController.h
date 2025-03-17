#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NBG_PlayerController.generated.h"

UCLASS()
class NUMBERBASEBALLGAME_API ANBG_PlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ANBG_PlayerController();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnInputCommitted(const FText& Text, ETextCommit::Type CommitMethod);

public:
	UFUNCTION(Server, Reliable)
	void SendGuessToServer(const FString& Input);
	void SendGuessToServer_Implementation(const FString& Input);

	UFUNCTION(Client, Reliable)
	void UpdateServerText(const FString& NewText);
	void UpdateServerText_Implementation(const FString& NewText);

	UFUNCTION(Client, Reliable)
	void UpdateResultText(const FString& NewText);
	void UpdateResultText_Implementation(const FString& NewText);

	UFUNCTION(Client, Reliable)
	void UpdateTriesText(int32 TriesLeft);
	void UpdateTriesText_Implementation(int32 TriesLeft);

	UFUNCTION(Client, Reliable)
	void UpdateTimerText(int32 SecondsLeft);
	void UpdateTimerText_Implementation(int32 SecondsLeft);

	UFUNCTION(Client, Reliable)
	void SetResultTextVisibility(bool bVisible);
	void SetResultTextVisibility_Implementation(bool bVisible);

	UFUNCTION(Client, Reliable)
	void SetInputVisibility(bool bVisible);
	void SetInputVisibility_Implementation(bool bVisible);

	UFUNCTION(Client, Reliable)
	void SetTriesTextVisibility(bool bVisible);
	void SetTriesTextVisibility_Implementation(bool bVisible);

	UFUNCTION(Client, Reliable)
	void SetTimerTextVisibility(bool bVisible);
	void SetTimerTextVisibility_Implementation(bool bVisible);

	UFUNCTION(Client, Reliable)
	void AddHistoryEntry(const FString& NewEntry);
	void AddHistoryEntry_Implementation(const FString& NewEntry);

	UFUNCTION(Client, Reliable)
	void ClearHistory();
	void ClearHistory_Implementation();

	void SetPlayerRole(bool bHost);
	bool IsHost() const { return bIsHost; }

private:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UUserWidget> PlayerWidgetClass;

	UUserWidget* PlayerWidget;

	class UVerticalBox* HistoryBox;
	class UTextBlock* ServerText;
	class UTextBlock* ResultText;
	class UTextBlock* TriesText;
	class UTextBlock* TimerText;
	class UEditableTextBox* InputText;

	bool bIsHost;
};
