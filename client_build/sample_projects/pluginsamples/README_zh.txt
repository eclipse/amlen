IBM® IoT MessageSight™ 协议插件 SDK V2.0
2016 年 6 月

目录
1. 描述
2. IBM IoT MessageSight 协议插件 SDK 内容
3. 使用 JSON 协议插件样本代码
4. 将样本 JSON 协议插件 SDK 和样本导入到 Eclipse 中
5. 使用 REST 协议插件样本代码
6. 将样本 REST 协议插件 SDK 和样本导入到 Eclipse 中
7. 局限性和已知问题


描述
===========
在 IBM IoT MessageSight 协议插件 SDK 的这个自述文件中，
包含了与内容、更新、修订、限制和已知问题相关的信息。
SDK 使您能够通过使用 Java 编程语言编写插件来扩展
IBM IoT MessageSight 服务器本机支持的协议集。

有关 IBM IoT MessageSight 的更多详细信息，请参阅 IBM Knowledge Center
中的产品文档：
http://ibm.biz/iotms_v20

本发行版中的新功能
===================
IBM IoT MessageSight 协议插件 SDK 现在在非 HA 环境中完全受支持。
请查看随附的许可协议，以了解使用限制。
现在通过 RESTful API 执行插件部署和配置

IBM IoT MessageSight 协议插件 SDK 内容
=================================================
ProtocolPluginV2.0.zip 软件包中的 ImaTools/ImaPlugin 子目录
提供 IBM IoT MessageSight 协议插件 SDK。对于用于
创建要在 IBM IoT MessageSight 中使用的协议插件的类，该软件包中
还提供了这些类的文档。最后，它还包含源组件和样本材料。

目录和文件：
    ImaTools/
        license/ - 包含 IBM IoT MessageSight 协议插件功能部件的许可协议文件

        ImaPlugin/
            README.txt - 本文件

            version.txt - 包含 IBM IoT MessageSight 协议插件 SDK 的版本信息


            doc/ - 包含关于使用 IBM IoT MessageSight 协议插件 SDK 来创建
                   协议插件的文档

            lib/
                imaPlugin.jar - IBM IoT MessageSight 协议插件 SDK

                jsonplugin.zip - 一个 zip 归档，其中包含可部署在 IBM IoT MessageSight 中的
                                 样本 JSON 协议插件。此归档包含
                    samples/jsonmsgPlugin 子目录中提供的样本插件代码
                    （源组件）的已编译类（样本材料）。
                jsonprotocol.jar - 样本插件的已编译类（样本材料）。此
                    JAR 文件也包含在 jsonplugin.zip 文件中。
                restplugin.zip - 一个 zip 归档，其中包含可部署在 IBM IoT MessageSight 中的
                                 样本 REST 协议插件。此归档包含
                    samples/restmsgPlugin 子目录中提供的样本插件代码
                    （源组件）的已编译类（样本材料）。
                restprotocol.jar - 样本 REST 协议插件的已编译类（样本
                     材料）。此 JAR 文件也包含在 restplugin.zip 文件中。
                samples/
                jsonmsgPlugin/
                    .classpath 和 .project - Eclipse 项目配置文件，
                     其允许将 jsonmsgPlugin 子目录作为 Eclipse 项目导入
            	
            	    build_plugin_zip.xml - 一个 Ant 构建文件，此文件可用于
                     创建插件归档
            	
                    src/ - 包含样本插件（源组件）的 Java 源代码，
                           该代码演示如何编写要在 IBM IoT MessageSight 中
                           使用的协议插件

                    config/ - 包含在 IBM IoT MessageSight 中
                              部署所编译样本插件时所需的
                              JSON 描述符文件（源组件）

                    doc/ - 包含样本插件源代码的文档

                jsonmsgClient/
                    .classpath 和 .project - Eclipse 项目配置文件，其允
                     许将 jsonmsgClient 子目录作为 Eclipse 项目导入
            	    JSONMsgWebClient.html - jsonmsgPlugin 中提供的样本插件
                     的客户机应用程序（源组件）。此客户机依赖于 js
                     子目录中的 JavaScript™ 库。
                    js/ - 包含样本 JavaScript 客户机库（源组件），此库
                     用于 jsonmsgPlugin 中提供的样本插件。
                    css/ - 包含样本客户机应用程序和样本客户机库使用的样
                     式表（源组件）和图像文件。
                    doc/ - 包含样本 JavaScript 客户机库的文档。
                restmsgPlugin/
                    .classpath 和 .project - Eclipse 项目配置文件，其允
                     许将 restmsgPlugin 子目录作为 Eclipse 项目导入
            	    build_plugin_zip.xml - 一个 Ant 构建文件，此文件可用于
                     创建插件归档
            	
                    src/ - 包含样本插件（源组件）的 Java 源代码，
                           该代码演示如何编写要在 IBM IoT MessageSight 中
                           使用的协议插件

                    config/ - 包含在 IBM IoT MessageSight 中
                              部署所编译样本插件时所需的
                              JSON 描述符文件（源组件）

                    doc/ - 包含样本插件源代码的文档
                 restmsgClient/
                    .classpath 和 .project - Eclipse 项目配置文件，其允许
                     将 restmsgClient 子目录作为 Eclipse 项目导入
            	    RESTMsgWebClient.html - restmsgPlugin 中提供的样本插件的
                     客户机应用程序（源组件）。此客户机依赖于 js 子目录中
                     的 JavaScript 库。
                    js/ - 包含样本 JavaScript 客户机库（源组件），此库用
                     于 restmsgPlugin 中提供的样本插件。
                    css/ - 包含样本客户机应用程序和样本客户机库使用的样式
                     表（源组件）和图像文件。
                    doc/ - 包含样本 JavaScript 客户机库的文档。
                使用样本 JSON 协议插件和客户机应用程序
