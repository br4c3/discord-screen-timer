# discord-screen-timer

This project is to record screen time by using discord activity.

---

## Table of Contents

- [Initialization](#initialization)
- [Reference](#reference)
- [License](#license)

---

## Initialization

### Ubuntu / Debian / WSL

**Install `cURL`**  

```bash
sudo apt update
sudo apt install build-essential pkg-config libssl-dev autoconf libtool libpsl-dev
```

### macOS

**Install `cURL`**  

```bash
brew install curl openssl pkg-config autoconf automake libtool libpsl
```

**Install `json` tool**  

```bash
brew install nlohmann-json
```

### Set environment variables

Set environment variables.  

```bash
set -a
source .env
set +a
```

### Set code style

```bash
clang-format -i src/*.cpp include/*h
```

---

## Reference

- [Discord Dev - Gateway](https://docs.discord.com/developers/events/gateway)

```text
contents  → curl이 방금 받은 데이터의 주소
size      → 데이터 한 원소의 크기
nmemb     → 원소 개수
userp     → 우리가 curl에게 넘겨준 변수의 주소
```

---

## License

This project is based on [LICENSE](./LICENSE).