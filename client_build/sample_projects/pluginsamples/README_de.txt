IBM® IoT MessageSight™ Protocol Plug-in SDK Version 2.0
Juni 2016

Inhalt
1. Beschreibung
2. Inhalt von IBM® IoT MessageSight™ Protocol Plug-in SDK
3. Mit dem Beispielcode des JSON-Protokoll-Plug-ins arbeiten
4. SDK für das Beispiel-JSON-Protokoll-Plug-in und Beispiele in Eclipse importieren
5. Mit dem Beispielcode des REST-Protokoll-Plug-ins arbeiten
6. SDK für das Beispiel-REST-Protokoll-Plug-in und Beispiele in Eclipse importieren
7. Einschränkungen und bekannte Probleme


BESCHREIBUNG
============
Diese Readme-Datei zu IBM® IoT MessageSight™ Protocol Plug-in SDK enthält
Informationen zum Inhalt, zu Aktualisierungen, Fixes, Einschränkungen und
bekannten Problemen. Das SDK ermöglicht Ihnen, den vom IBM IoT MessageSight-Server
nativ unterstützten Protokollsatz durch Schreiben von Plug-ins in der
Programmiersprache Java zu erweitern. 

Weitere Informationen zu IBM IoT MessageSight finden Sie in der Produktdokumentation im
IBM Knowledge Center unter der folgenden Webadresse:
http://ibm.biz/iotms_v20 

NEUERUNGEN IN DIESEM RELEASE
============================
IBM IoT MessageSight Protocol Plug-in SDK wird jetzt auch in Umgebungen ohne Hochverfügbarkeit
vollständig unterstützt. Bitte informieren Sie sich in den beigefügten Lizenzbedingungen über
Nutzungsbeschränkungen. Die Plug-in-Implementierung und die Plug-in-Konfiguration erfolgen
jetzt über eine REST-konforme API. 

INHALT VON IBM IoT MESSAGESIGHT PROTOCOL PLUG-IN SDK
====================================================
Das Unterverzeichnis "ImaTools/ImaPlugin" im Paket "ProtocolPluginV2.0.zip" enthält
das IBM IoT MessageSight Protocol Plug-in SDK. Es enthält auch die Dokumentation zu
den Klassen, die verwendet werden, um Protokoll-Plug-ins für die Verwendung mit
IBM IoT MessageSight zu erstellen. Außerdem enthält das Verzeichnis Quellenkomponenten
und Beispielmaterial. 

