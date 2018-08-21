
# HTTP/2 Parser
이 라이브러리는 HTTP/2 요청을 직접 파싱하거나, 반대로 바이트 스트림을 만들어줍니다.

## Data structure

- `class http2_frame`<br>HTTP/2에서 사용하는 Frame의 Header를 추상화한 클래스입니다.
- `class http2_frame_body`<br>HTTP/2에서 사용하는 Frame의 Body(Payload)를 추상화한 클래스입니다. 해당 클래스는 Frame의 종류에 따라 상속되어 구현됩니다. 현재는 아래 3가지의 Frame만 구현되어 있습니다.
	- `class http2_data_frame final : public http2_frame_body` (DATA FRAME)
	- `class http2_headers_frame final : public http2_frame_body` (HEADERS FRAME)
	- `class http2_settings_frame final : public http2_frame_body` (SETTINGS FRAME)

http2_frame은 Frame의 종류에 따라 http2_frame_body 인스턴스에 대한 포인터를 멤버 변수로 갖습니다. 반대로, http2_frame_body도 해당 Frame의 Header에 해당하는 http2_frame 인스턴스의 포인터를 멤버 변수로 갖습니다. 즉, 상호 참조가 가능합니다.

### class http2_frame

HTTP/2 Frame의 Header를 추상화한 클래스입니다. http2_frame 클래스는 다음과 같은 멤버 함수를 갖습니다.

 - `unsigned int get_length()` : Frame Payload의 바이트 길이를 반환합니다.
 - `unsigned char get_type()` : Frame의 종류를 반환합니다.
 - `unsigned char get_flags()` : Frame에 설정된 Flags를 반환합니다.
 - `unsigned int get_stream_id()` : Frame의 Stream Id를 반환합니다.
 - `bool get_reserved()` : Frame의 Reserved 비트가 설정되어 있는지 여부를 반환합니다.
 - `http2_frame_body* get_frame_body()` : Frame Body(Payload)의 포인터를 반환합니다. Frame의 종류에 맞게 타입 캐스팅을 하여 사용하여야 합니다.
 - `Buffer* get_frame_stream()` : 해당 Frame을 인코딩하여 (Payload를 포함한 전체) 바이트 스트림을 반환합니다.
 - `void set_length(unsigned int len)` : Frame Payload의 바이트 길이를 설정합니다.
 - `void set_type(unsigned char t)` : Frame의 종류를 설정합니다.
 - `void set_flags(unsigned char f)` : Frame의 Flags를 설정합니다.
 - `void set_stream_id(unsigned int id)` : Frame의 Stream Id를 설정합니다.
 - `void set_reserved(bool r)` : Frame의 Reserved 비트를 설정합니다.
 - `int recv_frame(const int fd)` : File descriptor로 부터 바이트 스트림을 읽어와 해당 Frame을 설정합니다.
 - `int send_frame(const int fd)` : 해당 Frame을 인코딩한 바이트 스트림을 File descriptor로 전송합니다.

### class http2_frame_body

HTTP/2 Frame의 Body에 해당하는 Payload를 추상화한 클래스입니다. http2_frame_body 클래스를 상속하는 클래스들은 공통적으로 다음과 같은 멤버 함수를 갖습니다.

 - `Buffer* get_frame_body_stream()` : Frame Body(Payload)를 인코딩하여 바이트 스트림을 얻어옵니다.
 - `http2_frame* get_frame_header()` : Frame Body의 Header의 포인터를 반환합니다.

    
다음은 http2_frame_body 클래스를 상속받아 Frame의 종류에 맞게 구체화한 클래스들입니다.

#### class http2_data_frame final : public http2_frame_body

DATA Frame을 추상화한 클래스입니다.

 - `unsigned int get_pad_length()` : Padding의 길이를 반환합니다.
 - `Buffer& get_data()` : Data를 반환합니다.
 - `void set_pad_length(unsigned char len)` : Padding 길이를 설정합니다.
 - `bool has_padding()` : Padding이 설정되어 있는지 여부를 반환합니다.

#### class http2_headers_frame final : public http2_frame_body