====================================================================
JSONMsgWebClient.html 应用程序使用 jsonmsg.js JavaScript 客户机库
与 json_msg 插件通信。JSONMsgWebClient.html 提供一个 Web 接口，
可向 json_msg 插件发送 QoS 0 及从该插件接收 QoS 0（最多一次）消息。

json_msg 协议插件样本包含两个类：JMPlugin 和 JMConnection。JMPlugin 实现
用于初始化和启动协议插件的 ImaPluginListener 接口。JMConnection 实现
ImaConnectionListener 接口，通过使用该接口，客户机可连接到 IBM IoT
MessageSight 并通过 json_message 协议来收发消息。
JMConnection 类从发布客户机接收 JSON 对象，并使用 ImaJson 实用程序类
来将这些对象解析为命令和消息，然后将这些命令和消息
发送到 IBM IoT MessageSight 服务器。同样，JMConnection 类
使用 ImaJson 实用程序类将 IBM IoT MessageSight 消息转换为
JSON 对象，以预订 json_msg 客户机。最后，
json_msg 协议插件包含名为 plugin.json 的描述符文件。用于部署协议插件的
zip 归档中需要此描述符文件，此文件必须始终使用这个文件名。json_msg
协议的样本插件归档 jsonplugin.zip 包含在 ImaTools/ImaPlugin/lib
目录中。它包含一个 JAR 文件（jsonprotocol.jar，其中含有已编译的 JMPlugin 和
JMConnection 类）以及 plugin.json 描述符文件。

要运行样本 JavaScript 客户机应用程序，您必须首先
将样本插件部署到 IBM IoT MessageSight。必须以 zip 或 jar 文件格式归档该插件。此文件必须包含为
目标协议实施该插件的一个或多个 JAR 文件。它还必须包含用于描述插件内容的
JSON 描述符文件。在 ImaTools/ImaPlugin/lib 中提供了名为 jsonplugin.zip
的归档，该归档包含 json_msg 样本插件。部署样本插件归档后，可以通过将
JSONMsgWebClient.html（在 ImaTools/ImaPlugin/samples/jsonmsgClient 中）
加载到 Web 浏览器来运行样本客户机应用程序。您必须维护
JSONMsgWebClient.html 所在的子目录结构，从而为样本应用程序加载样
本 JavaScript 客户机库和样式表。


在 IBM IoT MessageSight 中部署样本插件归档：
=============================================================
要将 jsonplugin.zip 部署到 IBM IoT MessageSight，您必须通过 cURL
使用 PUT 方法来导入插件 zip 文件 jsonmsg.zip：
curl -X PUT -T jsonmsg.zip http://127.0.0.1:9089/ima/v1/file/jsonmsg.zip

