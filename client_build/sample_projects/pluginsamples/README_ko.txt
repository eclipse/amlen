IBM® IoT MessageSight™ Protocol Plug-in SDK V2.0
2016년 6월

목차
1. 설명
2. IBM IoT MessageSight Protocol Plug-in SDK 컨텐츠
3. 샘플 JSON 프로토콜 플러그인 샘플 코드에 대한 작업
4. Eclipse로 샘플 JSON 프로토콜 플러그인 SDK 및 샘플 가져오기
5. 샘플 REST 프로토콜 플러그인 샘플 코드에 대한 작업
6. Eclipse로 샘플 REST 프로토콜 플러그인 SDK 및 샘플 가져오기
7. 제한사항 및 알려진 문제점


설명
====
IBM IoT MessageSight Protocol Plug-in SDK에 대한 이 Readme 파일에는
컨텐츠, 업데이트, 수정사항, 제한사항 및 알려진 문제점에 대한 정보가 포함되어 있습니다.
이 SDK는 Java 프로그래밍 언어로 플러그인을 작성하여 IBM IoT MessageSight 서버에서
기본적으로 지원하는 프로토콜 세트를 확장하는 데 사용됩니다. 

IBM IoT MessageSight에 대한 자세한 정보는 IBM Knowledge Center(http://ibm.biz/iotms_v20)에
있는 제품 문서를 참조하십시오.

이 릴리스의 새로운 기능
=======================
이제 비HA 환경에서 IBM IoT MessageSight Protocol Plug-in SDK가 완전히 지원됩니다.
사용 제한사항에 대해서는 함께 제공되는 라이센스를 참조하십시오.
이제 RESTful API를 통해 플러그인 배치 및 구성이 수행됩니다. 

IBM IoT MessageSight Protocol Plug-in SDK 컨텐츠
================================================
ProtocolPluginV2.0.zip 패키지의 ImaTools/ImaPlugin 서브디렉토리에서
IBM IoT MessageSight Protocol Plug-in SDK를 제공합니다. 여기에는
IBM IoT MessageSight에서 사용할 프로토콜 플러그인을 작성하는 데 사용되는
클래스에 대한 문서도 있습니다. 마지막으로 소스 컴포넌트 및 샘플 자료가
있습니다. 

디렉토리 및 파일:
    ImaTools/
        license/ - IBM IoT MessageSight 프로토콜 플러그인 기능에 대한 라이센스 계약
            파일이 있습니다.
        ImaPlugin/
            README.txt - 이 파일

            version.txt - IBM IoT MessageSight Protocol Plug-in SDK의
                버전 정보가 있습니다.

            doc/ - IBM IoT MessageSight Protocol Plug-in SDK를 사용하여
                프로토콜 플러그인을 작성하는 방법에 대한 문서가 있습니다.

            lib/
                imaPlugin.jar - IBM IoT MessageSight Protocol Plug-in SDK

                jsonplugin.zip - IBM IoT MessageSight에 배치할 수 있는 샘플 JSON
                    프로토콜 플러그인이 있는 zip 아카이브입니다. 이 아카이브에는
                    samples/jsonmsgPlugin 서브디렉토리에서 제공되는 샘플
                    플러그인 코드(소스 컴포넌트)에 대한 컴파일된 클래스(샘플 자료)가
                    있습니다.

                jsonprotocol.jar - 샘플 플러그인에 대한 컴파일된 클래스(샘플
                    자료)입니다. 이 jar 파일은 jsonplugin.zip 파일에도
                    있습니다.

                restplugin.zip - IBM IoT MessageSight에 배치할 수 있는 샘플 REST
                    프로토콜 플러그인이 있는 zip 아카이브입니다. 이 아카이브에는
                    samples/restmsgPlugin 서브디렉토리에서 제공되는 샘플
                    플러그인 코드(소스 컴포넌트)에 대한 컴파일된 클래스(샘플 자료)가
                    있습니다.

                restprotocol.jar - 샘플 REST 프로토콜 플러그인에 대한 컴파일된
                    클래스(샘플 자료)입니다. 이 jar 파일은 restplugin.zip 파일에도
                    있습니다.

            samples/
                jsonmsgPlugin/
                    .classpath 및 .project - jsonmsgPlugin 서브디렉토리를
            	        Eclipse 프로젝트로 가져올 수 있도록 하는 Eclipse
            	        프로젝트 구성 파일
            	
            	    build_plugin_zip.xml - 플러그인 아카이브를 작성하는 데
            	        사용할 수 있는 Ant 빌드 파일
            	
                    src/ - IBM IoT MessageSight에서 사용할 프로토콜
                        플러그인을 작성하는 방법을 설명하는 샘플 플러그인(소스
                        컴포넌트)의 Java 소스 코드가 있습니다.

                    config/ - IBM IoT MessageSight에 컴파일된 샘플 플러그인을
                        배치하는 데 필요한 JSON 디스크립터 파일(소스 컴포넌트)이
                        있습니다.

                    doc/ - 샘플 플러그인 소스 코드에 대한 문서가 있습니다. 

                jsonmsgClient/
                    .classpath 및 .project - jsonmsgClient 서브디렉토리를
            	        Eclipse 프로젝트로 가져올 수 있도록 하는 Eclipse
            	        프로젝트 구성 파일
            	
            	    JSONMsgWebClient.html - jsonmsgPlugin에서 제공되는 샘플
            	        플러그인의 클라이언트 애플리케이션(소스 컴포넌트)입니다.
            	        이 클라이언트는 js 서브디렉토리에 있는 JavaScript™
            	        라이브러리를 기반으로 합니다.
            	
                    js/ - jsonmsgPlugin에서 제공되는 샘플 플러그인에서 사용할
                        샘플 JavaScript 클라이언트 라이브러리(소스 컴포넌트)가
                        있습니다.

                    css/ - 샘플 클라이언트 애플리케이션 및 샘플 클라이언트
                        라이브러리에서 사용하는 스타일시트(소스 컴포넌트) 및
                        이미지 파일이 있습니다.

                    doc/ - 샘플 JavaScript 클라이언트 라이브러리에 대한
                        문서가 있습니다.

                restmsgPlugin/
                    .classpath 및 .project - restmsgPlugin 서브디렉토리를
            	        Eclipse 프로젝트로 가져올 수 있도록 하는 Eclipse
            	        프로젝트 구성 파일
            	
            	       build_plugin_zip.xml - 플러그인 아카이브를 작성하는 데
            	        사용할 수 있는 Ant 빌드 파일
            	
                    src/ - IBM IoT MessageSight에서 사용할 프로토콜
                        플러그인을 작성하는 방법을 설명하는 샘플 플러그인(소스
                        컴포넌트)의 Java 소스 코드가 있습니다.

                    config/ - IBM IoT MessageSight에 컴파일된 샘플 플러그인을
                        배치하는 데 필요한 JSON 디스크립터 파일(소스 컴포넌트)이
                        있습니다.

                    doc/ - 샘플 플러그인 소스 코드에 대한 문서가 있습니다.

                restmsgClient/
                    .classpath 및 .project - restmsgClient 서브디렉토리를
            	        Eclipse 프로젝트로 가져올 수 있도록 하는 Eclipse
            	        프로젝트 구성 파일
            	
               	    RESTMsgWebClient.html - restmsgPlugin에서 제공되는 샘플
            	           플러그인의 클라이언트 애플리케이션(소스 컴포넌트)입니다.
                        이 클라이언트는 js 서브디렉토리에 있는 JavaScript
                        라이브러리를 기반으로 합니다.

                    js/ - restmsgPlugin에서 제공되는 샘플 플러그인에서 사용할
                        샘플 JavaScript 클라이언트 라이브러리(소스 컴포넌트)가
                        있습니다.

                    css/ - 샘플 클라이언트 애플리케이션 및 샘플 클라이언트
                        라이브러리에서 사용하는 스타일시트(소스 컴포넌트) 및
                        이미지 파일이 있습니다.

                    doc/ - 샘플 JavaScript 클라이언트 라이브러리에 대한
                        문서가 있습니다.

샘플 JSON 프로토콜 플러그인 및 클라이언트 애플리케이션에 대한 작업
==================================================================
JSONMsgWebClient.html 애플리케이션은 jsonmsg.js JavaScript 클라이언트
라이브러리를 사용하여 json_msg 플러그인과 통신합니다. JSONMsgWebClient.html은
json_msg 플러그인과 메시지를 QoS 0(최대 한 번) 송수신하기 위한
웹 인터페이스를 제공합니다. 

json_msg 프로토콜 플러그인 샘플은 두 개의 클래스(JMPlugin 및 JMConnection)로
구성됩니다. JMPlugin은 프로토콜 플러그인을 초기화하고 시작하는 데 필요한
ImaPluginListener 인터페이스를 구현합니다. JMConnection은 클라이언트가
IBM IoT MessageSight에 연결하고 json_message 프로토콜을 통해 메시지를
송수신하는 데 사용되는 ImaConnectionListener 인터페이스를 구현합니다.
JMConnection 클래스는 발행 클라이언트에서 JSON 오브젝트를 수신하고
ImaJson 유틸리티 클래스를 사용하여 이러한 오브젝트를 명령 및 메시지로
구문 분석한 후 IBM IoT MessageSight 서버로 전송합니다. 마찬가지로,
JMConnection 클래스는 ImaJson 유틸리티 클래스를 사용하여 IBM IoT
MessageSight 메시지를 구독 json_msg 클라이언트용 JSON 오브젝트로 변환합니다.
마지막으로, json_msg 프로토콜 플러그인에는 plugin.json이라는 디스크립터
파일이 포함됩니다. 이 디스크립터 파일은 프로토콜 플러그인을 배치하는 데
사용되는 zip 아카이브에서 필요하며 항상 이 파일 이름을 사용해야 합니다.
json_msg 프로토콜에 대한 샘플 플러그인 아카이브인 jsonplugin.zip이
ImaTools/ImaPlugin/lib 디렉토리에 포함되어 있습니다. 여기에는 jar
파일(컴파일된 JMPlugin 및 JMConnection 클래스가 포함된 jsonprotocol.jar)
및 plugin.json 디스크립터 파일이 있습니다. 

샘플 JavaScript 클라이언트 애플리케이션을 실행하려면 먼저 IBM IoT MessageSight에
샘플 플러그인을 배치해야 합니다. 이 플러그인은 zip 또는 jar 파일에 아카이브되어
있어야 합니다. 이 파일에는 대상 프로토콜에 대한 플러그인을 구현하는 jar 파일이
있어야 합니다. 또한 플러그인 컨텐츠를 설명하는 JSON 디스크립터 파일도 있어야
합니다. jsonplugin.zip이라는 아카이브는 ImaTools/ImaPlugin/lib에서 사용 가능하며
json_msg 샘플 플러그인을 포함합니다. 이 샘플 플러그인 아카이브를 배치하면
웹 브라우저에 JSONMsgWebClient.html(ImaTools/ImaPlugin/samples/jsonmsgClient에
있음)을 로드하여 샘플 클라이언트 애플리케이션을 실행할 수 있습니다. 샘플
애플리케이션용 샘플 JavaScript 클라이언트 라이브러리 및 스타일시트를
로드하려면 JSONMsgWebClient.html이 있는 서브디렉토리 구조를 유지보수해야
합니다. 


IBM IoT MessageSight에 샘플 플러그인 아카이브 배치:
===================================================
IBM IoT MessageSight에 jsonplugin.zip을 배치하려면 PUT 메소드를 사용하여
플러그인 zip 파일 jsonmsg.zip을 가져와야 합니다. 다음 cURL을 사용하십시오.
	curl -X PUT -T jsonmsg.zip http://127.0.0.1:9089/ima/v1/file/jsonmsg.zip

다음으로, jsonmsg라는 프로토콜 플러그인을 작성하는 POST 메소드를 사용하여
플러그인을 작성해야 합니다. 다음 cURL을 사용하십시오.
    curl -X POST \
       -H 'Content-Type: application/json'  \
       -d  '{
               "Plugin": {
                "jsonmsg": {
                 "File": "jsonmsg.zip"
                }
             }
           }
     '  \
    http://127.0.0.1:9089/ima/v1/configuration/

