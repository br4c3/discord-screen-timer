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
sudo apt install libcurl4-openssl-dev
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