接下来，您必须通过 cURL 使用 POST 方法来
创建名为 jsonmsg 的协议插件：
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

最后，您必须使用 cURL 重新启动 IBM IoT MessageSight 协议插件服务器
以便启动该插件：
curl -X POST -d  '{"Service":"Plugin"}' http://127.0.0.1:9089/ima/v1/service/restart

要使用 cURL 确认已成功部署了该插件：
    curl -X GET http://127.0.0.1:9089/ima/v1/configuration/Plugin

您在生成的列表中应可以看到 json_msg。此值对应于 plugin.json 描述符文件中
指定的 Name 属性。

要使用 cURL 确认已成功注册了新协议：
    curl -X GET http://127.0.0.1:9089/ima/v1/configuration/Protocol

您在生成的列表中应可以看到 json_msg。此值对应于 plugin.json 描述符文件中
指定的 Protocol 属性。

如果您使用 DemoEndpoint，那么无需执行其他步骤，即可访问样本 json_msg
协议插件。如果您在使用其他端点，那么将需要遵循相同的过程来指定端点、
连接策略或消息传递策略允许哪些协议。将 DemoEndpoint 及其策略预先配置为
允许所有定义的协议。

运行 JavaScript 样本客户机应用程序：
=============================================
在 Web 浏览器中打开 JSONMsgWebClient.html。在“服务器”文本框中指定
IBM IoT MessageSight 的主机名或 IP 地址。如果您使用 DemoEndpoint，那么可以使用
端口 16102（如“端口”文本框中所列）。否则，请指定
配置了样本插件的 IBM IoT MessageSight 端点的正确
端口号。对于其他连接选项，请选择“会话”选项
卡。设置完连接属性后，单击“连接”按钮
以连接到 IBM IoT MessageSight。然后，利用“主题”选项卡，
通过样本插件在 IBM IoT MessageSight 目标中收发消息。



将 JSON_MSG 客户机导入到 ECLIPSE 中
==========================================
要将 IBM IoT MessageSight json_msg 客户机导入到 Eclipse 中，
请完成以下步骤：
1. 将 ProtocolPluginV2.0.zip 的内容解压缩
2. 在 Eclipse 中，选择“文件->导入->常规->现有项目到工作空间中”
3. 单击“下一步”
4. 选中“选择根目录”单选按钮
5. 单击“浏览”
6. 浏览至已解压缩的 zip 文件内容中的 ImaTools/ImaPlugin/samples/jsonmsgClient
   子目录并将其选中
7. 如果已选中“将项目复制到工作空间”复选框，请将其取消选中
8. 单击“完成”

在完成这些步骤之后，您可以在 Eclipse 中运行样本应用程序。

将 JSON_MSG 插件导入到 ECLIPSE 中
===========================================
要将 IBM IoT MessageSight json_msg 插件导入到 Eclipse 中，
请完成以下步骤：
1. 将 ProtocolPluginV2.0.zip 的内容解压缩
2. 在 Eclipse 中，选择“文件->导入->常规->现有项目到工作空间中”
3. 单击“下一步”
4. 选中“选择根目录”单选按钮
5. 单击“浏览”
6. 浏览至已解压缩的 zip 文件内容中的 ImaTools/ImaPlugin/samples/jsonmsgPlugin
   子目录并将其选中
7. 如果已选中“将项目复制到工作空间”复选框，请将其取消选中
8. 单击“完成”

在完成这些步骤之后，您可以在 Eclipse 中编译样本插件和自己的插件。

在 ECLIPSE 中构建插件归档
=======================================
如果您更新了 json_msg 样本插件，并且希望创建一个要
部署到 IBM IoT MessageSight 中的归档，请执行以下步骤：
注：这些步骤假设您已将 jsonmsgPlugin 项目导入到 Eclipse 中，
而 Eclipse 已编译了该项目的源文件并将类文件
放在 bin 子目录中。
1. 在 Eclipse 中，使用软件包资源管理器或导航器打开 jsonmsgPlugin。
2. 右键单击 build_plugin_zip.xml，然后选择“运行为 -> Ant 构建”。
   在 jsonmsgPlugin 下创建一个包含新 jsonplugin.zip 文件的插件子目
   录。您可以刷新 jsonmsgPlugin 项目，以在 Eclipse UI 中查看该插件
   子目录。