Verzeichnisse und Dateien:
    ImaTools/
        license/ - enthält die Dateien der Lizenzvereinbarung für
            IBM IoT MessageSight-Protokoll-Plug-in-Feature
        ImaPlugin/
            README.txt - die vorliegende Datei

            version.txt - enthält Versionsinformationen für IBM IoT MessageSight
                Protocol Plug-in SDK

            doc/ - enthält die Dokumentation für die Erstellung von Protokoll-Plug-ins
                mit IBM IoT MessageSight Protocol Plug-in SDK

            lib/
                imaPlugin.jar - das IBM IoT MessageSight Protocol Plug-in SDK

                jsonplugin.zip - ein ZIP-Archiv, das ein Beispiel-JSON-Protokoll-Plug-in
                    enthält, das in IBM IoT MessageSight implementiert werden kann. Dieses
                    Archiv enthält die kompilierten Klassen (Beispielmaterial) für
                    den Beispiel-Plug-in-Code (Quellenkomponenten), der im Unterverzeichnis
                    "samples/jsonmsgPlugin" bereitgestellt wird.

                jsonprotocol.jar - die kompilierten Klassen (Beispielmaterial) für
                    das Beispiel-Plug-in. Diese JAR-Datei ist auch in der Datei
                    "jsonplugin.zip" enthalten.

                restplugin.zip - ein ZIP-Archiv, das ein Beispiel-REST-Protokoll-Plug-in
                    enthält, das in IBM IoT MessageSight implementiert werden kann. Dieses
                    Archiv enthält die kompilierten Klassen (Beispielmaterial) für
                    den Beispiel-Plug-in-Code (Quellenkomponenten), der im Unterverzeichnis
                    "samples/restmsgPlugin" bereitgestellt wird.

                restprotocol.jar - die kompilierten Klassen (Beispielmaterial) für das
                    Beispiel-REST-Protokoll-Plug-in. Diese JAR-Datei ist auch
                    in der Datei "restplugin.zip" enthalten.

                samples/
                jsonmsgPlugin/
                    .classpath und .project - Konfigurationsdateien
            	       für das Eclipse-Projekt, die den Import des Unterverzeichnisses
            	       "jsonmsgPlugin" als Eclipse-Projekt ermöglichen
            	
            	    build_plugin_zip.xml - eine Ant-Builddatei, die Sie verwenden können,
            	       um ein Plug-in-Archiv zu erstellen
            	
                    src/ - enthält Java-Quellcode für ein Beispiel-Plug-in
                        (Quellenkomponenten), das veranschaulicht, wie ein Protokoll-Plug-in
                        für die Verwendung mit IBM IoT MessageSight geschrieben wird.

                    config/ - enthält die JSON-Deskriptordatei (Quellenkomponenten),
                        die für die Implementierung des kompilierten
                        Beispiel-Plug-ins in IBM IoT MessageSight erforderlich ist

                    doc/ - enthält die Dokumentation zum Beispiel-Plug-in-Quellcode
                        

                jsonmsgClient/
                    .classpath und .project - Konfigurationsdateien
            	       für das Eclipse-Projekt, die den Import des Unterverzeichnisses
            	       "jsonmsgClient" als Eclipse-Projekt ermöglichen
            	
            	    JSONMsgWebClient.html - die Clientanwendung (Quellenkomponenten)
            	        für das Beispiel-Plug-in, das im Unterverzeichnis "jsonmsgPlugin"
            	        bereitgestellt wird.  Dieser Client ist von der JavaScript™-Bibliothek
            	        im Unterverzeichnis "js" abhängig.
            	
                    js/ - enthält die JavaScript-Beispielclientbibliothek
                        (Quellenkomponenten) für die Verwendung mit dem Beispiel-Plug-in,
                        das im Unterverzeichnis "jsonmsgPlugin" bereitgestellt wird.

                    css/ - enthält Style-Sheets (Quellenkomponenten) und Imagedateien,
                        die von der Beispielclientanwendung und der Beispielclientbibliothek
                        verwendet werden.

                    doc/ - enthält Dokumentation zur
                        JavaScript-Beispielclientbibliothek.

            restmsgPlugin/
                    .classpath und .project - Konfigurationsdateien
            	       für das Eclipse-Projekt, die den Import des Unterverzeichnisses
            	       "restmsgPlugin" als Eclipse-Projekt ermöglichen
            	
            	    build_plugin_zip.xml - eine Ant-Builddatei, die Sie verwenden können,
            	       um ein Plug-in-Archiv zu erstellen
            	
                    src/ - enthält Java-Quellcode für ein Beispiel-Plug-in
                        (Quellenkomponenten), das veranschaulicht, wie ein Protokoll-Plug-in
                        für die Verwendung mit IBM IoT MessageSight geschrieben wird.

                    config/ - enthält die JSON-Deskriptordatei (Quellenkomponenten),
                        die für die Implementierung des kompilierten
                        Beispiel-Plug-ins in IBM IoT MessageSight erforderlich ist

                    doc/ - enthält Dokumentation zum
                        Beispiel-Plug-in-Quellcode

            restmsgClient/
                    .classpath und .project - Konfigurationsdateien
            	       für das Eclipse-Projekt, die den Import des Unterverzeichnisses
            	       "restmsgClient" als Eclipse-Projekt ermöglichen
            	
            	    RESTMsgWebClient.html - die Clientanwendung (Quellenkomponenten)
            	        für das Beispiel-Plug-in, das im Unterverzeichnis "restmsgPlugin"
            	        bereitgestellt wird. Dieser Client ist von der JavaScript-Bibliothek
            	        im Unterverzeichnis "js" abhängig.
            	
                    js/ - enthält die JavaScript-Beispielclientbibliothek
                        (Quellenkomponenten) für die Verwendung mit dem Beispiel-Plug-in,
                        das im Unterverzeichnis "restmsgPlugin" bereitgestellt wird.

                    css/ - enthält Style-Sheets (Quellenkomponenten) und Imagedateien,
                        die von der Beispielclientanwendung und der Beispielclientbibliothek
                        verwendet werden.

                    doc/ - enthält Dokumentation zur
                        JavaScript-Beispielclientbibliothek.