마지막으로 플러그인을 시작하기 위해 IBM IoT MessageSight 프로토콜 플러그인 서버를
재시작해야 합니다. 다음 cURL을 사용하십시오.
	curl -X POST -d  '{"Service":"Plugin"}' http://127.0.0.1:9089/ima/v1/service/restart

플러그인이 성공적으로 배치되었는지 확인하려면 다음 cURL을 사용하십시오.
    curl -X GET http://127.0.0.1:9089/ima/v1/configuration/Plugin

결과 목록에 json_msg가 표시됩니다. 이 값은 plugin.json 디스크립터 파일에
지정된 이름 특성에 해당합니다. 

새 프로토콜이 성공적으로 등록되었는지 확인하려면 다음 cURL을 사용하십시오.
    curl -X GET http://127.0.0.1:9089/ima/v1/configuration/Protocol

결과 목록에 json_msg가 표시됩니다. 이 값은 plugin.json 디스크립터 파일에
지정된 프로토콜 특성에 해당합니다. 

DemoEndpoint를 사용 중인 경우, 샘플 json_msg 프로토콜 플러그인에 액세스하기
위해 추가 단계가 필요하지 않습니다. 다른 엔드 포인트를 사용 중인 경우에는
엔드 포인트, 연결 정책 또는 메시징 정책에서 허용하는 프로토콜을 지정하기
위한 프로시저와 동일한 프로시저를 수행해야 합니다. DemoEndpoint 및 해당 정책은
정의된 모든 프로토콜을 허용하도록 사전 구성되어 있습니다. 

