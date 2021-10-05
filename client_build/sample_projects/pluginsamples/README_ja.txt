IBM® IoT MessageSight™ プロトコル・プラグイン SDK V2.0 2016 年 6 月

目次
1. 説明
2. IBM IoT MessageSight プロトコル・プラグイン SDK の内容
3. サンプル JSON プロトコル・プラグインのサンプル・コードを使用した作業
4. サンプル JSON プロトコル・プラグイン SDK およびサンプルの Eclipse へのインポート
5. サンプル REST プロトコル・プラグインのサンプル・コードを使用した作業
6. サンプル REST プロトコル・プラグイン SDK およびサンプルの Eclipse へのインポート
7. 制限および既知の問題


説明
====
IBM IoT MessageSight プロトコル・プラグイン SDK に関するこの README ファイルには、
内容、更新、修正、制限、および既知の問題に関する情報が記載されています。この SDK を使用して、
Java プログラミング言語でプラグインを作成することで、IBM IoT MessageSight サーバーがネイティブ・サポートしている一連のプロトコルを拡張できます。

IBM IoT MessageSight について詳しくは、 IBM Knowledge Center で製品資料を参照してください
(http://ibm.biz/iotms_v20)。

このリリースの新機能
===================
IBM IoT MessageSight プロトコル・プラグイン SDK は、非 HA 環境で完全サポートされるようになりました。
使用制限については、付属のライセンスを参照してください。
プラグインのデプロイメントおよび構成は、RESTful API を通じて実行するようになりました。

IBM IoT MESSAGESIGHT プロトコル・プラグイン SDK の内容
==================================================
IBM IoT MessageSight プロトコル・プラグイン SDK は、
ProtocolPluginV2.0.zip パッケージ内の ImaTools/ImaPlugin
サブディレクトリー内にあります。また、IBM IoT MessageSight
で使用するプロトコル・プラグインの作成に使用するクラス
の資料もこのサブディレクトリー内にあります。さらに、サンプルのソース・コンポーネントとサンプル本体も含まれています。

ディレクトリーおよびファイル:
    ImaTools/
        license/ - IBM IoT MessageSight プロトコル・プラグイン・フィーチャー
            の使用許諾契約書ファイルが含まれています。
        ImaPlugin/
            README.txt - このファイルです。

            version.txt - IBM IoT MessageSight プロトコル・プラグイン SDK の
                バージョン情報が含まれています。

            doc/ - IBM IoT MessageSight プロトコル・プラグイン SDK を使用して
                プロトコル・プラグインを作成するための資料が含まれています。

            lib/
                imaPlugin.jar - IBM IoT MessageSight プロトコル・プラグイン SDK。

                jsonplugin.zip - IBM IoT MessageSight へのデプロイが可能なサンプル
                    JSON プロトコル・プラグインを含む zip アーカイブ。
                    このアーカイブには、samples/jsonmsgPlugin サブディレクトリー
                    内で提供されているサンプル・プラグイン・コード
                    (ソース・コンポーネント) のコンパイル済みクラス
　　　　　　　　　　(サンプル本体) が含まれています。

                jsonprotocol.jar - サンプル・プラグインのコンパイル済みクラス
                    (サンプル本体)。この JAR ファイルは、jsonplugin.zip
                    ファイルにも含まれています。

                restplugin.zip - IBM IoT MessageSight へのデプロイが可能な
                    サンプル REST プロトコル・プラグインを含む zip アーカイブ。
                    このアーカイブには、samples/restmsgPlugin サブディレクトリー
                    内で提供されているサンプル・プラグイン・コード
                    (ソース・コンポーネント) のコンパイル済みクラス
                    (サンプル本体) が含まれています。

                restprotocol.jar - サンプル REST プロトコル・プラグインの
                    コンパイル済みクラス (サンプル本体)。この JAR ファイルは、
                    restplugin.zip ファイルにも含まれています。

            samples/
                jsonmsgPlugin/
                    .classpath および .project - jsonmsgPlugin サブディレクトリーを
            	       Eclipse プロジェクトとしてインポートすることを可能にする
            	       Eclipse プロジェクト構成ファイル。
            	
            	    build_plugin_zip.xml - プラグイン・アーカイブを作成するために
            	        使用できる Ant ビルド・ファイル。
            	
                    src/ - IBM IoT MessageSight で使用するプロトコル・プラグイン
                        の作成方法を示すサンプル・プラグイン (ソース・コンポーネント)
                        の Java ソース・コードが含まれています。

                    config/ - コンパイル済みサンプル・プラグインを
                        IBM IoT MessageSight にデプロイするために必要な
                        JSON 記述子ファイル (ソース・コンポーネント)
                        が含まれています。

                    doc/ - サンプル・プラグイン・ソース・コードの資料が
                        含まれています。

                jsonmsgClient/
                    .classpath および .project - jsonmsgClient サブディレクトリー
            	       を Eclipse プロジェクトとしてインポートすることを可能にする
                    Eclipse プロジェクト構成ファイル。
            	
            	    JSONMsgWebClient.html - jsonmsgPlugin で提供されている
            	        サンプル・プラグインのクライアント・アプリケーション
            	        (ソース・コンポーネント)。このクライアントは、js
            	        サブディレクトリー内の JavaScript™ ライブラリーに依存します。
            	
                    js/ - jsonmsgPlugin で提供されているサンプル・プラグインで
                        使用するためのサンプル JavaScript クライアント・ライブラリー
                        (ソース・コンポーネント) が含まれています。

                    css/ - サンプル・クライアント・アプリケーションおよび
                        サンプル・クライアント・ライブラリーによって使用される
                        スタイル・シート (ソース・コンポーネント) と
                        イメージ・ファイルが含まれています。

                    doc/ - サンプル JavaScript クライアント・ライブラリーの
                        資料が含まれています。

                restmsgPlugin/
                    .classpath および .project - restmsgPlugin サブディレクトリーを
                        Eclipse プロジェクトとしてインポートすることを可能にする
            	           Eclipse プロジェクト構成ファイル。

                    build_plugin_zip.xml - プラグイン・アーカイブを作成するために
                        使用できる Ant ビルド・ファイル。

                    src/ - IBM IoT MessageSight で使用するプロトコル・プラグイン
                        の作成方法を示すサンプル・プラグイン (ソース・コンポーネント)
                        の Java ソース・コードが含まれています。

                    config/ - コンパイル済みサンプル・プラグインを
                        IBM IoT MessageSight にデプロイするために必要な
                        JSON 記述子ファイル (ソース・コンポーネント)
                        が含まれています。

                    doc/ - サンプル・プラグイン・ソース・コードの資料が
                        含まれています。

                restmsgClient/
                    .classpath および .project - restmsgClient サブディレクトリー
                        を Eclipse プロジェクトとしてインポートすることを可能にする
                        Eclipse プロジェクト構成ファイル。

                    RESTMsgWebClient.html - restmsgPlugin で提供されている
                        サンプル・プラグインのクライアント・アプリケーション
                        (ソース・コンポーネント)。このクライアントは、
                        js サブディレクトリー内の JavaScript ライブラリー
                        に依存します。

                    js/ - restmsgPlugin で提供されているサンプル・プラグイン
                        で使用するためのサンプル JavaScript クライアント・
                        ライブラリー (ソース・コンポーネント) が含まれています。

                    css/ - サンプル・クライアント・アプリケーションおよび
                        サンプル・クライアント・ライブラリーによって使用される
                        スタイル・シート (ソース・コンポーネント) と
                        イメージ・ファイルが含まれています。

                    doc/ - サンプル JavaScript クライアント・ライブラリーの
                        資料が含まれています。

サンプル JSON プロトコル・プラグインおよびクライアント・アプリケーションを使用した作業
====================================================================
JSONMsgWebClient.html アプリケーションは、json_msg プラグインと通信するために
jsonmsg.js JavaScript クライアント・ライブラリーを使用します。JSONMsgWebClient.html は、
json_msg プラグインに対して QoS 0 (最大 1 回) メッセージを送受信するための Web インターフェースを提供しています。

json_msg プロトコル・プラグイン・サンプルは、JMPlugin と
JMConnection という、2 つのクラスで構成されています。JMPlugin は、
プロトコル・プラグインの初期化と起動に必要な ImaPluginListener
インターフェースを実装します。JMConnection は、クライアントが
IBM IoT MessageSight に接続し、json_message プロトコルを介してメッセージ
を送受信することを可能にする ImaConnectionListener インターフェースを
実装しています。
JMConnection クラスは、パブリッシュ・クライアントから JSON オブジェクトを
受信し、ImaJson ユーティリティー・クラスを使用してそれらのオブジェクトを
解析してコマンドとメッセージに変換し、それらを IBM IoT MessageSight サーバーに送信します。
同様に、JMConnection クラスは、json_msg クライアントをサブスクライブするために、
ImaJson ユーティリティー・クラスを使用して、IBM IoT MessageSight メッセージを
JSON オブジェクトに変換します。最後に、json_msg プロトコル・プラグインに、
plugin.json という名前の記述子ファイルが組み込まれます。この記述子ファイルは、
プロトコル・プラグインをデプロイするために使用される zip アーカイブ内に
必要であり、常にこのファイル名を使用する必要があります。json_msg
プロトコルのサンプル・プラグイン・アーカイブ jsonplugin.zip は、
ImaTools/ImaPlugin/lib ディレクトリーに含まれています。これには、
JAR ファイル (コンパイル済み JMPlugin クラス と JMConnection クラスを含む
jsonprotocol.jar) と plugin.json 記述子ファイルが含まれています。

サンプル JavaScript クライアント・アプリケーションを実行するには、
最初にサンプル・プラグインを IBM IoT MessageSight にデプロイする必要があります。
このプラグインは、zip ファイルまたは JAR ファイル内にアーカイブされている
必要があります。このファイルには、ターゲット・プロトコルのプラグイン
を実装する、1 つ以上の JAR ファイルが含まれている必要があります。また、
プラグインの内容を記述する JSON 記述子ファイルも含まれている必要があります。
jsonplugin.zip という名前のアーカイブが ImaTools/ImaPlugin/lib 内にあり、
json_msg サンプル・プラグインを含んでいます。サンプル・プラグイン・アーカイブ
がデプロイされたら、(ImaTools/ImaPlugin/samples/jsonmsgClient 内の)
JSONMsgWebClient.html を Web ブラウザーにロードすることにより、
サンプル・クライアント・アプリケーションを実行できます。
サンプル・アプリケーション用のサンプル JavaScript クライアント・ライブラリー
およびスタイル・シートをロードするためには、JSONMsgWebClient.html が
含まれているサブディレクトリー構造を維持する必要があります。


IBM IoT MessageSight へのサンプル・プラグイン・アーカイブのデプロイ
=============================================================
jsonplugin.zip を IBM IoT MessageSight にデプロイするには、
PUT メソッドを使用してプラグイン zip ファイル jsonmsg.zip をインポートする必要があります。
このために、次の cURL を使用します: curl -X PUT -T jsonmsg.zip http://127.0.0.1:9089/ima/v1/file/jsonmsg.zip

次に、POST メソッドを使用して、jsonmsg という名前のプロトコル・プラグインを作成する必要があります。
このために、次の cURL を使用します: curl -X POST \ -H 'Content-Type: application/json'  \
       -d  '{
               "Plugin": {
                "jsonmsg": {
                 "File": "jsonmsg.zip"
                }
             }
           }
     '  \
    http://127.0.0.1:9089/ima/v1/configuration/

