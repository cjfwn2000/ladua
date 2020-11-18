# 시리얼 기기 송신 프로그램

## 사용례

(사용자 권한이 충분해야 합니다. 예를 들어 root권한.)
```
# ./serialserver -f /dev/ttyACM0 -b 115200 -s 10001 -t 10002
# ./serialserver
```

## Arguments

* -f : File path. 목표기기를 가리키는 장치 파일 주소입니다. 기본값은 /dev/ttyACM0ᅟ입니다.
* -b : Baud rate. 시리얼 데이터 속도입니다. 기본값은 115200입니다.
* -s : SSH server Port. SSH 접속을 받을 포트 번호입니다. 기본값은 10001입니다.
* -t : Telnet server Port. 텔넷 접속을 받을 포트 번호입니다. 기본값은 10002입니다.

## Quickstart

TODO...