JavaScript 샘플 클라이언트 애플리케이션 실행:
=============================================
웹 브라우저에서 JSONMsgWebClient.html을 여십시오. 서버 텍스트 상자에
IBM IoT MessageSight의 호스트 이름 또는 IP 주소를 지정하십시오. DemoEndpoint를
사용 중인 경우 포트 텍스트 상자에 나열된 대로 포트 16102를 사용할 수 있습니다.
그렇지 않은 경우, 샘플 플러그인으로 구성된 IBM IoT MessageSight 엔드 포인트의
올바른 포트 번호를 지정하십시오. 추가 연결 옵션을 사용하려면 세션 탭을
선택하십시오. 연결 특성 설정을 완료한 후에는 연결 단추를 클릭하여
IBM IoT MessageSight에 연결하십시오. 그런 다음 토픽 탭을 사용하여 샘플
플러그인을 통해 IBM IoT MessageSight 목적지와 메시지를 송수신하십시오. 


Eclipse로 json_msg 클라이언트 가져오기
======================================
Eclipse로 IBM IoT MessageSight json_msg 클라이언트를 가져오려면 다음 단계를
완료하십시오.
1. ProtocolPluginV2.0.zip의 컨텐츠를 압축 해제하십시오.
2. Eclipse에서 파일->가져오기->일반->기존 프로젝트를 작업공간으로를 선택하십시오.
3. 다음을 클릭하십시오.
4. "루트 디렉토리 선택" 단일 선택 단추를 선택하십시오.
5. 찾아보기를 클릭하십시오.
6. 압축 해제된 zip 파일 컨텐츠의 ImaTools/ImaPlugin/samples/jsonmsgClient
   서브디렉토리로 이동하여 선택하십시오.
