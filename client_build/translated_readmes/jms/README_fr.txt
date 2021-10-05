IBM® IoT MessageSight™ JMS Client version 2.0
Juin 2016

Table des matières
1. Description
2. Contenu du package IBM IoT MessageSight JMS Client
3. Utilisation des modèles d'application
4. Importation du client JMS dans Eclipse
5. Limitations et problèmes connus


DESCRIPTION
===========

Le présent fichier README pour le package IBM IoT MessageSight JMS Client contient des
informations sur le contenu, les mises à jour, les correctifs, les limitations et les problèmes connus.

Pour plus de détails sur IBM IoT MessageSight, reportez-vous à la documentation du produit
dans l'IBM Knowledge Center :
http://ibm.biz/iotms_v20

NOUVEAUTES DE CETTE EDITION
===========================

Cette édition ne contient aucune nouvelle fonction IBM MessageSight JMS Client. 

CONTENU DU PACKAGE IBM IoT MESSAGESIGHT JMS CLIENT
==================================================

Le package ImaJmsClientV2.0.zip fournit l'implémentation du client IBM MessageSight
de Java Messaging Service (JMS) et de l'adaptateur de ressources IBM IoT MessageSight.
Il contient également la documentation relative aux classes qui servent à créer et
configurer des objets gérés par JMS en vue de leur utilisation dans IBM IoT MessageSight.
Enfin, il inclut les composants source et des exemples d'élément.

