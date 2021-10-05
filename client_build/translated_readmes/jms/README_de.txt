IBM® IoT MessageSight™ JMS Client Version 2.0
Juni 2016

Inhalt
1. Beschreibung
2. Inhalt des IBM IoT MessageSight JMS Client-Pakets
3. Mit Beispielanwendungen arbeiten
4. JMS Client in Eclipse importieren
5. Beschränkungen und bekannte Probleme


BESCHREIBUNG
============
Diese README-Datei für das Paket "IBM IoT MessageSight JMS Client" enthält Informationen
zum Inhalt, zu Aktualisierungen, Programmkorrekturen, Einschränkungen und bekannten Problemen.

Weitere Informationen zu IBM IoT MessageSight finden Sie in der Produktdokumentation im
IBM Knowledge Center unter der folgenden Webadresse:
http://ibm.biz/iotms_v20 

NEUERUNGEN IN DIESEM RELEASE
============================
Dieses Release enthält keine neuen Features für IBM IoT MessageSight JMS Client. 

INHALT VON IBM MESSAGESIGHT IoT JMS Client
==========================================
Das Paket "ImaJmsClientV2.0.zip" stellt die IBM IoT MessageSight-Clientimplementierung
des Java™ Messaging Service (JMS) und IBM MessageSight Resource Adapter bereit.
Es enthält auch die Dokumentation für die Klassen, die für die Erstellung und
Konfiguration von verwalteten JMS-Objekten für IBM IoT MessageSight verwendet werden.
Außerdem enthält das Paket Quellenkomponenten und Beispielmaterial. 

Verzeichnisse und Dateien:
    ImaClient/
        README.txt - diese Datei

        license/ - enthält die Dateien mit der Lizenzvereinbarung für
            IBM IoT MessageSight JMS Client und IBM IoT MessageSight Resource Adapter

        jms/
            version.txt - enthält die Versionsinformationen für IBM IoT MessageSight
                JMS Client

            .classpath und .project - Eclipse-Projektkonfigurationsdateien,
            mit denen das Unterverzeichnis "jms" als Eclipse-Projekt importiert
            werden kann
            	
            doc/ - enthält Dokumentation zur Erstellung und Konfiguration von
                verwalteten JMS-Objekten für IBM IoT MessageSight

            samples/ - enthält Beispielanwendungen (Quellenkomponenten),
                die veranschaulichen, wie verwaltete Objekte erstellt,
                Nachrichten über IBM IoT MessageSight gesendet und empfangen
                und Clientanwendungen, die Hochverfügbarkeitskonfigurationen
                unterstützen, geschrieben werden

            lib/
                imaclientjms.jar - die IBM IoT MessageSight-Implementierung der
                    JMS-Schnittstelle
                jms.jar - die JAR-Datei für die JMS-1.1-Schnittstelle
                jmssamples.jar - die kompilierten Klassen (Beispielmaterial) für den
                    Beispielcode (Quellenkomponenten), die im Lieferumfang von
                    IBM IoT MessageSight JMS Client enthalten sind
                fscontext.jar und providerutil.jar - JAR-Dateien, die einen JNDI-Provider
                    für das Dateisystem implementieren

         ImaResourceAdapter/
            version.txt - enthält die Versionsnummer für
                IBM IoT MessageSight Resource Adapter

            ima.jmsra.rar - das IBM IoT MessageSight Resource Adapter-Archiv

MIT BEISPIELANWENDUNGEN ARBEITEN
================================
Im Lieferumfang des Clients sind vier Beispielanwendungen enthalten. Diese
Beispielanwendungen sind:
    "JMSSampleAdmin"
    "JMSSample"
    "HATopicPublisher"
    "HADurableSubscriber"

Die Anwendung "JMSSampleAdmin" veranschaulicht, wie verwaltete JMS-Objekte für
IBM IoT MessageSight erstellt, konfiguriert und gespeichert werden. Diese Anwendung
liest Konfigurationen verwalteter IBM IoT MessageSight-JMS-Objekte aus einer Eingabedatei
und füllt ein JNDI-Repository mit Daten. Diese Anwendung kann LDAP- oder dateisystembasierte
JNDI-Repositorys mit Daten füllen. 

Die Anwendung "JMSSample" veranschaulicht, wie Nachrichten an IBM IoT MessageSight-Topics und -Warteschlangen
gesendet und von diesen empfangen werden. Diese Anwendung ist in den folgenden drei Klassen implementiert:
"JMSSample", "JMSSampleSend" und "JMSSampleReceive". Die Anwendung "JMSSample"
kann entweder verwaltete Objekte aus einem JNDI-Repository abrufen oder
die erforderlichen verwalteten Objekte zur Ausführungszeit erstellen und
konfigurieren. 

Die Anwendungen "HATopicPublisher" und "HADurableSubscriber" veranschaulichen, wie
JMS-Clientanwendungen für die Nutzung der IBM IoT MessageSight-Hochverfügbarkeitsfunktionen aktiviert werden. 

Für die Ausführung der mit dem Client bereitgestellten Beispielanwendungen
müssen im Klassenpfad drei JAR-Dateien angegeben sein: "imaclientjms.jar", "jms.jar"
und "jmssamples.jar". Alle drei Dateien befinden sich im Verzeichnis "ImaClient/jms/lib". Damit
Sie den JNDI-Provider des Dateisystems verwenden können, wenn Sie die Anwendungen "JMSSampleAdmin"
und "JMSSample" ausführen, benötigen Sie zwei weitere JAR-Dateien im Klassenpfad:
"fscontext.jar" und "providerutil.jar". Diese beiden JAR-Dateien befinden sich ebenfalls im Verzeichnis
"ImaClient/jms/lib".

