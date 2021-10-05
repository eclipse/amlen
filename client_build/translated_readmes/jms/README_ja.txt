IBM® IoT MessageSight™ JMS Client V2.0
2016 年 6 月

内容
1. 概要
2. IBM IoT MessageSight JMS Client パッケージの内容
3. サンプル・アプリケーションの使用
4. JMS Client の Eclipse へのインポート
5. 制限および既知の問題


概要
====
IBM IoT MessageSight JMS Client パッケージのこの README ファイルには、
内容、更新、修正、制限、および既知の問題に関する情報が含まれています。

IBM IoT MessageSight について詳しくは、IBM Knowledge Center にある製品資料を
参照してください。アドレスは次のとおりです。
http://ibm.biz/iotms_v20


このリリースでの新機能
======================
このリリースでは新しい IBM IoT MessageSight JMS Client 機能はありません。

IBM IoT MessageSight JMS Client の内容
===========================================
ImaJmsClientV2.0.zip パッケージは、Java™ Messaging Service (JMS) および
IBM IoT MessageSight Resource Adapter の IBM IoT MessageSight クライアント実装を
提供します。この中には、IBM IoT MessageSight で使用する JMS 管理対象オブジェクトを
作成および構成するために使用されるクラスのドキュメンテーションも含まれています。
さらに、サンプルのソース・コンポーネントとサンプル本体も含まれています。

ディレクトリーとファイル:
    ImaClient/
        README.txt - このファイル

        license/ - IBM IoT MessageSight JMS Client および IBM IoT MessageSight
            Resource Adapter の使用許諾契約ファイルが含まれています。

        jms/
            version.txt - IBM IoT MessageSight JMS Client のバージョン情報が
                含まれています。

            .classpath および .project - Eclipse プロジェクトとして
            	   インポートされる jms サブディレクトリーを許可する
                Eclipse プロジェクト構成ファイル
	
            doc/ - IBM IoT MessageSight の JMS 管理対象オブジェクトの作成
                および構成のためのドキュメンテーションが含まれています。

            samples/ - サンプル・アプリケーション (ソース・コンポーネント)
                    が含まれています。それらは、管理対象オブジェクトの作成方法、
                    IBM IoT MessageSight を通じてメッセージを送受信する方法、
                    および高可用性構成対応のクライアント・アプリケーションを
                    作成する方法を示すデモです。

            lib/
                imaclientjms.jar - JMS インターフェースの
                    IBM IoT MessageSight 実装
                jms.jar - JMS 1.1 インターフェース用 JAR ファイル
                jmssamples.jar - IBM IoT MessageSight JMS Client で
                    提供されているサンプル・コード (ソース・コンポーネント)
                    のコンパイル済みクラス (サンプル本体)
                fscontext.jar および providerutil.jar - ファイル・システム
                    JNDI プロバイダーを実装する JAR ファイル

         ImaResourceAdapter/
            version.txt - IBM IoT MessageSight Resource Adapter の
                バージョン情報が含まれています。

            ima.jmsra.rar - IBM IoT MessageSight Resource Adapter アーカイブ

サンプル・アプリケーションの作業
================================
クライアントには、4 つのサンプル・アプリケーションが提供されています。
それらのサンプル・アプリケーションは、
以下のとおりです。
    JMSSampleAdmin
    JMSSample
    HATopicPublisher
    HADurableSubscriber

JMSSampleAdmin アプリケーションは、IBM IoT MessageSight の JMS 管理対象オブジェクト
を作成、構成、および保管する方法を示します。このアプリケーションは、入力ファイルから
IBM IoT MessageSight JMS 管理対象オブジェクトの構成を読み取り、JNDI リポジトリーの
データを設定します。このアプリケーションにより、LDAP またはファイル・システム JNDI
リポジトリーのいずれかのデータを設定できます。

JMSSample アプリケーションは、IBM IoT MessageSight のトピックとキューの間で
メッセージを送受信する方法を示します。このアプリケーションは、JMSSample、
JMSSampleSend、および JMSSampleReceive という 3 つのクラスによって実装
されています。JMSSample アプリケーションを利用することにより、
JNDI リポジトリーから管理対象オブジェクトを取得したり、必要な
管理対象オブジェクトを実行時に作成および構成したりすることができます。

HATopicPublisher および HADurableSubscriber は、JMS クライアント・
アプリケーションで IBM IoT MessageSight 高可用性フィーチャーを
使用できるようにする方法を示します。