MIT BEISPIEL-JSON-PROTOKOLL-PLUG-IN UND BEISPIELCLIENTANWENDUNG ARBEITEN
====================================================================
Die Anwendung "JSONMsgWebClient.html" verwendet die JavaScript-Clientbibliothek
"jsonmsg.js" für die Kommunikation mit dem Plug-in "json_msg". Die Datei
"JSONMsgWebClient.html" enthält eine Webschnittstelle, über die Nachrichten
der Servicequalität 0 (höchstens einmal) mit dem Plug-in "json_msg" ausgetauscht
werden können. 

Das Beispielprotokoll-Plug-in "json_msg" besteht aus zwei Klassen, "JMPlugin" und
"JMConnection". "JMPlugin" implementiert die Schnittstelle "ImaPluginListener",
die erforderlich ist, um ein Protokoll-Plug-in zu initialisieren und zu starten.
"JMConnection" implementiert die Schnittstelle "ImaConnectionListener", über die
Clients eine Verbindung zu IBM IoT MessageSight herstellen und Nachrichten über
das Protokoll "json_message" senden und empfangen können. Die Klasse "JMConnection"
empfängt JSON-Objekte von Veröffentlichungsclients und verwendet die Dienstprogrammklasse
"ImaJson", um diese Objekte in Befehle und Nachrichten zu parsen, die sie an den
IBM IoT MessageSight-Server sendet. In ähnlicher Weise verwendet die Klasse "JMConnection"
die Dienstprogrammklasse "ImaJson", um IBM IoT MessageSight-Nachrichten zwecks Subskription
von json_msg-Clients in JSON-Objekte zu konvertieren. Schließlich enthält das Protokoll-Plug-in
"json_msg" die Deskriptordatei "plugin.json". Diese Deskriptordatei ist erforderlich im
ZIP-Archiv, das zur Implementierung eines Protokoll-Plug-ins verwendet wird, und muss immer
diesen Dateinamen verwenden. Ein Beispiel-Plug-in-Archiv für das Protokoll "json_msg",
"jsonplugin.zip", ist im Verzeichnis "ImaTools/ImaPlugin/lib" enthalten. Es enthält
eine JAR-Datei ("jsonprotocol.jar" mit den kompilierten Klassen "JMPlugin" und "JMConnection")
und die Deskriptordatei "plugin.json".

Um die JavaScript-Beispielclientanwendung auszuführen, müssen Sie zuerst das Beispiel-Plug-in
in IBM IoT MessageSight implementieren. Das Plug-in muss in einer ZIP- oder JAR-Datei archiviert
sein. Diese Datei muss die JAR-Datei oder JAR-Dateien, die das Plug-in für das Zielprotokoll
implementieren, enthalten. Sie muss auch eine JSON-Deskriptordatei enthalten, die den
Plug-in-Inhalt beschreibt. Ein Archiv mit dem Namen "jsonplugin.zip" ist im Verzeichnis
"ImaTools/ImaPlugin/lib" verfügbar und enthält das Beispiel-Plug-in "json_msg". Sobald
das Beispiel-Plug-in-Archiv implementiert ist, können Sie die Beispielclientanwendung
ausführen, indem Sie "JSONMsgWebClient.html" (im Verzeichnis "ImaTools/ImaPlugin/samples/jsonmsgClient")
in einem Web-Browser laden. Sie müssen die Unterverzeichnisstruktur, in der die Datei
"JSONMsgWebClient.html" sich befindet, beibehalten, um die JavaScript-Beispielclientbibliothek
und Style-Sheets für die Beispielanwendung laden zu können.