7. "작업공간으로 프로젝트 복사" 선택란이 선택된 경우 이 선택란을 선택 취소하십시오.
8. 완료를 클릭하십시오. 

이 단계를 완료한 후에는 Eclipse에서 샘플 애플리케이션을 실행할 수 있습니다. 

Eclipse로 json_msg 플러그인 가져오기
====================================
Eclipse로 IBM IoT MessageSight json_msg 플러그인을 가져오려면 다음 단계를
완료하십시오.
1. ProtocolPluginV2.0.zip의 컨텐츠를 압축 해제하십시오.
2. Eclipse에서 파일->가져오기->일반->기존 프로젝트를 작업공간으로를 선택하십시오.
3. 다음을 클릭하십시오.
4. "루트 디렉토리 선택" 단일 선택 단추를 선택하십시오.
5. 찾아보기를 클릭하십시오.
6. 압축 해제된 zip 파일 컨텐츠의 ImaTools/ImaPlugin/samples/jsonmsgPlugin
   서브디렉토리로 이동하여 선택하십시오.
7. "작업공간으로 프로젝트 복사" 선택란이 선택된 경우 이 선택란을 선택 취소하십시오.
8. 완료를 클릭하십시오. 

이 단계를 완료한 후에는 Eclipse에서 샘플 플러그인 및 사용자 자신의
플러그인을 컴파일할 수 있습니다.