クライアントに提供されているサンプル・アプリケーションを実行するには、
クラスパスの中に imaclientjms.jar、jms.jar、および jmssamples.jar の
3 つの JAR ファイルが必要です。これら 3 つのファイルはすべて ImaClient/jms/lib
の中に入っています。JMSSampleAdmin および JMSSample の各アプリケーション
を実行する際にファイル・システム JNDI プロバイダーを使用するには、
クラスパスの中にさらに 2 つの JAR ファイル fscontext.jar および
providerutil.jar が必要です。それらの JAR ファイルは、いずれも
ImaClient/jms/lib の中にあります。

コンパイル済みサンプルの実行 (Linux):
======================================
ImaClient/jms から実行し、CLASSPATH 環境変数を以下のように
設定します:
    lib/imaclientjms.jar:lib/jms.jar:lib/jmssamples.jar

あるいは、ファイル・システム JNDI プロバイダーを使用する場合、
CLASSPATH は以下のように設定します。
    lib/imaclientjms.jar:lib/jms.jar:lib/jmssamples.jar:lib/fscontext.jar:lib/providerutil.jar

ImaClient/jms から実行します。以下のコマンド・ラインの例では、各アプリケーションを実行し、
それぞれの使用方法を表示します。

    java -classpath $CLASSPATH com.ibm.ima.samples.jms.JMSSampleAdmin
    java -classpath $CLASSPATH com.ibm.ima.samples.jms.JMSSample
    java -classpath $CLASSPATH com.ibm.ima.samples.jms.HATopicPublisher
    java -classpath $CLASSPATH com.ibm.ima.samples.jms.HADurableSubscriber

コンパイル済みサンプルの実行 (Windows):
========================================
ImaClient/jms から実行し、CLASSPATH 環境変数を以下のように
設定します:
    lib\imaclientjms.jar;lib\jms.jar;lib\jmssamples.jar

あるいは、ファイル・システム JNDI プロバイダーを使用する場合、
CLASSPATH は以下のように設定します。
    lib\imaclientjms.jar;lib\jms.jar;lib\jmssamples.jar;lib\fscontext.jar;lib\providerutil.jar

ImaClient/jms から実行します。以下のコマンド・ラインの例では、各アプリケーションを実行し、
それぞれの使用方法を表示します。

    java -classpath %CLASSPATH% com.ibm.ima.samples.jms.JMSSampleAdmin
    java -classpath %CLASSPATH% com.ibm.ima.samples.jms.JMSSample
    java -classpath %CLASSPATH% com.ibm.ima.samples.jms.HATopicPublisher
    java -classpath %CLASSPATH% com.ibm.ima.samples.jms.HADurableSubscriber

サンプル・アプリケーションをローカルにビルドする場合、クラスパス中に
必要なのは jms.jar と imaclientjms.jar のみです。


JMS Client の Eclipse へのインポート
====================================
IBM IoT MessageSight JMS Client を Eclipse にインポートするには、
以下の手順を実行します。
1. ImaJmsClientV2.0.zip の内容を unzip します。
2. Eclipse で、「ファイル」-> 「インポート」-> 「一般」->
   「既存プロジェクトをワークスペースへ」を選択します。
3. 「次へ」をクリックします。
4. 「ルート・ディレクトリーの選択」ラジオ・ボタンを選択します。
5. 「参照」をクリックします。
6. 解凍した zip ファイルの内容の中で、jms サブディレクトリーまでナビゲートして
   選択します。
7. 「プロジェクトをワークスペースにコピー」のチェック・ボックスが選択されている
   場合は、チェックを外します。
8. 「完了」をクリックします。

このステップを完了すると、サンプル・アプリケーションおよび自分で作成した
クライアント・アプリケーションを Eclipse からコンパイルおよび実行できる
ようになります。


制限および既知の問題
====================
1. IBM IoT MessageSight JMS 実装では、メッセージは受信された順序で配信されます。
メッセージ優先順位の設定値は、送信の順序に
影響しません。

2. IBM IoT MessageSight Resource Adapter は、z/OS® を除くすべてのプラットフォーム
において、WebSphere Application Server バージョン 8.0 以降でサポートされます。

商標
=======
1. IBM、IBM ロゴ、ibm.com および MessageSight は、世界の多くの国で登録された
International Business Machines Corporation の商標です。
Linux は、Linus Torvalds の米国およびその他の国における商標です。
他の会社名、製品名およびサービス名等はそれぞれ各社の商標です。
現時点での IBM の商標リストについては、
www.ibm.com/legal/copytrade.shtml をご覧ください。

2. Java およびすべての Java 関連の商標およびロゴは Oracle やその関連会社の米国およびその他の
国における商標または登録商標です。