Beispiel-Plug-in-Archiv in IBM IoT MessageSight implementieren:
=============================================================
Zum Implementieren der Datei "jsonplugin.zip" in IBM IoT MessageSight müssen
Sie mit der Methode PUT die Plug-in-ZIP-Datei "jsonmsg.zip" unter Verwendung
von cURL importieren:
	curl -X PUT -T jsonmsg.zip http://127.0.0.1:9089/ima/v1/file/jsonmsg.zip

Als Nächstes müssen Sie das Plug-in mit der Methode POST erstellen, um ein
Protokoll-Plug-in mit dem Namen "jsonmsg" unter Verwendung von cURL zu erstellen:
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

Abschließend müssen Sie den IBM IoT MessageSight-Protokoll-Plug-in-Server erneut starten,
um das Plug-in unter Verwendung von cURL zu starten:
	curl -X POST -d  '{"Service":"Plugin"}' http://127.0.0.1:9089/ima/v1/service/restart

Überprüfen Sie mit cURL, ob das Plug-in erfolgreich implementiert wurde:
    curl -X GET http://127.0.0.1:9089/ima/v1/configuration/Plugin

In der Ergebnisliste sollte "json_msg" angezeigt werden. Dieser Wert
entspricht der Eigenschaft "Name", die in der Deskriptordatei "plugin.json" angegeben ist. 

Überprüfen Sie mit cURL, ob das neue Protokoll erfolgreich registriert wurde:
    curl -X GET http://127.0.0.1:9089/ima/v1/configuration/Protocol

In der Ergebnisliste sollte "json_msg" angezeigt werden. Dieser Wert entspricht
der Eigenschaft "Protocol", die in der Deskriptordatei "plugin.json" angegeben ist. 

Wenn Sie den Demoendpunkt (DemoEndpoint) verwenden, sind keine weiteren Schritte
erforderlich, um auf das Beispielprotokoll-Plug-in "json_msg" zuzugreifen. Wenn Sie einen anderen
Endpunkt verwenden, müssen Sie dieselben Prozeduren ausführen, um anzugeben, welche Protokolle
von einem Endpunkt, einer Verbindungsrichtlinie oder einer Messaging-Richtlinie zugelassen werden. Der
Demoendpunkt und seine Richtlinien sind vorkonfiguriert und lassen alle definierten Protokolle zu. 

JavaScript-Beispielclientanwendung ausführen:
=============================================
Öffnen Sie die Datei "JSONMsgWebClient.html" in einem Web-Browser. Geben Sie den
Hostnamen oder die IP-Adresse für IBM IoT MessageSight im Textfeld "Server" an.
Wenn Sie den Demoendpunkt verwenden, können Sie Port 16102, der im Textfeld
"Port" aufgelistet ist, verwenden. Andernfalls geben Sie die richtige Portnummer
für den IBM IoT MessageSight-Endpunkt an, der mit dem Beispiel-Plug-in konfiguriert
ist. Um auf weitere Verbindungsoptionen zuzugreifen, wählen Sie die Registerkarte
"Sitzung" aus. Wenn Sie mit dem Festlegen der Verbindungseigenschaften fertig sind,
klicken Sie auf die Schaltfläche "Verbinden", um eine Verbindung zu IBM IoT MessageSight
herzustellen. Verwenden Sie dann die Registerkarte "Topic", um über das Beispiel-Plug-in
Nachrichten an IBM IoT MessageSight-Ziele zu senden und von diesen zu empfangen. 