使用样本 REST 协议插件和客户机应用程序
====================================================================
RESTMsgWebClient.html 应用程序使用 restmsg.js JavaScript 客户机库
与 restmsg 插件通信。RESTMsgWebClient.html 提供一个 Web 接口，
可向 restmsg 插件发送 QoS 0 及从该插件接收 QoS 0（最多一次）消息。

restmsg 协议插件样本包含两个类：RestMsgPlugin 和 RestMsgConnection。
RestMsgPlugin 实现用于初始化和启动协议插件的 ImaPluginListener 接口。
RestMsgConnection 实现 ImaConnectionListener 接口，通过使用该接口，
客户机可连接到 IBM IoT MessageSight 并通过 restmsg 协议来收发消息。
RestMsgConnection 类从发布客户机接收 REST 对象，并将这些对象解析为
命令和消息，然后将这些命令和消息发送到 IBM IoT
MessageSight 服务器。同样，RestMsgConnection 类将 IBM IoT
MessageSight 消息转换为 REST 对象，以预订 restmsg 客户机。最后，restmsg 协议插件包含名为 plugin.json 的描述符
文件。用于部署协议插件的 zip 归档中需要此描述符文件，此文件必须始终使用
这个文件名。restmsg 协议的样本插件归档 restplugin.zip 包含在
ImaTools/ImaPlugin/lib 目录中。它包含一个 JAR 文件（restprotocol.jar，
其中含有已编译的 RestMsgPlugin 和 RestMsgConnection 类）以及 plugin.json
描述符文件。

要运行样本 JavaScript 客户机应用程序，您必须首先在
IBM IoT MessageSight 中部署样本插件。必须以 zip 或 jar 文件格式归档该插件。此文件必须包含为
目标协议实施该插件的一个或多个 JAR 文件。它还必须包含用于描述插件内容的
JSON 描述符文件。在 ImaTools/ImaPlugin/lib 中提供了名为 restplugin.zip
的归档，该归档包含 restmsg 样本插件。部署样本 restmsg 插件归档后，可以
通过将 RESTMsgWebClient.html（在 ImaTools/ImaPlugin/samples/restmsgClient
中）加载到 Web 浏览器来运行样本客户机应用程序。您必须维护
RESTMsgWebClient.html 所在的子目录结构，从而为样本应用程序加载样本
JavaScript 客户机库和样式表。


在 IBM IoT MessageSight 中部署 REST 样本插件归档：
==================================================================
要将 restplugin.zip 部署到 IBM IoT MessageSight，您必须通过 cURL
使用 PUT 方法来导入插件 zip 文件 jsonmsg.zip：
curl -X PUT -T restplugin.zip http://127.0.0.1:9089/ima/v1/file/restplugin.zip

接下来，您必须通过 cURL 使用 POST 方法来
创建名为 jsonmsg 的协议插件：
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
   
最后，您必须停止然后重新启动 IBM IoT MessageSight 协议插件服务器
以启动该插件。

最后，您必须使用 cURL 重新启动 IBM IoT MessageSight 协议插件服务器
以便启动该插件：
curl -X POST -d  '{"Service":"Plugin"}' http://127.0.0.1:9089/ima/v1/service/restart

要使用 cURL 确认已成功部署了该插件：
    curl -X GET http://127.0.0.1:9089/ima/v1/configuration/Plugin

您在生成的列表中应可以看到 restmsg。此值对应于 plugin.json 描述符文件中
指定的 Name 属性。

要使用 cURL 确认已成功注册了新协议：
    curl -X GET http://127.0.0.1:9089/ima/v1/configuration/Protocol

您在生成的列表中应可以看到 restmsg。此值对应于 plugin.json 描述符文件中
指定的 Protocol 属性。

如果您使用 DemoEndpoint，那么无需执行其他步骤，即可访问样本 restmsg 协议
插件。如果您在使用其他端点，那么将需要遵循相同的过程来指定端点、
连接策略或消息传递策略允许哪些协议。将 DemoEndpoint 及其策略预先配置为
允许所有定义的协议。