最後に、プラグインを起動するために、IBM IoT MessageSight プロトコル・プラグイン・サーバーを
再始動する必要があります。このために、次の cURL を使用します:
	curl -X POST -d  '{"Service":"Plugin"}' http://127.0.0.1:9089/ima/v1/service/restart

プラグインが正常にデプロイされたことを確認します。このために、次の cURL を使用します:
    curl -X GET http://127.0.0.1:9089/ima/v1/configuration/Plugin

結果リストに json_msg が表示されます。この値は、plugin.json 記述子ファイル
に指定されている Name プロパティーに対応します。

新しいプロトコルが正常に登録されたことを確認します。このために、次の cURL を使用します:
    curl -X GET http://127.0.0.1:9089/ima/v1/configuration/Protocol

結果リストに json_msg が表示されます。この値は、plugin.json 記述子ファイル
に指定されている Protocol プロパティーに対応します。

DemoEndpoint を使用している場合、サンプル json_msg プロトコル・プラグイン
にアクセスするために、これ以上のステップは必要はありません。別のエンドポイント
を使用している場合は、同じ手順に従って、どのプロトコルが
エンドポイント、接続ポリシー、またはメッセージング・ポリシーによって
許可されるかを指定する必要があります。DemoEndpoint とそのポリシーは、
定義されているすべてのプロトコル許可するように、事前に構成されています。

