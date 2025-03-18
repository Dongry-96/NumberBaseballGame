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

	/***네트워크에서 변수를 복제할 수 있도록 설정하는 메서드***/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/***사용자 입력 감지 이벤트에 바인딩되는 메서드(Enter 감지)***/
	UFUNCTION()
	void OnInputCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	/***Text Widget 업데이트***/
	void UpdateText(class UTextBlock* TextBlock, const FString& NewText);

	/***Widget의 Visible 설정***/
	void SetWidgetVisibility(class UWidget* Widget, bool bVisible);

public:
	/***서버에 클라이언트 입력 전달, Server RPC(클라이언트에서 호출)***/
	UFUNCTION(Server, Reliable)
	void SendGuessToServer(const FString& Input);
	void SendGuessToServer_Implementation(const FString& Input);

	/***UI 업데이트, Client RPC(서버에서 호출)***/
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
	void SetPlayerTextVisibility(bool bVisible);
	void SetPlayerTextVisibility_Implementation(bool bVisible);

	UFUNCTION(Client, Reliable)
	void AddHistoryEntry(const FString& NewEntry);
	void AddHistoryEntry_Implementation(const FString& NewEntry);

	UFUNCTION(Client, Reliable)
	void ClearHistory();
	void ClearHistory_Implementation();

	/***TotalTries에 대한 동기화 후 실행될 함수, Property Replication***/
	UFUNCTION()
	void OnRep_TotalTries();

	/***Replication 설정 및 OnRep 함수 바인딩***/
	// 서버에서 값 변경 시, 자동으로 동기화
	UPROPERTY(ReplicatedUsing = OnRep_TotalTries)
	int32 TotalTries;

	/***Host 지정 메서드***/
	void SetPlayerRole(bool bHost);

	/***Host 여부 반환 메서드***/
	bool IsHost() const { return bIsHost; }

private:
	/***GameMode 레퍼런스***/
	class ANBG_GameMode* GameMode;

	/***UMG 관련 변수***/
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UUserWidget> PlayerWidgetClass;
	UUserWidget* PlayerWidget;

	class UTextBlock* ServerText;
	class UTextBlock* ResultText;
	class UTextBlock* TriesText;
	class UTextBlock* TimerText;
	class UTextBlock* PlayerText;
	class UEditableTextBox* InputText;
	class UVerticalBox* HistoryBox;

	/***Host 여부 판단 변수***/
	bool bIsHost;
};