HEADER Frame을 추상화한 클래스입니다.

 - `unsigned char get_pad_length()` : Padding의 길이를 반환합니다.
 - `bool get_exclusive()` : (Priority가 설정된 경우) Exclusive를 반환합니다.
 - `unsigned int get_stream_dependency()` : (Priority가 설정된 경우) Stream dependency를 반환합니다.
 - `unsigned char get_weight()` : (Priority가 설정된 경우) Weight를 반환합니다.
 - `Buffer& get_header()` : Header를 반환합니다.
 - `void set_pad_length(unsigned char len)` : Padding의 길이를 반환합니다.
 - `void set_exclusive(bool e)` : Header에 Exclusive를 설정합니다.
 - `void set_stream_dependency(unsigned int dep)` : (Priority를 설정한 경우) Stream dependency를 설정합니다.
 - `void set_weight(unsigned char w)` : (Priority를 설정한 경우) Weight를 설정합니다.
 - `bool has_padding()` : Padding이 설정되어 있는지 여부를 반환합니다.
 - `bool has_priority()` : Priority가 설정되어 있는지 여부를 반환합니다.

#### class http2_settings_frame final : public http2_frame_body

SETTINGS Frame을 추상화한 클래스입니다.

 - `http2_settings& get_settings()` : Setting을 반환합니다.
 - `void set_settings(http2_settings set)` : Setting을 설정합니다.

## Functions

 - `bool http2_check_preface(const char* buffer, int len)` : Buffer로부터 Preface를 확인합니다.
 - `bool http2_check_preface(int fd)` : File descriptor로부터 Preface를 읽어옵니다.

위 두 함수는 상대방으로부터 최초 HTTP/2 요청이 들어왔을 때 Preface를 확인하는 함수입니다. HTTP/2 통신을 시작하기 전에 꼭 사용하여야 하는 함수입니다.

## HTTP(Hypertext Transfer Protocol)의 역사
### HTTP의 시작
HTTP는 1989년에 CERN(유럽원자핵공동연구소)에서 근무하던 팀 버너스 리(Tim Berners-Lee)가 각종 실험에서 나오는 정보들을 관리하기 위한 시스템을 제안하면서 World Wide Web (WWW)과 함께 만들어졌습니다.

Hypertext라는 용어는 1965년 소프트웨어 설계자이자 철학자인 테드넬슨에 의해 처음 발표되었습니다. 넬슨은 상호 연결되어 누구나 쉽게 정보를 탐색할 수 있는 문서우주(Docuverse)를 만들고자 했고, 1970년 제어두(Xanadu) 프로젝트에서 처음 하이퍼텍스트의 시제품을 구현했습니다.

팀 버너스 리는 넬슨이 제안한 두 가지 개념을 HTTP 고안에 채택합니다.

 1. 하이퍼텍스트(Hypertext): 문서들이 제약이 없는 방식으로 서로 연결되어 사람이 읽을 수 있는 정보
 2. 하이퍼미디어(Hypermedia): 정보는 반드시 텍스트 형태일 필요가 없다.

### HTTP/0.9
HTTP/0.9는 아주 간단하게 GET 요청만 지원하는 (데이터를 가져오기만 하는) 프로토콜이었습니다. 요청은 Header 같은 것도 없는 한 줄 짜리의 단순한 형태였고, 이미지와 같은 미디어는 처리할 수 없었으며 텍스트로 구성된 HTML만 가져올 수 있었습니다.

> (example)
> GET /my-page.html

### HTTP/1.0
90년대 초반 80번 포트를 HTTP로 사용하는 서버의 수가 크게 증하였고, 1996년에 HTTP/0.9를 대폭 개선한 HTTP/1.0이 RFC 1945로 발표되었습니다. 1.0으로 넘어오면서 다음과 같은 개념이 추가되었습니다.

 - Header
 - Response codes
 - Redirects
 - Errors
 - Conditional requests
 - Encoding (Compression)
 - Request methods

이렇게 1.0은 0.9에 비해 크게 개선이 되었지만, 여전히 해결해야 할 점들이 많았습니다. 여러 요청을 연결이 유지된 상태에서 처리할 수 없었으며, Host 헤더(서버의 도메인과 포트 번호를 적는 헤더)를 적는 것이 필수가 아니었기 때문에 가상 호스팅(동일 IP주소를 갖는 단일 서버에서 여러 사이트를 구축하는 것)이 불가능했습니다.

