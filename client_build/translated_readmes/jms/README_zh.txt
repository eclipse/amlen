IBM® IoT MessageSight™ JMS 客户机 V2.0
2016 年 6 月

目录
1. 描述
2. IBM IoT MessageSight JMS 客户机程序包内容
3. 使用样本应用程序
4. 将 JMS 客户机导入 Eclipse
5. 局限性和已知问题


描述
===========
IBM IoT MessageSight JMS 客户机程序包自述文件包含有关内容、
更新、修订、限制和已知问题的信息。

有关 IBM IoT MessageSight 的更多详细信息，请参阅 IBM Knowledge Center
中的产品文档：
http://ibm.biz/iotms_v20

本发行版中的新增内容
===================
在本发行版中没有新的 IBM IoT MessageSight JMS 客户机功能。

IBM IoT MESSAGESIGHT JMS 客户机内容
===================================
ImaJmsClientV2.0.zip 程序包提供了 Java™ 消息传递服务 (JMS) 和
IBM IoT MessageSight 资源适配器的 IBM IoT MessageSight 客户机
实施。它还提供了在创建和配置要用于 IBM IoT MessageSight 的 JMS
受管对象时所使用的类的文档。
最后，它包含源组件和样本材料。

目录和文件：
    ImaClient/
        README.txt - 本文件

        license/ - 包含 IBM IoT MessageSight JMS 客户机和 IBM
            IoT MessageSight 资源适配器许可协议文件

        jms/
            version.txt - 包含 IBM IoT MessageSight
                JMS 客户机的版本信息

            .classpath 和 .project - Eclipse 项目配置文件，允许将
                jms 子目录作为 Eclipse 项目进行导入
            	
            doc/ - 包含用于为 IBM IoT MessageSight 创建和配置 JMS
                受管对象的文档

            samples/ - 包含样本应用程序（源组件），
                用于展示如何创建受管对象、如何通过 IBM IoT MessageSight 发送
                和接收消息，以及如何编写已启用高可用性配置的客户机应用程序

            lib/
                imaclientjms.jar - JMS 接口的 IBM IoT MessageSight 实现
                jms.jar - JMS 1.1 接口的 JAR 文件
                jmssamples.jar - 随 IBM IoT MessageSight JMS 客户机提供的样本
                    代码（源组件）的已编译类（样本材料）
                fscontext.jar 和 providerutil.jar - 用于实现文件系统 JNDI 提供
                    程序的 JAR 文件

         ImaResourceAdapter/
            version.txt - 包含 IBM IoT MessageSight
                资源适配器的版本信息

            ima.jmsra.rar - IBM IoT MessageSight 资源适配器归档

使用样本应用程序
================================
随该客户机提供了四种样本应用程序。这些样本应用程序是：
    JMSSampleAdmin
    JMSSample
    HATopicPublisher
    HADurableSubscriber

JMSSampleAdmin 应用程序演示如何为 IBM IoT MessageSight 创建、配置和存储
JMS 受管对象。该应用程序会从输入文件读取 IBM IoT MessageSight JMS
受管对象配置，并填充 JNDI 存储库。该应用程序可以填充 LDAP 或文件系统 JNDI 存储库。

JMSSample 应用程序演示如何将消息发送至 IBM IoT MessageSight 主题和队列，
以及如何从这些主题和队列接收消息。该应用程序在以下三个类中实施：
JMSSample、JMSSampleSend 和 JMSSampleReceive。JMSSample 应用程序可以从
JNDI 存储库检索受管对象，或者可以在运行时创建和配置所需的受管对象。

HATopicPublisher 和 HADurableSubscriber 演示如何对 JMS 客户机应用程序启用
IBM IoT MessageSight 高可用性功能。

要运行随客户机提供的样本应用程序，在类路径中需要三个 JAR 文件：
imaclientjms.jar、jms.jar 和 jmssamples.jar。这三个文件都位于
ImaClient/jms/lib 中。要在运行 JMSSampleAdmin 和 JMSSample
应用程序时使用文件系统 JNDI 提供程序，在类路径中需要另两个 JAR 文件：
fscontext.jar 和 providerutil.jar。这两个 JAR 文件同样位于 ImaClient/jms/lib 中。