JavaScript サンプル・クライアント・アプリケーションの実行
=========================================================
Web ブラウザーで JSONMsgWebClient.html を開きます。「Server」テキスト・ボックス
に、IBM IoT MessageSight のホスト名または IP アドレスを指定します。
DemoEndpoint を使用している場合は、「Port」テキスト・ボックスにリストされている
ポート 16102 を使用できます。それ以外の場合、サンプル・プラグイン
が構成されている IBM IoT MessageSight エンドポイントの正しいポート番号
を指定してください。追加の接続オプションについては、「Session」タブ
を選択します。接続プロパティーの設定が終了したら、「Connect」ボタンを
クリックして IBM IoT MessageSight に接続します。次に、「Topic」タブを使用して、
サンプル・プラグインを介した IBM IoT MessageSight 宛先とのメッセージの
送受信を実行します。


ECLIPSE への JSON_MSG クライアントのインポート
==============================================
IBM IoT MessageSight json_msg クライアントを Eclipse にインポートするには、
以下のステップを実行します。
1. ProtocolPluginV2.0.zip の内容を unzip します。
2. Eclipse で、「ファイル」->「インポート」->「一般」->
  「既存のプロジェクトをワークスペースへ」を選択します。
3. 「次へ」をクリックします。
4. 「ルート・ディレクトリーの選択」ラジオ・ボタンを選択します。
5. 「参照」をクリックします。
6. 解凍した zip ファイル内容の中の ImaTools/ImaPlugin/samples/jsonmsgClient
   サブディレクトリーにナビゲートし、選択します。
