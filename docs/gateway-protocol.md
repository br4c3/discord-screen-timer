# Gateway Protocol

## Main Logic

```text
프로그램 시작
    │
    ▼
.env에서 BOT_TOKEN 읽기
    │
    ▼
Discord Gateway WebSocket 연결
    │
    ▼
HELLO (OP 10)
    │
    ├─ heartbeat_interval 저장
    │
    ▼
IDENTIFY (OP 2)
    │
    ├─ TOKEN
    └─ Intents
    │
    ▼
READY
    │
    ▼
┌──────── Gateway Event Loop ────────┐
│                                    │
│  Discord 이벤트 도착?                 │
│       │                            │
│       ├─ YES ─→ 이벤트 처리           │
│       │                            │
│       └─ NO                        │
│                                    │
│  Heartbeat 시간 됨?                  │
│       │                            │
│       └─ YES ─→ HEARTBEAT(OP 1)    │
│                    │               │
│                    └─ ACK(OP 11)   │
│                                    │
└────────────────────────────────────┘
```

## Gateway Event

```text
Gateway Event
     │
     ▼
    op?
     │
     ├─ OP 0  → Discord Event
     │
     ├─ OP 7  → Reconnect
     │
     ├─ OP 9  → Invalid Session
     │
     └─ OP 11 → Heartbeat ACK
```

## DISPATCH

```text
DISPATCH
   │
   ▼
 event["t"]
   │
   ├─ READY
   │
   ├─ GUILD_CREATE
   │
   ├─ MESSAGE_CREATE
   │       └─ !screen 같은 명령어
   │
   └─ PRESENCE_UPDATE
           └─ Activity 확인
```