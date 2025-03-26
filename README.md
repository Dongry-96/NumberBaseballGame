# 🎲 채팅으로 하는 숫자 야구 게임 (Listen Server 기반)

> 언리얼 엔진의 Listen Server 방식을 활용하여 제작한 실시간 채팅 기반의 숫자 야구 게임입니다.  
> 서버-클라이언트 구조를 직접 구현하며 RPC, RepNotify 등을 활용한 **데이터 동기화** 및 **역할 제어**, **턴 관리 시스템**을 설계하였습니다.

<br>

## 📌 프로젝트 개요

- **프로젝트 명**: Number Baseball Game
- **엔진 / 언어**: Unreal Engine 5, C++
- **서버 구조**: Listen Server (호스트가 서버 역할 수행)
- **주요 기능**:
  - 숫자 야구 게임 규칙 기반 턴제 게임
  - 채팅 입력으로 숫자 전송
  - 실시간 결과 처리 (S/B/OUT)
  - Host / Guest 역할 자동 할당
  - RepNotify / ClientRPC를 통한 UI 갱신
  - 턴 제한 및 자동 전환 기능 포함

<br>

## 🧠 게임 규칙

- 서버는 1~9 사이의 서로 다른 숫자 3자리를 무작위로 생성합니다.
- 플레이어는 **"/123"** 형식으로 숫자를 입력해 정답을 추측합니다.
- 입력한 숫자와 서버의 정답을 비교해 아래와 같은 결과를 반환합니다:
  - **S (Strike)**: 자리와 숫자가 모두 일치
  - **B (Ball)**: 숫자는 일치하지만 자리가 다름
  - **OUT**: 해당되지 않음

  <br>
  
### 예시
| 서버 숫자 | 입력 | 결과  |
|-----------|------|-------|
| 386       | /124 | OUT   |
| 386       | /167 | 0S1B  |
| 386       | /367 | 1S1B  |
| 386       | /396 | 2S0B  |
| 386       | /386 | 3S0B → 승리 |

- **3S를 먼저 맞춘 플레이어가 승리**
- 게임은 자동으로 리셋되어 재시작
- **턴 제한** 존재: 입력 시간 초과 시 자동 턴 변경
- **무승부** 조건도 존재함 (양쪽 모두 실패 시)

<br>

## 🕹️ 네트워크 구조

### ✅ 역할 분배: Host vs Guest

- `GameMode::PostLogin` 오버라이딩하여 첫 접속자는 **Host**, 이후 접속자는 **Guest**로 자동 분배.
- `APlayerController`에서 역할을 판별하고 RPC 호출로 UI에 표시됨.

```cpp
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
            PlayerController->TotalTries = TotalTries; // 서버에서 TotalTries 설정
        }
        else // 두 번째 플레이어는 Guest
        {
            GuestPlayer = PlayerController;
            PlayerController->SetPlayerRole(false);
            PlayerController->TotalTries = TotalTries;
        }
    }
}
```

<br>

## 🔄 데이터 동기화: RepNotify & RPC

클라이언트와 서버 간 데이터 동기화는 Unreal의 **RepNotify**와 **Client/Server RPC**를 활용합니다.

### 📌 TotalTries 값 동기화 (RepNotify)

- 서버에서 TotalTries(총 입력 가능 횟수) 값을 설정하면, 클라이언트에 자동으로 복제되고 `OnRep_TotalTries()`가 호출됩니다.
- 이를 통해 UI 갱신이 일어납니다.

```cpp
// PlayerController.h
UPROPERTY(ReplicatedUsing = OnRep_TotalTries)
int32 TotalTries;

UFUNCTION()
void OnRep_TotalTries();

// PlayerController.cpp
void ANBG_PlayerController::OnRep_TotalTries()
{
    Client_UpdateTriesText(0);
}
```

### 📌 클라이언트 → 서버: 입력 전달 (Server RPC)

- 클라이언트가 입력한 채팅 숫자는 Server RPC를 통해 서버로 전달됩니다.

```cpp
UFUNCTION(Server, Reliable)
void Server_SendGuessToServer(const FString& Input);

void ANBG_PlayerController::Server_SendGuessToServer_Implementation(const FString& Input)
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
```

### 📌 서버 → 클라이언트: UI 갱신 (Client RPC)

- 서버에서 게임 로직 결과를 계산한 후, 클라이언트에게 UI를 갱신하라는 명령을 내립니다.

```cpp
UFUNCTION(Client, Reliable)
void Client_UpdateTriesText(int32 TriesLeft);

void ANBG_PlayerController::Client_UpdateTriesText_Implementation(int32 TriesLeft)
{
    FString TriesMessage = FString::Printf(TEXT("남은 기회: %d"), TotalTries - TriesLeft);
    UpdateText(TriesText, TriesMessage);
}
```

이처럼 각 역할과 목적에 따라 **서버와 클라이언트가 명확히 분리된 방식으로 통신**하며,
게임 내 모든 입력과 출력은 **네트워크 구조 기반으로 처리**됩니다.

<br>

## ⏱️ 턴 제어 시스템

- Host와 Guest가 번갈아 가며 숫자를 입력
- 자신의 턴에만 입력 가능
- 일정 시간 내 입력하지 않으면 자동으로 턴이 넘어감
- 턴은 GameMode에서 제어하며 PlayerController로 전달됨

<br>

## 🧪 기술적 포인트

| 항목 | 내용 |
|------|------|
| 서버 구조 | Listen Server (로컬 호스트가 서버 역할) |
| 데이터 복제 | Replicated, RepNotify, ClientRPC |
| 역할 관리 | PostLogin 기반 자동 분배 |
| UI 연동 | 클라이언트에서 남은 기회 표시 |
| 입력 처리 | 키보드 채팅 기반 숫자 입력 |
| 게임 흐름 | 3S로 승리 판정 → 자동 재시작 |

<br>

## 📎 관련 링크

- 🔗 [개발 블로그 원문](https://dong-grae.tistory.com/199)
- 💻 [GitHub 저장소](https://github.com/Dongry-96/NumberBaseballGame)