7. 「プロジェクトをワークスペースにコピー」のチェック・ボックス
   が選択されている場合は、選択解除します。
8. 「終了」をクリックします。

これらのステップを完了すると、Eclipse からサンプル・アプリケーションを
実行できるようになります。

ECLIPSE への JSON_MSG プラグインの インポート
============================================
IBM IoT MessageSight json_msg プラグインを Eclipse にインポートするには、
以下のステップを実行します。
1. ProtocolPluginV2.0.zip の内容を unzip します。
2. Eclipse で、「ファイル」->「インポート」->「一般」->
  「既存のプロジェクトをワークスペースへ」を選択します。
3. 「次へ」をクリックします。
4. 「ルート・ディレクトリーの選択」ラジオ・ボタンを選択します。
5. 「参照」をクリックします。
6. 解凍した zip ファイル内容の中の ImaTools/ImaPlugin/samples/jsonmsgPlugin
   サブディレクトリーにナビゲートし、選択します。
7. 「プロジェクトをワークスペースにコピー」のチェック・ボックス
   が選択されている場合は、選択解除します。
8. 「終了」をクリックします。

これらのステップを完了すると、サンプル・プラグインおよび自分で作成した
プラグインを Eclipse からコンパイルできるようになります。

ECLIPSE でのプラグイン・アーカイブのビルド
==========================================
json_msg サンプル・プラグインを更新し、IBM IoT MessageSight にデプロイする
アーカイブを作成する場合は、以下のステップを実行してください。
注: これらのステップは、jsonmsgPlugin プロジェクトを Eclipse に
インポートしていること、および Eclipse で既にプロジェクト・ソース・ファイルが
コンパイルされ、クラス・ファイルが bin サブディレクトリーに
配置されていることを前提としています。
1. Eclipse で、jsonmsgPlugin をパッケージ・エクスプローラー
   またはナビゲーターで開きます。