运行 JavaScript 样本客户机应用程序：
=============================================
在 Web 浏览器中打开 RESTMsgWebClient.html。在“服务器”文本框中指定
IBM IoT MessageSight 的主机名或 IP 地址。如果您使用 DemoEndpoint，那么可以使用
端口 16102（如“端口”文本框中所列）。否则，请指定
配置了样本插件的 IBM IoT MessageSight 端点的正确
端口号。对于其他连接选项，请选择“会话”选项
卡。设置完连接属性后，单击“连接”按钮
以连接到 IBM IoT MessageSight。然后，利用“主题”选项卡，
通过样本插件在 IBM IoT MessageSight 目标中收发消息。



将 RESTMSG 客户机导入到 ECLIPSE 中
==========================================
要将 IBM IoT MessageSight restmsg 客户机导入到 Eclipse 中，
请完成以下步骤：
1. 将 ProtocolPluginV2.0.zip 的内容解压缩
2. 在 Eclipse 中，选择“文件->导入->常规->现有项目到工作空间中”
3. 单击“下一步”
4. 选中“选择根目录”单选按钮
5. 单击“浏览”
6. 浏览至已解压缩的 zip 文件内容中的 ImaTools/ImaPlugin/samples/restmsgClient 子目录并将其选中
7. 如果已选中“将项目复制到工作空间”复选框，请将其取消选中
8. 单击“完成”

在完成这些步骤之后，您可以在 Eclipse 中运行样本应用程序。

将 RESTMSG 插件导入到 ECLIPSE 中
==========================================
要将 IBM IoT MessageSight restmsg 插件导入到 Eclipse 中，
请完成以下步骤：
1. 将 ProtocolPluginV2.0.zip 的内容解压缩
2. 在 Eclipse 中，选择“文件->导入->常规->现有项目到工作空间中”
3. 单击“下一步”
4. 选中“选择根目录”单选按钮
5. 单击“浏览”
6. 浏览至已解压缩的 zip 文件内容中的 ImaTools/ImaPlugin/samples/restmsgPlugin
   子目录并将其选中
7. 如果已选中“将项目复制到工作空间”复选框，请将其取消选中
8. 单击“完成”

在完成这些步骤之后，您可以在 Eclipse 中编译样本插件和自己的插件。

在 ECLIPSE 中构建 RESTMSG 插件归档
===============================================
如果您更新了 restmsg 样本插件，并且希望创建一个要
部署到 IBM IoT MessageSight 中的归档，请执行以下步骤：
注：这些步骤假设您已将 restmsgPlugin 项目导入到 Eclipse 中，
而 Eclipse 已编译了该项目的源文件并将类文件
放到 bin 子目录中。
1. 在 Eclipse 中，使用软件包资源管理器或导航器打开 restmsgPlugin。
2. 右键单击 build_plugin_zip.xml，然后选择“运行为 -> Ant 构建”。
   在 restmsgPlugin 下创建一个包含新 restplugin.zip 文件的插件子目
   录。您可以刷新 restmsgPlugin 项目，以在 Eclipse UI 中查看该插件子
   目录。
监控插件进程
==============================

可以通过预订 $SYS/ResourceStatistics/Plugin 主题来监控插件进程的堆大小、
垃圾回收率和 CPU 使用率。

限制和已知问题
==============================
1. MessageSight Web UI 不支持部署协议插件。必须从命令行中完成此操作。
2. 协议插件不支持共享预订。
3. IBM IoT MessageSight 不会调用 ImaPluginListener.onAuthenticate() 方法。
4. 不支持高可用性配置。


声明
=======
1. IBM、IBM 徽标、ibm.com 和 MessageSight 是 International Business
Machines Corp. 在全球许多管辖区域内注册的商标或注册商标。其他产品和
服务名称可能是 IBM 或其他公司的商标。Web 站点
www.ibm.com/legal/copytrade.shtml 上的“Copyright and trademark
information”中提供了 IBM 商标的最新列表。

2. Java 和所有基于 Java 的商标和徽标是 Oracle 和/或其子公司的商标或注册
商标。