Kompilierte Beispiele unter Linux ausführen
===========================================
Setzen Sie die Umgebungsvariable CLASSPATH wie folgt, um die Beispiele im Verzeichnis "ImaClient/jms"
auszuführen:
    lib/imaclientjms.jar:lib/jms.jar:lib/jmssamples.jar

Alternativ können Sie CLASSPATH wie folgt setzen, wenn Sie den JNDI-Provider des Dateisystems verwenden möchten:
    lib/imaclientjms.jar:lib/jms.jar:lib/jmssamples.jar:lib/fscontext.jar:lib/providerutil.jar

Mit den folgenden Beispielbefehlszeilen können Sie im Verzeichnis "ImaClient/jms" jede Anwendung
ausführen und einen Verwendungshinweis für die jeweilige Anwendung anzeigen. 

    java -classpath $CLASSPATH com.ibm.ima.samples.jms.JMSSampleAdmin
    java -classpath $CLASSPATH com.ibm.ima.samples.jms.JMSSample
    java -classpath $CLASSPATH com.ibm.ima.samples.jms.HATopicPublisher
    java -classpath $CLASSPATH com.ibm.ima.samples.jms.HADurableSubscriber

Kompilierte Beispiele unter Windows ausführen
=============================================
Setzen Sie die Umgebungsvariable CLASSPATH wie folgt, um die Beispiele im Verzeichnis "ImaClient/jms"
auszuführen:
    lib\imaclientjms.jar;lib\jms.jar;lib\jmssamples.jar

Alternativ können Sie CLASSPATH wie folgt setzen, wenn Sie den JNDI-Provider des Dateisystems verwenden möchten:
    lib\imaclientjms.jar;lib\jms.jar;lib\jmssamples.jar;lib\fscontext.jar;lib\providerutil.jar 

Mit den folgenden Beispielbefehlszeilen können Sie im Verzeichnis "ImaClient/jms" jede Anwendung
ausführen und einen Verwendungshinweis für die jeweilige Anwendung anzeigen. 

    java -classpath %CLASSPATH% com.ibm.ima.samples.jms.JMSSampleAdmin
    java -classpath %CLASSPATH% com.ibm.ima.samples.jms.JMSSample
    java -classpath %CLASSPATH% com.ibm.ima.samples.jms.HATopicPublisher
    java -classpath %CLASSPATH% com.ibm.ima.samples.jms.HADurableSubscriber
    
Wenn Sie die Beispielanwendungen lokal erstellen möchten, sind nur die Dateien "jms.jar"
und "imaclientjms.jar" im Buildklassenpfad erforderlich. 


JMS-CLIENT IN ECLIPSE IMPORTIEREN
=================================
Führen Sie die folgenden Schritte aus, um IBM MessageSight JMS Client
in Eclipse zu importieren:
1. Entpacken Sie die Datei "ImaJmsClientV2.0.zip".
2. Wählen Sie in Eclipse "Datei -> Importieren -> Allgemein -> Vorhandene Projekte in Arbeitsbereich" aus.
3. Klicken Sie auf "Weiter".
4. Wählen Sie das Optionsfeld "Stammverzeichnis auswählen" aus.
5. Klicken Sie auf "Durchsuchen".
6. Navigieren Sie zum Unterverzeichnis "jms" in dem aus der ZIP-Datei extrahierten Inhalt
   und wählen Sie es aus.
7. Wenn das Kontrollkästchen für "Projekte in Arbeitsbereich kopieren" ausgewählt ist,
   entfernen Sie das Häkchen.
8. Klicken Sie auf "Fertigstellen".

Wenn Sie diese Schritte ausgeführt haben, können Sie die Beispielanwendungen und Ihre eigenen Clientanwendungen
kompilieren und in Eclipse ausführen.


EINSCHRÄNKUNGEN UND BEKANNTE PROBLEME
=====================================
1. Die IBM IoT MessageSight-JMS-Implementierung stellt Nachrichten in der Reihenfolge zu,
in der sie empfangen werden. Die Einstellungen für die Nachrichtenpriorität haben keinen
Einfluss auf die Reihenfolge der Zustellung.

2. IBM IoT MessageSight Resource Adapter wird in WebSphere Application Server Version 8.0
oder höher auf allen Plattformen mit Ausnahme von z/OS® unterstützt.

BEMERKUNGEN
===========
1. IBM, das IBM Logo, ibm.com und MessageSight sind Marken oder eingetragene
Marken der IBM Corporation in den USA und/oder anderen Ländern. Linux ist eine
eingetragene Marke von Linus Torvalds in den USA und/oder anderen Ländern. 
Weitere Produkt- und Servicenamen können Marken von IBM oder anderen Unternehmen
sein. Eine aktuelle Liste der IBM Marken finden Sie auf der Webseite "Copyright
and trademark information" unter www.ibm.com/legal/copytrade.shtml. 

2. Java und alle auf Java basierenden Marken und Logos sind Marken oder
eingetragene Marken der Oracle Corporation und/oder ihrer verbundenen Unternehmen.