2. build_plugin_zip.xml を右クリックし、「実行」->「Ant ビルド」を選択します。
   jsonmsgPlugin の下に plugin サブディレクトリーが作成され、新しい
   jsonplugin.zip ファイルが含まれています。jsonmsgPlugin プロジェクトを
   最新表示すると、Eclipse UI 内の plugin サブディレクトリーを確認できます。


サンプル REST プロトコル・プラグインおよびクライアント・アプリケーションを使用した作業
====================================================================
RESTMsgWebClient.html アプリケーションは、restmsg プラグインと通信するために、
restmsg.js JavaScript クライアント・ライブラリーを使用します。RESTMsgWebClient.html は、
restmsg プラグインに対して QoS 0 (最大 1 回) メッセージを送受信するための Web インターフェースを提供しています。

restmsg プロトコル・プラグイン・サンプルは、RestMsgPlugin と
RestMsgConnection という、2 つのクラスで構成されています。
RestMsgPlugin は、プロトコル・プラグインの初期化と起動に必要な
ImaPluginListener インターフェースを実装します。RestMsgConnection は、
クライアントが IBM IoT MessageSight に接続し、restmsg protocol を介してメッセージ
を送受信することを可能にする ImaConnectionListener インターフェースを
実装しています。RestMsgConnection クラスは、パブリッシュ・クライアントから
REST オブジェクトを受信し、それらのオブジェクトを解析してコマンドとメッセージに
変換し、それらを IBM IoT MessageSight サーバーに送信します。同様に、
RestMsgConnection クラスは、restmsg クライアントをサブスクライブするために
IBM IoT MessageSight メッセージを REST オブジェクトに変換します。最後に、
restmsg プロトコル・プラグインは、plugin.json という名前の
記述子ファイルが組み込まれます。この記述子ファイルは、
プロトコル・プラグインをデプロイするために使用される zip アーカイブ内に
必要であり、常にこのファイル名を使用する必要があります。restmsg
プロトコルのサンプル・プラグイン・アーカイブ restplugin.zip は、
ImaTools/ImaPlugin/lib ディレクトリーに含まれています。これには、
JAR ファイル (コンパイル済みの RestMsgPlugin クラスと RestMsgConnection クラスを含む
restprotocol.jar) と plugin.json 記述子ファイルが含まれています。

サンプル JavaScript クライアント・アプリケーションを実行するには、
最初にサンプル・プラグインを IBM IoT MessageSight にデプロイする必要があります。
このプラグインは、zip ファイルまたは JAR ファイル内にアーカイブされている
必要があります。このファイルには、ターゲット・プロトコルのプラグイン
を実装する、1 つ以上の JAR ファイルが含まれている必要があります。また、
プラグインの内容を記述する JSON 記述子ファイルも含まれている必要があります。
restplugin.zip という名前のアーカイブが ImaTools/ImaPlugin/lib にあり、
restmsg サンプル・プラグインを含んでいます。
サンプル restmsg プラグイン・アーカイブがデプロイされたら、
(ImaTools/ImaPlugin/samples/restmsgClient 内の) RESTMsgWebClient.html を
Web ブラウザーにロードすることにより、サンプル・クライアント・アプリケーション
を実行できます。サンプル・アプリケーション用のサンプル JavaScript
クライアント・ライブラリーおよびスタイル・シートをロードするためには、
RESTMsgWebClient.html が含まれているサブディレクトリー構造を
維持する必要があります。


