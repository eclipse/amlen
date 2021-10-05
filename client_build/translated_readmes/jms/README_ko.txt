IBM® IoT MessageSight™ JMS 클라이언트 V2.0
2016년 6월

컨텐츠
1. 설명
2. IBM IoT MessageSight JMS 클라이언트 패키지 컨텐츠
3. 샘플 애플리케이션에 대한 작업
4. JMS 클라이언트를 Eclipse로 가져오기
5. 제한사항 및 알려진 문제점


설명
====
이 IBM IoT MessageSight JMS 클라이언트 패키지에 대한 이 README 파일에는 컨텐츠,
업데이트, 수정사항, 제한사항 및 알려진 문제점에 대한 정보가 포함되어 있습니다. 

IBM IoT MessageSight에 대한 자세한 정보는
IBM Knowledge Center(http://ibm.biz/iotms_v20)에 있는 제품 문서를
참조하십시오.

이 릴리스의 새로운 기능
=======================
이 릴리스에는 새로운 IBM IoT MessageSight JMS 클라이언트 기능이 없습니다. 

IBM IoT MESSAGESIGHT JMS 클라이언트 컨텐츠
==========================================
ImaJmsClientV2.0.zip 패키지는 JMS(Java™ Messaging Service) 및 IBM IoT MessageSight
자원 어댑터의 IBM IoT MessageSight 클라이언트 구현을 제공합니다. 이 패키지는
IBM IoT MessageSight에서 사용할 수 있도록 JMS 관리 오브젝트를 작성하고 구성하는 데
사용되는 클래스에 대한 문서도 제공합니다. 마지막으로 이 패키지에는 소스 컴포넌트 및
샘플 자료가 포함되어 있습니다. 

디렉토리 및 파일:
    ImaClient/
        README.txt - 이 파일

        license/ - IBM IoT MessageSight JMS 클라이언트 및 IBM
            IoT MessageSight 자원 어댑터 라이센스 계약 파일이 포함되어 있음

        jms/
            version.txt - IBM IoT MessageSight JMS 클라이언트에 대한 버전 정보가
                포함되어 있음

            .classpath 및 .project - jms 서브디렉토리를 Eclipe 프로젝트로
                가져올 수 있게 허용하는 Eclipse 프로젝트 구성 파일
            	
            doc/ - IBM IoT MessageSight를 위해 JMS 관리 오브젝트를 작성하고
                구성하는 데 필요한 문서가 포함되어 있음

            samples/ - 관리 오브젝트를 작성하는 방법, IBM IoT MessageSight를 통해
                메시지를 송수신하는 방법, 고가용성 구성에 사용할 수 있는
                클라이언트 애플리케이션을 작성하는 방법을 보여주는 샘플
                애플리케이션(소스 컴포넌트)이 포함되어 있음

            lib/
                imaclientjms.jar - JMS 인터페이스의 IBM IoT MessageSight 구현
                jms.jar - JMS 1.1 인터페이스용 JAR 파일
                jmssamples.jar - IBM IoT MessageSight JMS 클라이언트와 함께 제공되는
                    샘플 코드(소스 컴포넌트)에 대한 컴파일된 클래스(샘플 자료)
                fscontext.jar 및 providerutil.jar - 파일 시스템 JNDI 제공자를
                    구현하는 JAR 파일

         ImaResourceAdapter/
            version.txt - IBM IoT MessageSight 자원 어댑터에 대한 버전 정보가
                포함되어 있음

            ima.jmsra.rar - IBM IoT MessageSight 자원 어댑터 아카이브

샘플 애플리케이션에 대한 작업
=============================
클라이언트와 함께 네 가지 샘플 애플리케이션이 제공됩니다. 이 샘플 애플리케이션은
다음과 같습니다.
    JMSSampleAdmin
    JMSSample
    HATopicPublisher
    HADurableSubscriber

JMSSampleAdmin 애플리케이션은 IBM IoT MessageSight를 위해 JMS 관리 오브젝트를
작성하고 구성하고 저장하는 방법을 보여줍니다. 이 애플리케이션은 입력 파일에서
IBM IoT MessageSight JMS 관리 오브젝트 구성을 읽어와서 JNDI 저장소를 채웁니다.
이 애플리케이션은 LDAP 또는 파일 시스템 JNDI 저장소를 채울 수 있습니다. 

JMSSample 애플리케이션은 IBM IoT MessageSight 토픽 및 큐와 메시지를 송수신하는
방법을 보여줍니다. 이 애플리케이션은 세 가지 클래스(JMSSample, JMSSampleSend,
JMSSampleReceive)에서 구현됩니다. JMSSample 애플리케이션은 JNDI 저장소에서
관리 오브젝트를 검색하거나 런타임 시 필수 관리 오브젝트를 작성하고 구성할 수
있습니다. 

HATopicPublisher 및 HADurableSubscriber는 JMS 클라이언트 애플리케이션이
IBM IoT MessageSight 고가용성 기능을 사용할 수 있게 하는 방법을 보여줍니다. 

클라이언트와 함께 제공되는 샘플 애플리케이션을 실행하려면 클래스 경로에
세 가지 JAR 파일(imaclientjms.jar, jms.jar, jmssamples.jar)이 필요합니다.
세 가지 파일은 모두 ImaClient/jms/lib에 있습니다. JMSSampleAdmin 및
JMSSample 애플리케이션 실행 시 파일 시스템 JNDI 제공자를 사용하려면
클래스 경로에 두 개의 추가적인 JAR 파일(fscontext.jar, providerutil.jar)이
필요합니다. 이 두 JAR 파일은 모두 ImaClient/jms/lib에 있습니다. 

Linux에서 컴파일된 샘플 실행:
=============================
ImaClient/jms에서 실행하여 다음 방식으로 CLASSPATH 환경 변수를
설정하십시오.
    lib/imaclientjms.jar:lib/jms.jar:lib/jmssamples.jar

또는 파일 시스템 JNDI 제공자를 사용하려면 CLASSPATH를 다음으로 설정하십시오.
    lib/imaclientjms.jar:lib/jms.jar:lib/jmssamples.jar:lib/fscontext.jar:lib/providerutil.jar

ImaClient/jms에서 실행하여 다음의 예제 명령행이 각각의 애플리케이션을
실행하고 각각에 대한 사용법 명령문을 제공합니다. 

    java -classpath $CLASSPATH com.ibm.ima.samples.jms.JMSSampleAdmin
    java -classpath $CLASSPATH com.ibm.ima.samples.jms.JMSSample
    java -classpath $CLASSPATH com.ibm.ima.samples.jms.HATopicPublisher
    java -classpath $CLASSPATH com.ibm.ima.samples.jms.HADurableSubscriber

Windows에서 컴파일된 샘플 실행:
===============================
ImaClient/jms에서 실행하여 다음 방식으로 CLASSPATH 환경 변수를
설정하십시오.
    lib\imaclientjms.jar;lib\jms.jar;lib\jmssamples.jar

또는 파일 시스템 JNDI 제공자를 사용하려면 CLASSPATH를 다음으로 설정하십시오.
    lib\imaclientjms.jar;lib\jms.jar;lib\jmssamples.jar;lib\fscontext.jar;lib\providerutil.jar 

ImaClient/jms에서 실행하여 다음의 예제 명령행이 각각의 애플리케이션을
실행하고 각각에 대한 사용법 명령문을 제공합니다. 

    java -classpath %CLASSPATH% com.ibm.ima.samples.jms.JMSSampleAdmin
    java -classpath %CLASSPATH% com.ibm.ima.samples.jms.JMSSample
    java -classpath %CLASSPATH% com.ibm.ima.samples.jms.HATopicPublisher
    java -classpath %CLASSPATH% com.ibm.ima.samples.jms.HADurableSubscriber
    
샘플 애플리케이션을 로컬로 빌드하려면 빌드 클래스 경로에 jms.jar 및 imaclientjms.jar만
필요합니다. 


JMS 클라이언트를 ECLIPSE로 가져오기
===================================
IBM IoT MessageSight JMS 클라이언트를 Eclipse로 가져오려면 다음의 단계를
완료하십시오.
1. ImaJmsClientV2.0.zip의 컨텐츠 압축 풀기
2. Eclipse에서 파일->가져오기->일반->기본 프로젝트를 작업공간으로 선택
3. 다음 클릭
4. "루트 디렉토리 선택" 단일 선택 단추 선택
5. 찾아보기 클릭
6. 압축 해제된 zip 파일 컨텐츠의 jms 서브디렉토리로 이동하여 선택
7. "프로젝트를 작업공간으로 복사"의 선택란이 선택된 경우 해당 선택 표시 제거
8. 마침 클릭

이 단계를 완료하고 나면 Eclipse에서 샘플 애플리케이션 및 자체 클라이언트
애플리케이션을 컴파일하고 실행할 수 있습니다. 


제한사항 및 알려진 문제점
=========================
1. IBM IoT MessageSight JMS 구현에서는 수신되는 순서대로 메시지를 전달합니다.
메시지 우선순위 설정은 전달 순서에 영향을 미치지 않습니다. 

2. IBM IoT MessageSight 자원 어댑터는 z/OS®를 제외한 모든 플랫폼의 WebSphere
Application Server 버전 8.0 이상에서 지원됩니다. 

주의사항
========
1. IBM, IBM 로고, ibm.com 및 MessageSight는 전세계 여러 국가에 등록된
International Business Machines Corp.의 상표 또는 등록상표입니다.
Linux는 미국 또는 기타 국가에서 사용되는 Linus Torvalds의 등록상표입니다.
기타 제품 및 서비스 이름은 IBM 또는 타사의 상표입니다. 현재 IBM 상표 목록은
웹 "저작권 및 상표 정보"(http://www.ibm.com/legal/copytrade.shtml)에 있습니다.

2. Java 및 모든 Java 기반 상표와 로고는 Oracle 및/또는 그 계열사의 상표
또는 등록상표입니다.
	
