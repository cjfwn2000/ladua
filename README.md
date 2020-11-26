# 시리얼 기기 송신 프로그램

## Usage

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

1. 빌드에 필요한 라이브러리 설치를 확인합니다.
    * glibc (libpthread가 포함됨)
    * libssh
    ```
    (Archlinux계열)
    # pacman -S glibc
    # pacman -S libssh
    ```
    ```
    (Debian계열)
    # apt install libc6-dev
    # apt install libssh-dev
    ```

1. 자신의 컴퓨터에 SSH용 키가 존재하는 지 확인합니다.
    ```
    $ ls -al /etc/ssh/
    ```
    존재하지 않는다면 아래 "Troubleshooting - SSH키" 부분을 참조해주시길 바랍니다.

1. make 명령으로 빌드합니다.
    ```
    $ make
    ```

1. 디렉토리 bin이 생겨난 걸 확인할 수 있습니다. bin/serialserver을 실행할 수 있습니다. (루트계정으로 실행 바람)
    ```
    # bin/serialserver
    ```
    
* (참고) 실행할 때 -u와 -p 옵션으로 임의의 요구계정정보를 설정할 수 있고, -s 옵션으로 임의의 SSH 연결 포트를 설정할 수도 있습니다.
    ```
    sudo bin/serialserver -u mynameblahblah -p -s 8880
    Please input your new password:
    .....
    ```

## Troubleshooting - SSH키

1. (디렉토리 /etc/ssh/가 없는 경우) 패키지 'openssh'을 설치
    ```
    (Archlinux계열)
    # pacman -S openssh
    ```
    ```
    (Debian계열)
    # apt install openssh-server
    ```
    2단계의 /etc/ssh/ 디렉토리를 만들기 위해, 3단계의 ssh-keygen을 실행하기 위한 단계입니다.

2. 디렉토리 및 파일 존재 확인
    ```
    # ls -al /etc/ssh/
    ```
    디렉토리는 물론, ssh_host_dsa_key, ssh_host_rsa_key도 존재한다면 여기서 마칩니다.
    디렉토리만 존재하면 3단계로 가주세요.

3. 키 생성
    ```
    # ssh-keygen -A
    ```
    그리고 다시 
    ```
    # ls -al /etc/ssh/
    ```
    을 실행하여 키가 존재하면 성공입니다.