CLIENT JSON_MSG IN ECLIPSE IMPORTIEREN
======================================
Gehen Sie wie folgt vor, um das IBM IoT MessageSight-Plug-in "json_msg" in Eclipse
zu importieren:
1. Entpacken Sie die Datei "ProtocolPluginV2.0.zip".
2. In Eclipse wählen Sie Folgendes aus: Datei -> Importieren -> Allgemein -> Vorhandene Projekte in den Arbeitsbereich.
3. Klicken Sie auf "Weiter".
4. Wählen Sie das Optionsfeld "Stammverzeichnis auswählen" aus.
5. Klicken Sie auf "Durchsuchen".
6. Navigieren Sie im Inhalt der entpackten ZIP-Datei zum Unterverzeichnis
   "ImaTools/ImaPlugin/samples/jsonmsgClient" und wählen Sie es aus.
7. Ist das Kontrollkästchen "Projekte in Arbeitsbereich kopieren" ausgewählt, wählen Sie es ab.
8. Klicken Sie auf "Fertigstellen". 

Wenn Sie diese Schritte ausgeführt haben, können Sie die Beispielanwendung in Eclipse ausführen. 

PLUG-IN JSON_MSG IN ECLIPSE IMPORTIEREN
=======================================
Gehen Sie wie folgt vor, um das IBM IoT MessageSight-Plug-in "json_msg" in Eclipse
zu importieren:
1. Entpacken Sie die Datei "ProtocolPluginV2.0.zip".
2. Wählen Sie in Eclipse folgende Optionen aus: Datei -> Importieren -> Allgemein -> Vorhandene Projekte in den Arbeitsbereich.
3. Klicken Sie auf "Weiter".
4. Wählen Sie das Optionsfeld "Stammverzeichnis auswählen" aus.
5. Klicken Sie auf "Durchsuchen".
6. Navigieren Sie im Inhalt der entpackten ZIP-Datei zum Unterverzeichnis
   "ImaTools/ImaPlugin/samples/jsonmsgPlugin" und wählen Sie es aus.
7. Ist das Kontrollkästchen "Projekte in Arbeitsbereich kopieren" ausgewählt, wählen Sie es ab.
8. Klicken Sie auf "Fertigstellen". 

Wenn Sie diese Schritte ausgeführt haben, können Sie das Beispiel-Plug-in und
Ihre eigenen Plug-ins in Eclipse kompilieren. 

PLUG-IN-ARCHIV IN ECLIPSE ERSTELLEN
===================================
Wenn Sie das Beispiel-Plug-in "json_msg" aktualisieren und ein Archiv erstellen möchten,
das in IBM IoT MessageSight implementiert werden soll, führen Sie die folgenden Schritte aus:
HINWEIS: Es wird vorausgesetzt, dass Sie das Projekt "jsonmsgPlugin" in Eclipse importiert
haben und dass Eclipse bereits die Projektquellendateien kompiliert und die Klassendateien
in das Unterverzeichnis "bin" gestellt hat.
1. Öffnen Sie in Eclipse "jsonmsgPlugin" im Paketexplorer oder im Navigator.
2. Klicken Sie mit der rechten Maustaste auf "build_plugin_zip.xml" und wählen Sie
   "Ausführen als -> Ant-Build" aus. Unter "jsonmsgPlugin" wird das Unterverzeichnis "plugin"
   mit einer neuen Datei "jsonplugin.zip" erstellt. Sie können das Projekt "jsonmsgPlugin"
   aktualisieren, um das Unterverzeichnis "plugin" in der Eclipse-Benutzerschnittstelle zu sehen. 


MIT BEISPIEL-REST-PROTOKOLL-PLUG-IN UND BEISPIELCLIENTANWENDUNG ARBEITEN
====================================================================
Die Anwendung "RESTMsgWebClient.html" verwendet die JavaScript-Clientbibliothek
"restmsg.js" für die Kommunikation mit dem Plug-in "restmsg". Die Datei
"RESTMsgWebClient.html" enthält eine Webschnittstelle, über die Nachrichten
der Servicequalität 0 (höchstens einmal) mit dem Plug-in "restmsg" ausgetauscht
werden können. 