Eclipse에 플러그인 아카이브 빌드
================================
json_msg 샘플 플러그인을 업데이트하고 IBM IoT MessageSight에 배치할
아카이브를 작성하려는 경우 다음 단계를 따르십시오.
참고: 다음 단계에서는 Eclipse에 jsonmsgPlugin 프로젝트를 가져왔으며
Eclipse에서 프로젝트 소스 파일을 이미 컴파일했고 bin 서브디렉토리에
클래스 파일을 배치했다고 가정합니다.
1. Eclipse의 패키지 탐색기 또는 네비게이터에서 jsonmsgPlugin을 여십시오.
2. build_plugin_zip.xml을 마우스의 오른쪽 단추로 클릭하고 실행 도구 -> Ant 빌드를 선택하십시오.
   jsonmsgPlugin 아래에 새 jsonplugin.zip 파일이 포함된 플러그인 서브디렉토리가
   작성됩니다. jsonmsgPlugin 프로젝트를 새로 고쳐 Eclipse UI에 이 플러그인
   서브디렉토리를 표시할 수 있습니다. 


샘플 REST 프로토콜 플러그인 및 클라이언트 애플리케이션에 대한 작업
==================================================================
RESTMsgWebClient.html 애플리케이션은 restmsg.js JavaScript 클라이언트
라이브러리를 사용하여 restmsg 플러그인과 통신합니다. RESTMsgWebClient.html은
restmsg 플러그인과 메시지를 QoS 0(최대 한 번) 송수신하기 위한
웹 인터페이스를 제공합니다. 

restmsg 프로토콜 플러그인 샘플은 두 개의 클래스(RestMsgPlugin 및 RestMsgConnection)로
구성됩니다. RestMsgPlugin은 프로토콜 플러그인을 초기화하고 시작하는 데 필요한
ImaPluginListener 인터페이스를 구현합니다. RestMsgConnection은 클라이언트가
IBM IoT MessageSight에 연결하고 restmsg 프로토콜을 통해 메시지를
송수신하는 데 사용되는 ImaConnectionListener 인터페이스를 구현합니다.
RestMsgConnection 클래스는 발행 클라이언트에서 REST 오브젝트를 수신하고
이러한 오브젝트를 명령 및 메시지로 구문 분석한 후 IBM IoT MessageSight
서버로 전송합니다. 마찬가지로, RestMsgConnection은 IBM IoT MessageSight
메시지를 구독 restmsg 클라이언트용 REST 오브젝트로 변환합니다.
마지막으로, restmsg 프로토콜 플러그인에는 plugin.json이라는 디스크립터
파일이 포함됩니다. 이 디스크립터 파일은 프로토콜 플러그인을 배치하는 데
사용되는 zip 아카이브에서 필요하며 항상 이 파일 이름을 사용해야 합니다.
restmsg 프로토콜에 대한 샘플 플러그인 아카이브인 restplugin.zip이
ImaTools/ImaPlugin/lib 디렉토리에 포함되어 있습니다. 여기에는 jar
파일(컴파일된 RestMsgPlugin 및 RestMsgConnection 클래스가 포함된 restprotocol.jar)
및 plugin.json 디스크립터 파일이 있습니다. 

샘플 JavaScript 클라이언트 애플리케이션을 실행하려면 먼저 IBM IoT MessageSight에
샘플 플러그인을 배치해야 합니다. 이 플러그인은 zip 또는 jar 파일에 아카이브되어
있어야 합니다. 이 파일에는 대상 프로토콜에 대한 플러그인을 구현하는 jar 파일이
있어야 합니다. 또한 플러그인 컨텐츠를 설명하는 JSON 디스크립터 파일도 있어야
합니다. restplugin.zip이라는 아카이브는 ImaTools/ImaPlugin/lib에서 사용 가능하며
restmsg 샘플 플러그인을 포함합니다. 샘플 restmsg 플러그인 아카이브를 배치하면
웹 브라우저에 RESTMsgWebClient.html(ImaTools/ImaPlugin/samples/restmsgClient에
있음)을 로드하여 샘플 클라이언트 애플리케이션을 실행할 수 있습니다. 샘플
애플리케이션용 샘플 JavaScript 클라이언트 라이브러리 및 스타일시트를
로드하려면 RESTMsgWebClient.html이 있는 서브디렉토리 구조를 유지보수해야
합니다. 