### HTTP/1.1
1.0이 나오고 머지 않아 1.1이 발표되었으며, 지금까지 가장 널리 사용 되는 HTTP 프로토콜입니다. 1.0에서의 문제들이 많이 수정되었습니다. Host 헤더를 필수로 적도록 하여 가상 호스팅이 가능해졌고, directive를 이용함으로써 TCP 연결이 유지된 상태에서 여러 요청을 보낼 수 있도록 개선하였습니다.
### HTTP/2

## Frame
### 종류
HTTP/2의 모든 메시지는 Frame 형태로 이동합니다. Frame의 종류는 다음과 같으며, 각 Frame 종류에 따라 Type값이 지정되어 있습니다.

 - DATA (0x0)
 - HEADERS (0x1)
 - ~~PRIORITY (0x2)~~
 - ~~RST_STREAM (0x3)~~
 - SETTINGS (0x4)
 - ~~PUSH_PROMISE (0x5)~~
 - ~~PING (0x6)~~
 - ~~GOAWAY (0x7)~~
 - ~~WINDOW_UPDATE (0x8)~~
 - ~~CONTINUATION (0x9)~~
 
 **(대시 처리된 것은 라이브러리에서 아직 지원하지 않습니다.)**
 
 ### 구조
 모든 Frame은 9Byte 길이의 Header를 갖고 있으며, Header 뒤에는 Frame 종류에 따른 Payload가 붙습니다. 즉, 모든 Frame의 구조는 다음과 같습니다.
 <html><pre>
+-----------------------------------------------+
|                 Length (24)                   |
+---------------+---------------+---------------+
|   Type (8)    |   Flags (8)   |
+-+-------------+---------------+-------------------------------+
|R|                 Stream Identifier (31)                      |
+=+=============================================================+
|                   Frame Payload (0...)                      ...
+---------------------------------------------------------------+
 </pre></html>
 
 - Length: Frame Payload의 길이를 바이트 단위로 저장합니다.
 - Type: Frame의 종류값을 저장합니다.
 - Flags: Frame의 종류에 따라 옵션을 지정할 때 사용합니다.
 - R: Reserved된 bit입니다.
 - Stream Identifier: 서로 연관된 요청과 응답을 구분하기 위한 값입니다.
 - Frame Payload: Frame의 종류에 맞는 Payload 입니다.

즉, HTTP/2 메시지를 파싱하기 위해서는 먼저 9바이트 길이의 Header를 읽고, Frame의 종류와 길이에 맞게 Frame Payload를 추가적으로 읽어와야 합니다.

## Payload
다음은 Frame 종류에 따른 Payload의 구조입니다.

### DATA Frame (0x0)
<html>
<pre>
+---------------+
|Pad Length? (8)|
+---------------+-----------------------------------------------+
|                            Data (*)                         ...
+---------------------------------------------------------------+
|                           Padding (*)                       ...
+---------------------------------------------------------------+
</pre>
</html>

 - Pad Length: Header의 Flags를 PADDED로 설정한 경우 Padding의 길이를 지정합니다.
 - Data: 전송하고자 하는 데이터를 담는 부분입니다. 예를 들어, HTTP/1.1 응답이 될 수 있습니다.
 - Padding: Header의 Flags를 PADDED로 설정한 경우 Padding을 덧붙이는 부분입니다. Padding은 모든 바이트의 값이 0으로 설정됩니다.

DATA Frame은 Header에 다음과 같은 옵션을 Flags에 설정할 수 있습니다.
|옵션|값|내용|
|--|--|--|
|END_STREAM|0x1|해당 스트림에서 전송하는 마지막 프레임일 때 설정합니다.
|PADDED|0x8|Frame에 Padding을 사용할 때 설정합니다.

### HEADERS Frame (0x1)
<html><pre>
+---------------+
|Pad Length? (8)|
+-+-------------+-----------------------------------------------+
|E|                 Stream Dependency? (31)                     |
+-+-------------+-----------------------------------------------+
|  Weight? (8)  |
+-+-------------+-----------------------------------------------+
|                   Header Block Fragment (*)                 ...
+---------------------------------------------------------------+
|                           Padding (*)                       ...
+---------------------------------------------------------------+
</pre></html>

### SETTINGS Frame (0x2)
<html><pre>
+-------------------------------+
|       Identifier (16)         |
+-------------------------------+-------------------------------+
|                        Value (32)                             |
+---------------------------------------------------------------+
</pre></html>