Das Beispielprotokoll-Plug-in "restmsg" besteht aus zwei Klassen, "RestMsgPlugin" und
"RestMsgConnection". "RestMsgPlugin" implementiert die Schnittstelle "ImaPluginListener",
die erforderlich ist, um ein Protokoll-Plug-in zu initialisieren und zu starten. 
"RestMsgConnection" implementiert die Schnittstelle "ImaConnectionListener", über die
Clients eine Verbindung zu IBM IoT MessageSight herstellen und Nachrichten über das
Protokoll "restmsg" senden und empfangen können. Die Klasse "RestMsgConnection" empfängt
REST-Objekte von Veröffentlichungsclients und parst diese Objekte in Befehle und Nachrichten,
die sie an den IBM IoT MessageSight-Server sendet. In ähnlicher Weise konvertiert die Klasse
"RestMsgConnection" IBM IoT MessageSight-Nachrichten in REST-Objekte zwecks Subskription von
restmsg-Clients. Schließlich enthält das Protokoll-Plug-in "restmsg" die Deskriptordatei
"plugin.json". Diese Deskriptordatei ist erforderlich im ZIP-Archiv, das zur Implementierung
eines Protokoll-Plug-ins verwendet wird, und muss immer diesen Dateinamen verwenden. Ein
Beispiel-Plug-in-Archiv für das Protokoll "restmsg", "restplugin.zip", ist im Verzeichnis
"ImaTools/ImaPlugin/lib" enthalten. Es enthält eine JAR-Datei ("restprotocol.jar" mit den
kompilierten Klassen "RestMsgPlugin" und "JMConnection") und die Deskriptordatei "plugin.json".

Um die JavaScript-Beispielclientanwendung auszuführen, müssen Sie zuerst das Beispiel-Plug-in
in IBM IoT MessageSight implementieren. Das Plug-in muss in einer ZIP- oder JAR-Datei archiviert
sein. Diese Datei muss die JAR-Datei oder JAR-Dateien, die das Plug-in für das Zielprotokoll
implementieren, enthalten. Sie muss auch eine JSON-Deskriptordatei enthalten, die den
Plug-in-Inhalt beschreibt. Ein Archiv mit dem Namen "restplugin.zip" ist im Verzeichnis
"ImaTools/ImaPlugin/lib" verfügbar und enthält das Beispiel-Plug-in "restmsg". Sobald
das Beispiel-Plug-in-Archiv "restmsg" implementiert ist, können Sie die Beispielclientanwendung
ausführen, indem Sie "RESTMsgWebClient.html" (im Verzeichnis "ImaTools/ImaPlugin/samples/restmsgClient")
in einem Web-Browser laden. Sie müssen die Unterverzeichnisstruktur, in der die Datei
"RESTMsgWebClient.html" sich befindet, beibehalten, um die JavaScript-Beispielclientbibliothek
und Style-Sheets für die Beispielanwendung laden zu können.




Beispiel-REST-Plug-in-Archiv in IBM IoT MessageSight implementieren:
==================================================================
Zum Implementieren der Datei "restplugin.zip" in IBM IoT MessageSight müssen Sie
mit der Methode PUT die Plug-in-ZIP-Datei "jsonmsg.zip" unter Verwendung von
cURL importieren:
	curl -X PUT -T restplugin.zip http://127.0.0.1:9089/ima/v1/file/restplugin.zip

Als Nächstes müssen Sie das Plug-in mit der Methode POST erstellen, um ein
Protokoll-Plug-in mit dem Namen "jsonmsg" unter Verwendung von cURL zu erstellen:
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
   
Schließlich müssen Sie den IBM IoT MessageSight-Protokoll-Plug-in-Server stoppen
und erneut starten, um das Plug-in zu starten. 