IBM IoT MessageSight への REST サンプル・プラグイン・アーカイブのデプロイ
=============================================================
restplugin.zip を IBM IoT MessageSight にデプロイするには、
PUT メソッドを使用してプラグイン zip ファイル jsonmsg.zip をインポートする必要があります。
このために、次の cURL を使用します: curl -X PUT -T restplugin.zip http://127.0.0.1:9089/ima/v1/file/restplugin.zip

次に、POST メソッドを使用して、jsonmsg という名前のプロトコル・プラグインを作成する必要があります。
このために、次の cURL を使用します: curl -X POST \ -H 'Content-Type: application/json'  \
       -d  '{
               "Plugin": {
                "restmsg": {
                 "File": "restplugin.zip"
                }
             }
           }
     '  \
    http://127.0.0.1:9089/ima/v1/configuration/
   
最後に、プラグインを起動するために、IBM IoT MessageSight プロトコル・プラグイン・サーバーを停止し、
再始動する必要があります。

最後に、プラグインを起動するために、IBM IoT MessageSight プロトコル・プラグイン・サーバーを
再始動する必要があります。このために、次の cURL を使用します:
	curl -X POST -d  '{"Service":"Plugin"}' http://127.0.0.1:9089/ima/v1/service/restart

プラグインが正常にデプロイされたことを確認します。このために、次の cURL を使用します:
    curl -X GET http://127.0.0.1:9089/ima/v1/configuration/Plugin

結果リストに restmsg が表示されます。この値は、plugin.json 記述子ファイル
に指定されている Name プロパティーに対応します。

新しいプロトコルが正常に登録されたことを確認します。このために、次の cURL を使用します:
    curl -X GET http://127.0.0.1:9089/ima/v1/configuration/Protocol

結果リストに restmsg が表示されます。この値は、plugin.json 記述子ファイル
に指定されている Protocol プロパティーに対応します。

DemoEndpoint を使用している場合、サンプル restmsg プロトコル・プラグイン
にアクセスするために、これ以上のステップは必要はありません。別のエンドポイント
を使用している場合は、同じ手順に従って、どのプロトコルが
エンドポイント、接続ポリシー、またはメッセージング・ポリシーによって
許可されるかを指定する必要があります。DemoEndpoint とそのポリシーは、
定義されているすべてのプロトコル許可するように、事前に構成されています。

JavaScript サンプル・クライアント・アプリケーションの実行
=========================================================
Web ブラウザーで RESTMsgWebClient.html を開きます。「Server」テキスト・ボックス
に、IBM IoT MessageSight のホスト名または IP アドレスを指定します。
DemoEndpoint を使用している場合は、「Port」テキスト・ボックスにリストされている
ポート 16102 を使用できます。それ以外の場合、サンプル・プラグイン
が構成されている IBM IoT MessageSight エンドポイントの正しいポート番号
を指定してください。追加の接続オプションについては、「Session」タブ
を選択します。接続プロパティーの設定が終了したら、「Connect」ボタンを
クリックして IBM IoT MessageSight に接続します。次に、「Topic」タブを使用して、
サンプル・プラグインを介した IBM IoT MessageSight 宛先とのメッセージの
送受信を実行します。


ECLIPSE への RESTMSG クライアントのインポート
=============================================
IBM IoT MessageSight restmsg クライアントを Eclipse にインポートするには、
以下のステップを実行します。
1. ProtocolPluginV2.0.zip の内容を unzip します。
2. Eclipse で、「ファイル」->「インポート」->「一般」->
  「既存のプロジェクトをワークスペースへ」を選択します。
