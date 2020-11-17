# [개행이 매끄럽지 못한 문제]

## 전제
* 목표기기: MIMXRT1060-EVK
* 파일: /dev/ttyACM0
* Baud rate: 115200

## 현 상황
(사진1 from lattepanda)

## 현 상황의 터미널 세팅
```
$ sudo stty -F /dev/ttyACM0      
speed 115200 baud; line = 0;
min = 0; time = 1;
-brkint -imaxbel
-opost -onlcr
-isig -icanon -iexten -echo -echoe -echok -echoctl -echoke
```

## Idea
리눅스의 도구 'screen'을 이용하였을 때는 이런 이상한 출력이 나오지 않았다.
그렇다면 'screen'을 켜서 터미널 세팅이 어떻게 바뀌는지 알아보자.
```
$ sudo screen /dev/ttyACM0 115200
```

## 터미널 세팅의 변화
```
sudo stty -F /dev/ttyACM0
[sudo] password for acxalien: 
speed 115200 baud; line = 0;
min = 100; time = 2;
-icrnl -imaxbel
-opost -onlcr
-isig -icanon -echo
```
소감: 전에 비해 확연히 간소한 설정이 되었다.

## 해결 상황
위 설정대로 시행하였더니 바라던 결과가 나왔다.
(사진2 from lattepanda)

## 탐구

### 사라진 flags
* brkint: Interrupt signal을 받는다
* iexten: Non-POSIX 문자 허락
* echoe: =crterase; 문자 지우는 방법에 관한것?
* echok: Echo a newline after a kill character
  * "Kill character"가 뭐지?
* echo...

### 생겨난 flags
* icrnl: CR -> NL로 치환
  * 혹시 이게 핵심이 아닐까?

## 이용
* 우리의 송신모듈이 목표기기를 초기화할 때 위 해결상황의 터미널설정을 따르도록 해야 할 것이다.
* TdevChannel_init
  * cfmakeraw(&opts);