在 Linux 上运行已编译的样本：
======================================
从 ImaClient/jms 运行，按以下方式设置 CLASSPATH 环境变量：
    lib/imaclientjms.jar:lib/jms.jar:lib/jmssamples.jar

或者，如果要使用文件系统 JNDI 提供程序，请将 CLASSPATH 设置为：
    lib/imaclientjms.jar:lib/jms.jar:lib/jmssamples.jar:lib/fscontext.jar:lib/providerutil.jar

从 ImaClient/jms 运行以下示例命令行，这些命令行会运行每个应用程序并为每个应用程序提供一个用法语句。

    java -classpath $CLASSPATH com.ibm.ima.samples.jms.JMSSampleAdmin
    java -classpath $CLASSPATH com.ibm.ima.samples.jms.JMSSample
    java -classpath $CLASSPATH com.ibm.ima.samples.jms.HATopicPublisher
    java -classpath $CLASSPATH com.ibm.ima.samples.jms.HADurableSubscriber

在 Windows 上运行已编译的样本：
========================================
从 ImaClient/jms 运行，按以下方式设置 CLASSPATH 环境变量：
    lib\imaclientjms.jar;lib\jms.jar;lib\jmssamples.jar

或者，如果要使用文件系统 JNDI 提供程序，请将 CLASSPATH 设置为：
    lib\imaclientjms.jar;lib\jms.jar;lib\jmssamples.jar;lib\fscontext.jar;lib\providerutil.jar

从 ImaClient/jms 运行以下示例命令行，这些命令行会运行每个应用程序并为每个应用程序提供一个用法语句。

    java -classpath %CLASSPATH% com.ibm.ima.samples.jms.JMSSampleAdmin
    java -classpath %CLASSPATH% com.ibm.ima.samples.jms.JMSSample
    java -classpath %CLASSPATH% com.ibm.ima.samples.jms.HATopicPublisher
    java -classpath %CLASSPATH% com.ibm.ima.samples.jms.HADurableSubscriber
    
要在本地构建样本应用程序，在构建类路径中只需 jms.jar 和 imaclientjms.jar。


将 JMS 客户机导入 ECLIPSE
=====================================
要将 IBM IoT MessageSight JMS 客户机导入 Eclipse，请完成以下步骤：
1. 将 ImaJmsClientV2.0.zip 的内容解压缩
2. 在 Eclipse 中，选择“文件->导入->常规->现有项目到工作空间中”
3. 单击“下一步”
4. 选中“选择根目录”单选按钮
5. 单击 Browse
6. 浏览并选择解压缩后的 zip 文件内容中的 jms 子目录
7. 如果已选中“Copy projects into workspace”的复选框，请取消选中
8. 单击“完成”

完成以上步骤之后，即可从 Eclipse 编译并运行样本应用程序和您自己的客户机应用程序。


限制和已知问题
==============================
1. IBM IoT MessageSight JMS 实施将按照收到消息的顺序传递这些消息。消息优先级设置对交付顺序没有影响。

2. 在除 z/OS® 以外的所有其他平台上的 WebSphere Application Server V8.0 或更高版本上都支持 IBM IoT MessageSight 资源适配器。

声明
=======
1. IBM、IBM 徽标、ibm.com 和 MessageSight 是 International Business
Machines Corp. 在全球许多管辖区域内注册的商标或注册商标。Linux 是 Linus Torvalds 在美国和\或其他国家或地区的注册商标。其他产品和
服务名称可能是 IBM 或其他公司的商标。Web 站点
www.ibm.com/legal/copytrade.shtml 上的“Copyright and trademark
information”中提供了 IBM 商标的最新列表。

2. Java 和所有基于 Java 的商标和徽标是 Oracle 和/或其关联公司的商标或注册商标。
	