IBM IoT MessageSight에 REST 샘플 플러그인 아카이브 배치:
========================================================
IBM IoT MessageSight에 restplugin.zip을 배치하려면 PUT 메소드를 사용하여
플러그인 zip 파일 jsonmsg.zip을 가져와야 합니다. 다음 cURL을 사용하십시오.
	curl -X PUT -T restplugin.zip http://127.0.0.1:9089/ima/v1/file/restplugin.zip

다음으로, jsonmsg라는 프로토콜 플러그인을 작성하는 POST 메소드를 사용하여
플러그인을 작성해야 합니다. 다음 cURL을 사용하십시오.
    curl -X POST \
       -H 'Content-Type: application/json'  \
       -d  '{
               "Plugin": {
                "restmsg": {
                 "File": "restplugin.zip"
                }
             }
           }
     '  \
    http://127.0.0.1:9089/ima/v1/configuration/
   
마지막으로 플러그인을 시작하기 위해 IBM IoT MessageSight 프로토콜 플러그인 서버를
중지한 후 재시작해야 합니다. 

마지막으로 플러그인을 시작하기 위해 IBM IoT MessageSight 프로토콜 플러그인 서버를
재시작해야 합니다. 다음 cURL을 사용하십시오.
	curl -X POST -d  '{"Service":"Plugin"}' http://127.0.0.1:9089/ima/v1/service/restart

플러그인이 성공적으로 배치되었는지 확인하려면 다음 cURL을 사용하십시오.
    curl -X GET http://127.0.0.1:9089/ima/v1/configuration/Plugin

결과 목록에 restmsg가 표시됩니다. 이 값은 plugin.json 디스크립터 파일에
지정된 이름 특성에 해당합니다. 

새 프로토콜이 성공적으로 등록되었는지 확인하려면 다음 cURL을 사용하십시오.
    curl -X GET http://127.0.0.1:9089/ima/v1/configuration/Protocol

결과 목록에 restmsg가 표시됩니다. 이 값은 plugin.json 디스크립터 파일에
지정된 프로토콜 특성에 해당합니다. 

DemoEndpoint를 사용 중인 경우, 샘플 restmsg 프로토콜 플러그인에 액세스하기
위해 추가 단계가 필요하지 않습니다. 다른 엔드 포인트를 사용 중인 경우에는
엔드 포인트, 연결 정책 또는 메시징 정책에서 허용하는 프로토콜을 지정하기
위한 프로시저와 동일한 프로시저를 수행해야 합니다. DemoEndpoint 및 해당 정책은
정의된 모든 프로토콜을 허용하도록 사전 구성되어 있습니다. 

JavaScript 샘플 클라이언트 애플리케이션 실행:
=============================================
웹 브라우저에서 RESTMsgWebClient.html을 여십시오. 서버 텍스트 상자에
IBM IoT MessageSight의 호스트 이름 또는 IP 주소를 지정하십시오. DemoEndpoint를
사용 중인 경우 포트 텍스트 상자에 나열된 대로 포트 16102를 사용할 수 있습니다.
그렇지 않은 경우, 샘플 플러그인으로 구성된 IBM IoT MessageSight 엔드 포인트의
올바른 포트 번호를 지정하십시오. 추가 연결 옵션을 사용하려면 세션 탭을
선택하십시오. 연결 특성 설정을 완료한 후에는 연결 단추를 클릭하여
IBM IoT MessageSight에 연결하십시오. 그런 다음 토픽 탭을 사용하여 샘플
플러그인을 통해 IBM IoT MessageSight 목적지와 메시지를 송수신하십시오. 