Répertoires et fichiers :

    ImaClient/
        README.txt : le présent fichier

        license/ : contient les fichiers des contrats de licence d'IBM IoT
                   MessageSight JMS Client et de l'adaptateur de ressources IBM IoT MessageSight

        jms/
            version.txt : contient les informations de version d'IBM IoT MessageSight JMS Client

            .classpath et .project : fichiers de configuration de projet Eclipse qui permettent
                                     l'importation du sous-répertoire jms en tant que projet Eclipse
            	
            doc/ : contient la documentation relative à la création et la configuration
                   d'objets gérés par JMS pour IBM IoT MessageSight

            samples/ : contient des modèles d'application (composants source)
                       illustrant la création d'objets gérés, l'envoi et la réception de
                       messages via IBM IoT MessageSight et l'écriture d'applications
                       client compatibles avec des configurations à haute disponibilité

            lib/
                imaclientjms.jar : implémentation IBM IoT MessageSight de l'interface JMS
                jms.jar : fichier JAR de l'interface JMS 1.1
                jmssamples.jar : classes compilées (exemples d'élément) de
                                 l'exemple de code (composants source) fourni avec
                                 IBM IoT MessageSight JMS Client
                fscontext.jar et providerutil.jar : fichiers JAR qui implémentent un
                                                    fournisseur JNDI de système de fichiers

         ImaResourceAdapter/
            version.txt : contient les informations de version de l'adaptateur de
                          ressources IBM IoT MessageSight

            ima.jmsra.rar : archive de l'adaptateur de ressources IBM IoT MessageSight

UTILISATION DES MODELES D'APPLICATION
=====================================

Quatre modèles d'application sont fournis avec le client. Il s'agit de :
    JMSSampleAdmin
    JMSSample
    HATopicPublisher
    HADurableSubscriber

L'application JMSSampleAdmin montre comment créer, configurer et stocker
des objets gérés par JMS pour IBM IoT MessageSight. Elle lit les configurations
d'objet géré par JMS pour IBM IoT MessageSight à partir d'un fichier d'entrée et
remplit un référentiel JNDI. Elle peut alimenter des référentiels LDAP ou des
référentiels JNDI de système de fichiers.  

L'application JMSSample montre comment envoyer et recevoir des messages à partir de
rubriques et de files d'attente IBM IoT MessageSight. Elle est
implémentée dans trois classes : JMSSample, JMSSampleSend et JMSSampleReceive.
Elle peut extraire des objets gérés d'un référentiel JNDI ou créer et
configurer les objets gérés requis lors de la phase d'exécution.

Les applications HATopicPublisher et HADurableSubscriber montrent comment permettre aux
applications client JMS d'utiliser les fonctions à haute disponibilité d'IBM IoT MessageSight.

Pour exécuter les modèles d'application fournis avec le client, trois fichiers JAR
sont requis dans le chemin d'accès aux classes : imaclientjms.jar, jms.jar et
jmssamples.jar. Ces trois fichiers se trouvent dans ImaClient/jms/lib. Pour pouvoir
utiliser le fournisseur JNDI du système de fichiers lors de l'exécution des applications
JMSSampleAdmin et JMSSample, deux fichiers JAR supplémentaires sont requis dans le
chemin d'accès aux classes : fscontext.jar et providerutil.jar. Ces deux fichiers JAR sont
également disponibles dans ImaClient/jms/lib.

Exécution des exemples compilés sous Linux :
============================================

A partir d'ImaClient/jms, définissez la variable d'environnement CLASSPATH
comme suit :
    lib/imaclientjms.jar:lib/jms.jar:lib/jmssamples.jar

Ou, si vous voulez utiliser le fournisseur JNDI du système de fichiers, associez CLASSPATH à :
    lib/imaclientjms.jar:lib/jms.jar:lib/jmssamples.jar:lib/fscontext.jar:lib/providerutil.jar

A partir d'ImaClient/jms, les lignes de commande de l'exemple ci-après exécutent chaque
application et indiquent la syntaxe pour chacune.

    java -classpath $CLASSPATH com.ibm.ima.samples.jms.JMSSampleAdmin
    java -classpath $CLASSPATH com.ibm.ima.samples.jms.JMSSample
    java -classpath $CLASSPATH com.ibm.ima.samples.jms.HATopicPublisher
    java -classpath $CLASSPATH com.ibm.ima.samples.jms.HADurableSubscriber

Exécution des exemples compilés sous Windows :
==============================================

A partir d'ImaClient/jms, définissez la variable d'environnement CLASSPATH
comme suit :
    lib\imaclientjms.jar;lib\jms.jar;lib\jmssamples.jar

Ou, si vous voulez utiliser le fournisseur JNDI du système de fichiers, associez CLASSPATH à :
    lib\imaclientjms.jar;lib\jms.jar;lib\jmssamples.jar;lib\fscontext.jar;lib\providerutil.jar

A partir d'ImaClient/jms, les lignes de commande de l'exemple ci-après exécutent chaque
application et indiquent la syntaxe pour chacune.

    java -classpath %CLASSPATH% com.ibm.ima.samples.jms.JMSSampleAdmin
    java -classpath %CLASSPATH% com.ibm.ima.samples.jms.JMSSample
    java -classpath %CLASSPATH% com.ibm.ima.samples.jms.HATopicPublisher
    java -classpath %CLASSPATH% com.ibm.ima.samples.jms.HADurableSubscriber
    
Pour générer les modèles d'application localement, seuls jms.jar et imaclientjms.jar sont
requis dans le chemin d'accès aux classes de génération.


IMPORTATION DU CLIENT JMS DANS ECLIPSE
======================================

Pour importer IBM IoT MessageSight JMS Client dans Eclipse, procédez
comme suit :
1. Décompressez le contenu d'ImaJmsClientV2.0.zip
2. Dans Eclipse, sélectionnez Fichier->Importer->Général->Projets existants dans l'espace de travail
3. Cliquez sur Suivant
4. Choisissez le bouton d'option "Sélectionner le répertoire racine"
5. Cliquez sur Parcourir
6. Accédez au sous-répertoire jms dans le contenu des fichiers zip décompressés et sélectionnez-le
7. Si la case "Copier les projets dans l'espace de travail" est cochée, désélectionnez-la
8. Cliquez sur Terminer

Une fois que vous avez effectué ces étapes, vous pouvez compiler et exécuter les modèles d'application
et vos propres applications client à partir d'Eclipse.


LIMITATIONS ET PROBLEMES CONNUS
===============================

1. L'implémentation d'IBM IoT MessageSight JMS distribue les messages suivant l'ordre
dans lequel ils ont été reçus. Les paramètres de priorité des messages n'ont aucun impact
sur l'ordre de distribution.

2. L'adaptateur de ressources IBM IoT MessageSight est pris en charge dans WebSphere
Application Server version 8.0 (ou version ultérieure), sur toutes les plateformes, à
l'exception de z/OS®.

REMARQUES
=========

1. IBM, le logo IBM, ibm.com et MessageSight sont des marques d'International Business
Machines Corp., dans de nombreux pays. Linux est une marque de Linus Torvalds aux Etats-Unis
et/ou dans certains autres pays. Les autres noms de produits et de services peuvent
être des marques d'IBM ou d'autres sociétés. La liste actualisée de toutes les marques
d'IBM est disponible sur la page Web "Copyright and trademark information"
à www.ibm.com/legal/copytrade.shtml. 

2. Java ainsi que tous les logos et toutes les marques incluant Java sont des marques
d'Oracle et/ou de ses sociétés affiliées.
	