Abschließend müssen Sie den IBM IoT MessageSight-Protokoll-Plug-in-Server erneut starten,
um das Plug-in unter Verwendung von cURL zu starten:
	curl -X POST -d  '{"Service":"Plugin"}' http://127.0.0.1:9089/ima/v1/service/restart

Überprüfen Sie mit cURL, ob das Plug-in erfolgreich implementiert wurde:
    curl -X GET http://127.0.0.1:9089/ima/v1/configuration/Plugin

In der Ergebnisliste sollte "restmsg" angezeigt werden. Dieser Wert entspricht
der Eigenschaft "Name", die in der Deskriptordatei "plugin.json" angegeben ist. 

Überprüfen Sie mit cURL, ob das neue Protokoll erfolgreich registriert wurde:
    curl -X GET http://127.0.0.1:9089/ima/v1/configuration/Protocol

In der Ergebnisliste sollte "restmsg" angezeigt werden. Dieser Wert entspricht
der Eigenschaft "Protocol", die in der Deskriptordatei "plugin.json" angegeben ist. 

Wenn Sie den Demoendpunkt (DemoEndpoint) verwenden, sind keine weiteren Schritte
erforderlich, um auf das Beispielprotokoll-Plug-in "restmsg" zuzugreifen. Wenn Sie einen anderen
Endpunkt verwenden, müssen Sie dieselben Prozeduren ausführen, um anzugeben, welche Protokolle
von einem Endpunkt, einer Verbindungsrichtlinie oder einer Messaging-Richtlinie zugelassen werden. Der
Demoendpunkt und seine Richtlinien sind vorkonfiguriert und lassen alle definierten Protokolle zu. 

JavaScript-Beispielclientanwendung ausführen:
=============================================
Öffnen Sie die Datei "RESTMsgWebClient.html" in einem Web-Browser. Geben Sie den
Hostnamen oder die IP-Adresse für IBM IoT MessageSight im Textfeld "Server" an.
Wenn Sie den Demoendpunkt verwenden, können Sie Port 16102, der im Textfeld
"Port" aufgelistet ist, verwenden. Andernfalls geben Sie die richtige Portnummer
für den IBM IoT MessageSight-Endpunkt an, der mit dem Beispiel-Plug-in konfiguriert
ist. Um auf weitere Verbindungsoptionen zuzugreifen, wählen Sie die Registerkarte
"Sitzung" aus. Wenn Sie mit dem Festlegen der Verbindungseigenschaften fertig sind,
klicken Sie auf die Schaltfläche "Verbinden", um eine Verbindung zu IBM IoT MessageSight
herzustellen. Verwenden Sie dann die Registerkarte "Topic", um über das Beispiel-Plug-in
Nachrichten an IBM IoT MessageSight-Ziele zu senden und von diesen zu empfangen. 


CLIENT RESTMSG IN ECLIPSE IMPORTIEREN
======================================
Gehen Sie wie folgt vor, um den IBM IoT MessageSight-Client "restmsg" in Eclipse
zu importieren:
1. Entpacken Sie die Datei "ProtocolPluginV2.0.zip".
2. Wählen Sie in Eclipse "Datei -> Importieren -> Allgemein -> Vorhandene Projekte in Arbeitsbereich" aus.
3. Klicken Sie auf "Weiter".
4. Wählen Sie das Optionsfeld "Stammverzeichnis auswählen" aus.
5. Klicken Sie auf "Durchsuchen".
6. Navigieren Sie im Inhalt der entpackten ZIP-Datei zum Unterverzeichnis
   "ImaTools/ImaPlugin/samples/restmsgClient" und wählen Sie es aus.
7. Ist das Kontrollkästchen "Projekte in Arbeitsbereich kopieren" ausgewählt, wählen Sie es ab.
8. Klicken Sie auf "Fertigstellen".

Wenn Sie diese Schritte ausgeführt haben, können Sie die Beispielanwendung in Eclipse ausführen. 