Eclipse로 restmsg 클라이언트 가져오기
=====================================
Eclipse로 IBM IoT MessageSight restmsg 클라이언트를 가져오려면 다음 단계를
완료하십시오.
1. ProtocolPluginV2.0.zip의 컨텐츠를 압축 해제하십시오.
2. Eclipse에서 파일->가져오기->일반->기존 프로젝트를 작업공간으로를 선택하십시오.
3. 다음을 클릭하십시오.
4. "루트 디렉토리 선택" 단일 선택 단추를 선택하십시오.
5. 찾아보기를 클릭하십시오.
6. 압축 해제된 zip 파일 컨텐츠의 ImaTools/ImaPlugin/samples/restmsgClient
   서브디렉토리로 이동하여 선택하십시오.
7. "작업공간으로 프로젝트 복사" 선택란이 선택된 경우 이 선택란을 선택 취소하십시오.
8. 완료를 클릭하십시오. 

이 단계를 완료한 후에는 Eclipse에서 샘플 애플리케이션을 실행할 수 있습니다. 

Eclipse로 restmsg 플러그인 가져오기
===================================
Eclipse로 IBM IoT MessageSight restmsg 플러그인을 가져오려면 다음 단계를
완료하십시오.
1. ProtocolPluginV2.0.zip의 컨텐츠를 압축 해제하십시오.
2. Eclipse에서 파일->가져오기->일반->기존 프로젝트를 작업공간으로를 선택하십시오.
3. 다음을 클릭하십시오.
4. "루트 디렉토리 선택" 단일 선택 단추를 선택하십시오.
5. 찾아보기를 클릭하십시오.
6. 압축 해제된 zip 파일 컨텐츠의 ImaTools/ImaPlugin/samples/restmsgPlugin
   서브디렉토리로 이동하여 선택하십시오.
7. "작업공간으로 프로젝트 복사" 선택란이 선택된 경우 이 선택란을 선택 취소하십시오.
8. 완료를 클릭하십시오. 

이 단계를 완료한 후에는 Eclipse에서 샘플 플러그인 및 사용자 자신의
플러그인을 컴파일할 수 있습니다.

Eclipse에 restmsg 플러그인 아카이브 빌드
========================================
restmsg 샘플 플러그인을 업데이트하고 IBM IoT MessageSight에 배치할
아카이브를 작성하려는 경우 다음 단계를 따르십시오.
참고: 다음 단계에서는 Eclipse에 restmsgPlugin 프로젝트를 가져왔으며
Eclipse에서 프로젝트 소스 파일을 이미 컴파일했고 bin 서브디렉토리에
클래스 파일을 배치했다고 가정합니다.
1. Eclipse의 패키지 탐색기 또는 네비게이터에서 restmsgPlugin을 여십시오.
2. build_plugin_zip.xml을 마우스의 오른쪽 단추로 클릭하고 실행 도구 -> Ant 빌드를 선택하십시오.
   restmsgPlugin 아래에 새 restplugin.zip 파일이 포함된 플러그인 서브디렉토리가
   작성됩니다. restmsgPlugin 프로젝트를 새로 고쳐 Eclipse UI에 이 플러그인
   서브디렉토리를 표시할 수 있습니다.

플러그인 프로세스 모니터링
==========================

$SYS/ResourceStatistics/Plugin 토픽을 구독하여 플러그인 프로세스의 힙(heap)
크기, 가비지 콜렉션 비율 및 CPU 사용량을 모니터할 수 있습니다. 

제한사항 및 알려진 문제점
=========================
1. 프로토콜 플러그인 배치를 위한 MessageSight 웹 UI 지원이 없습니다. 명령행에서
   이를 수행해야 합니다.
2. 프로토콜 플러그인에 대해 공유 구독이 지원되지 않습니다.
3. IBM IoT MessageSight에서 ImaPluginListener.onAuthenticate() 메소드를 호출하지 않습니다.
4. 고가용성 구성이 지원되지 않습니다. 


주의사항
========
1. IBM, IBM 로고, ibm.com 및 MessageSight는 전세계 여러 국가에 등록된
International Business Machines Corp.의 상표 또는 등록상표입니다. 기타
제품 및 서비스 이름은 IBM 또는 타사의 상표입니다. 현재 IBM 상표 목록은
웹 "저작권 및 상표 정보"(http://www.ibm.com/legal/copytrade.shtml)에 있습니다.

2. Java 및 모든 Java 기반 상표와 로고는 Oracle 및/또는 해당 자회사의
상표 또는 등록상표입니다.
