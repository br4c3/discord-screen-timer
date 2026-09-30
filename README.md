# discord-screen-timer

This project is to record screen time by using discord activity.

## Table of Contents

## Reference

```text
contents  → curl이 방금 받은 데이터의 주소
size      → 데이터 한 원소의 크기
nmemb     → 원소 개수
userp     → 우리가 curl에게 넘겨준 변수의 주소
```

## Initialization

If you don't have curl library,  

```bash
sudo apt update
sudo apt install build-essential pkg-config libssl-dev autoconf libtool
```

sudo apt update
sudo apt install libpsl-dev

```bash
cd /tmp

wget https://curl.se/download/curl-8.22.0.tar.gz
tar -xzf curl-8.22.0.tar.gz
cd curl-8.22.0
```

```bash
./configure \
    --prefix=/usr/local \
    --with-openssl \
    --enable-websockets
```

```bash
make -j$(nproc)
sudo make install
```

### Install dependencies

#### Ubuntu / Debian / WSL

```bash
sudo apt update
sudo apt install build-essential pkg-config libssl-dev autoconf libtool libpsl-dev
```

#### macOS

```bash
brew install curl openssl pkg-config autoconf automake libtool libpsl
```


Set environment variables.  

```bash
set -a
source .env
set +a
```

Set project code style.

```bash
clang-format -i src/*.cpp include/*h
```

## License

This project is based on [LICENSE](./LICENSE).