PLUG-IN RESTMSG IN ECLIPSE IMPORTIEREN
======================================
Gehen Sie wie folgt vor, um das IBM IoT MessageSight-Plug-in "restmsg" in IBM MessageSight
zu importieren:
1. Entpacken Sie die Datei "ProtocolPluginV2.0.zip".
2. Wählen Sie in Eclipse "Datei -> Importieren -> Allgemein -> Vorhandene Projekte in Arbeitsbereich" aus.
3. Klicken Sie auf "Weiter".
4. Wählen Sie das Optionsfeld "Stammverzeichnis auswählen" aus.
5. Klicken Sie auf "Durchsuchen".
6. Navigieren Sie im Inhalt der entpackten ZIP-Datei zum Unterverzeichnis
   "ImaTools/ImaPlugin/samples/restmsgPlugin" und wählen Sie es aus.
7. Ist das Kontrollkästchen "Projekte in Arbeitsbereich kopieren" ausgewählt,
   wählen Sie es ab.
8. Klicken Sie auf "Fertigstellen".

Wenn Sie diese Schritte ausgeführt haben, können Sie das Beispiel-Plug-in und
Ihre eigenen Plug-ins in Eclipse kompilieren. 

PLUG-IN-ARCHIV RESTMSG IN ECLIPSE ERSTELLEN
===========================================
Wenn Sie das Beispiel-Plug-in "restmsg" aktualisieren und Sie ein Archiv erstellen möchten,
das in IBM IoT MessageSight implementiert werden soll, führen Sie die folgenden Schritte aus:
HINWEIS: Es wird vorausgesetzt, dass Sie das Projekt "restmsgPlugin" in Eclipse importiert
haben und dass Eclipse bereits die Projektquellendateien kompiliert und die Klassendateien
in das Unterverzeichnis "bin" gestellt hat.
1. Öffnen Sie in Eclipse "restmsgPlugin" im Paketexplorer oder im Navigator.
2. Klicken Sie mit der rechten Maustaste auf "build_plugin_zip.xml" und wählen Sie
   "Ausführen als -> Ant-Build" aus. Unter "restmsgPlugin" wird das Unterverzeichnis "plugin"
   mit einer neuen Datei "restplugin.zip" erstellt. Sie können das Projekt "restmsgPlugin"
   aktualisieren, um das Unterverzeichnis "plugin" in der Eclipse-Benutzerschnittstelle zu sehen.

PLUG-IN-PROZESS ÜBERWACHEN
==========================

Die Größe des Heapspeichers, die Garbage-Collection-Geschwindigkeit und die CPU-Auslastung
des Plug-in-Prozesses kann durch Subskription des Topics $SYS/ResourceStatistics/Plugin überwacht werden.    

EINSCHRÄNKUNGEN UND BEKANNTE PROBLEME
=====================================
1. Die MessageSight-Webbenutzerschnittstelle wird für die Implementierung von Protokoll-Plug-ins
   nicht unterstützt. Dieser Vorgang muss über die Befehlszeile ausgeführt werden.
2. Gemeinsam genutzte Subskriptionen werden für Protokoll-Plug-ins nicht unterstützt.
3. Die Methode "ImaPluginListener.onAuthenticate()" wird von IBM IoT MessageSight nicht aufgerufen.
4. Hochverfügbarkeitskonfigurationen werden nicht unterstützt. 


BEMERKUNGEN
===========
1. IBM, das IBM Logo, ibm.com und MessageSight sind Marken oder eingetragene Marken der
IBM Corporation in den USA und/oder anderen Ländern. Weitere Produkt- und Servicenamen
können Marken von IBM oder anderen Unternehmen sein. Eine aktuelle Liste der IBM Marken
finden Sie auf der Webseite "Copyright and trademark information" unter
www.ibm.com/legal/copytrade.shtml. 

2. Java und alle auf Java basierenden Marken und Logos sind Marken oder eingetragene
Marken der Oracle Corporation und/oder ihrer verbundenen Unternehmen.