3. 「次へ」をクリックします。
4. 「ルート・ディレクトリーの選択」ラジオ・ボタンを選択します。
5. 「参照」をクリックします。
6. 解凍した zip ファイル内容の中の ImaTools/ImaPlugin/samples/restmsgClient
   サブディレクトリーにナビゲートし、選択します。
7. 「プロジェクトをワークスペースにコピー」のチェック・ボックス
   が選択されている場合は、選択解除します。
8. 「終了」をクリックします。

これらのステップを完了すると、Eclipse からサンプル・アプリケーションを
実行できるようになります。

ECLIPSE への RESTMSG プラグインのインポート
==========================================
IBM IoT MessageSight restmsg プラグインを Eclipse にインポートするには、
以下のステップを実行します。
1. ProtocolPluginV2.0.zip の内容を unzip します。
2. Eclipse で、「ファイル」->「インポート」->「一般」->
  「既存のプロジェクトをワークスペースへ」を選択します。
3. 「次へ」をクリックします。
4. 「ルート・ディレクトリーの選択」ラジオ・ボタンを選択します。
5. 「参照」をクリックします。
6. 解凍した zip ファイル内容の中の ImaTools/ImaPlugin/samples/restmsgPlugin
   サブディレクトリーにナビゲートし、選択します。
7. 「プロジェクトをワークスペースにコピー」のチェック・ボックス
   が選択されている場合は、選択解除します。
8. 「終了」をクリックします。

これらのステップを完了すると、サンプル・プラグインおよび自分で作成した
プラグインを Eclipse からコンパイルできるようになります。

ECLIPSE での RESTMSG プラグイン・アーカイブのビルド
==========================================
restmsg サンプル・プラグインを更新し、IBM IoT MessageSight にデプロイする
アーカイブを作成する場合は、以下のステップを実行してください。
注: これらのステップは、restmsgPlugin プロジェクトを Eclipse に
インポートしていること、および Eclipse で既にプロジェクト・ソース・ファイルが
コンパイルされ、クラス・ファイルが bin サブディレクトリーに
配置されていることを前提としています。
1. Eclipse で、restmsgPlugin をパッケージ・エクスプローラーまたは
   ナビゲーターで開きます。.
2. build_plugin_zip.xml を右クリックし、「実行」->「Ant ビルド」を選択します。
   restmsgPlugin の下に plugin サブディレクトリーが作成され、新しい
   restplugin.zip ファイルが含まれています。restmsgPlugin プロジェクトを
   再表示すると、Eclipse UI 内の plugin サブディレクトリーを確認できます。

プラグイン・プロセスのモニタリング
==================================

プラグイン・プロセスのヒープ・サイズ、ガーベッジ・コレクション率、
および CPU 使用量は、$SYS/ResourceStatistics/Plugin トピックを
サブスクライブすることによってモニターできます。

制限および既知の問題
==============================
1. プロトコル・プラグインをデプロイするための MessageSight Web UI
   サポートはありません。コマンド行から実行する必要があります。
2. プロトコル・プラグインでは、共有サブスクリプションは
   サポートされていません。
3. ImaPluginListener.onAuthenticate() メソッドは、IBM IoT MessageSight により呼び出されません。
4. 高可用性構成はサポートされていません。


商標
=======
1. IBM、IBM ロゴ、ibm.com および MessageSight は、世界の多くの国で登録された
International Business Machines Corporation の商標です。
他の会社名、製品名およびサービス名等はそれぞれ各社の商標です。
現時点での IBM の商標リストについては、
www.ibm.com/legal/copytrade.shtml をご覧ください。

2. Java およびすべての Java 関連の商標およびロゴは Oracle やその関連会社の米国およびその他の
国における商標または登録商標です。
