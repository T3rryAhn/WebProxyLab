# Web Proxy

1. 쓰레드 적용
2. 캐싱 적용

# 캐싱
## 구조체
### `CacheEntry` 구조체
> 캐시된 파일 구조체
- `request`: 요청 복사본 포인터
- `response`: 응답 복사본 포인터
- `size`: `response`의 크기
- `timestamp`: 구조체 접근 시간. 구조체를 읽거나 쓸때마다  현재 시간(초)으로 갱신됨. <time.h>

### `Cache` 구조체
> `CacheEntry`를 담는 구조체
- `entries`: `CacheEntry` 들을 담은 리스트 포인터
- `count`: 캐시된 엔트리 갯수
- `capacity`: 캐시할 엔트리 갯수 제한(용량) 이유는 검색시간이 너무 길어지는 것을 방지하고, 캐시 삭제가 잘되는지 디버깅을 위해.
- `current_size`: 캐시한 용량 (byte)
- `lock`: 뮤텍스 

## 캐시 저장
1. 뮤텍스 잠그기 -> 왜? 동시에 여러 쓰레드가 캐시를 작성하면 꼬인다.
1. 캐시용량을 초과하는가? or 캐시 갯수를 초과하는가? -> 가장 오래된 캐시 삭제.
2. 다음의 과정들을 새로운 캐시 == `entries[count]`에 저장한다.    (`count`는 캐시된 엔트리 갯수이고 0부터 시작하므로 count 인덱스에 저장)
3. 클라이언트 요청을 동적메모리에 실제로 저장하고 해당 주소를 `CacheEntry`에 저장.
4. 서버에게 받은 응답을 동적메모리에 실제로 저장하고 해당주소를 `CacheEntry`에 저장.
5. `CacheEntry`에  `size`저장
6.  `CacheEntry`에 `time` 저장
7. `count` +1 증가하기
8. 뮤텍스 잠금 풀기

## 캐시 삭제
> 가장 오래된 == `timestamp`가 작은 `CacheEntry`삭제하기

1. 0번 인덱스부터 순차적으로 방문하면서 가장 오래된 캐시 찾기(`timestamp` == min 인)
2. 해당 인덱스 삭제하기
3. 삭제한 위치에 맨뒤에 있는 `CacheEntry`집어 넣어주기 -> 왜? 중간중간 리스트가 비어있지 않도록 하기위해(구멍방지)
4. 카운트 하나감소 (맨뒤를 삭제안하더라도 카운트감소해서 삭제된거랑 같이 동작함)

## 캐시 전달하기
> 캐시를 읽는것도 뮤텍스로 잠가주어야 한다. 왜냐하면 다른 쓰레드가 해당 캐시를 쓰거나 삭제하는중에 읽으면 오류가 나니까!

1. 뮤텍스 잠그기
2. `CacheEntry`의  `response` 의 값 버퍼에 복사하기
3. 뮤텍스 잠금풀기
4. 버퍼의 값을 클라이언트에게 전달해주기 (doit에서 진행)

-------------------------
# 이슈 해결

캐시된 html은 제대로 브라우저에 나오지만
이미지 파일은 제대로 안나오는 상황 발생
![image](https://github.com/SWJungle/P8-Week04-06/assets/122368337/a1d15622-44e7-4f56-ae23-685291e41d7b)
해당 크기만큼 안나오고 118B 만 전송되는 것을 확인

### 이유
strcpy 등 스트링 복사함수를 사용해서 `널`값을 만나면 더이상 복사를 하지않아서 널값 이전의 데이터까지만 전송이 되고 있었음.
### 해결
memcpy 로 정확한 size 만큼 전달하도록 수정함.


---------------------
# 개선할점

프록시  서버에 찍히는 로그가 쓰레드 동시성을 반영안해서 여러 쓰레드가 동작할경우 중구난방으로 찍힐수 있음.
>>> 뮤텍스로 해결해볼까?


아래는 원본
####################################################################
# CS:APP Proxy Lab
#
# Student Source Files
####################################################################

This directory contains the files you will need for the CS:APP Proxy
Lab.

proxy.c
csapp.h
csapp.c
    These are starter files.  csapp.c and csapp.h are described in
    your textbook. 

    You may make any changes you like to these files.  And you may
    create and handin any additional files you like.

    Please use `port-for-user.pl' or 'free-port.sh' to generate
    unique ports for your proxy or tiny server. 

Makefile
    This is the makefile that builds the proxy program.  Type "make"
    to build your solution, or "make clean" followed by "make" for a
    fresh build. 

    Type "make handin" to create the tarfile that you will be handing
    in. You can modify it any way you like. Your instructor will use your
    Makefile to build your proxy from source.

port-for-user.pl
    Generates a random port for a particular user
    usage: ./port-for-user.pl <userID>

free-port.sh
    Handy script that identifies an unused TCP port that you can use
    for your proxy or tiny. 
    usage: ./free-port.sh

driver.sh
    The autograder for Basic, Concurrency, and Cache.        
    usage: ./driver.sh

nop-server.py
     helper for the autograder.         

tiny
    Tiny Web server from the CS:APP text

