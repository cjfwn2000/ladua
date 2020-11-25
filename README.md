# 시리얼 기기 송신 프로그램

## 사용례

(사용자 권한이 충분해야 합니다. 예를 들어 root권한.)
```
# ./serialserver
# ./serialserver -u mobidigm -p -f /dev/ttyACM0 -s 10001 -t 10002
# ./serialserver -u lee2021 -p
```

## Arguments

* -u (name) : login User name. 셸 접속에 필요할 계정 이름을 설정합니다. 기본값은 mobidigm입니다.
* -p : login Password. 셸 접속에 필요할 계정 패스워드를 설정합니다. 서버 프로그램 시작 시 원하는 패스워드를 입력하도록 합니다. 기본값은 dj2020입니다.
* -f (path) : File path. 목표기기를 가리키는 장치 파일 주소를 설정합니다. 기본값은 /dev/ttyACM0ᅟ입니다.
* -s (portnumber) : SSH server Port. SSH 접속을 받을 포트 번호를 설정합니다. 기본값은 10001입니다.
* -t (portnumber) : Telnet server Port. 텔넷 접속을 받을 포트 번호를 설정합니다. 기본값은 10002입니다.

## Quickstart

TODO...