IBM IoT MessageSight Protocol Plug-in SDK version 2.0
Juin 2016

Sommaire
1. Description
2. Contenu du kit de développement de logiciels du plug-in de protocole IBM IoT MessageSight
3. Utilisation de l'exemple de code d'échantillon de plug-in de protocole JSON
4. Importation du kit de développement de logiciels et des échantillons de l'exemple de
   plug-in de protocole JSON
5. Utilisation de l'exemple de code d'échantillon de plug-in de protocole REST
6. Importation du kit de développement de logiciels et des échantillons de l'exemple de
   plug-in de protocole REST
7. Limites et problèmes connus


DESCRIPTION
===========
Ce fichier README du kit de développement de logiciels du plug-in de protocole IBM
IoT MessageSight contient des informations sur le contenu, les mises à jour, les correctifs,
les limites et les problèmes connus.
Ce kit de développement de logiciels vous permet d'étendre le jeu de protocoles
pris en charge de manière native par le serveur IBM IoT MessageSight en écrivant des
plug-ins dans le langage de programmation Java. 

Pour plus de détails sur IBM IoT MessageSight, reportez-vous à la documentation du produit dans
l'IBM Knowledge Center :
http://ibm.biz/iotms_v20

NOUVEAUTES DE CETTE VERSION
===========================
Le kit de développement de logiciels de plug-in de protocole IBM IoT MessageSight est désormais
intégralement pris en charge dans les environnements autres que les environnements à
haute disponibilité.
Pour connaître les restrictions d'utilisation, reportez-vous à sa licence.
Le déploiement et la configuration du plug-in sont effectués via une API RESTful

CONTENU DU KIT DE DEVELOPPEMENT DE LOGICIELS DU PLUG-IN DE PROTOCOLE IBM IoT MESSAGESIGHT
=========================================================================================
Le sous-répertoire ImaTools/ImaPlugin du package ProtocolPluginV2.0.zip
fournit le kit de développement de logiciels de plug-in de protocole IBM IoT MessageSight.
Il fournit également la documentation des classes utilisées pour créer des plug-ins de
protocole à utiliser avec IBM IoT MessageSight. Enfin, il inclut les composants source
et des exemples de ressources.

Répertoires et fichiers :
    ImaTools/
        license/ : contient les fichiers de contrat de licence de la fonction de plug-in
            de protocole IBM IoT MessageSight
    ImaPlugin/
            README.txt : le présent fichier

            version.txt : contient les informations de version du kit de développement de
                logiciels du plug-in de protocole IBM IoT MessageSight


            doc/ : contient la documentation permettant de créer des plug-ins de protocole
                à l'aide du kit de développement de logiciels du plug-in de protocole
                IBM IoT MessageSight

            lib/
                imaPlugin.jar : kit de développement de logiciels du plug-in de protocole
                    IBM IoT MessageSight

                jsonplugin.zip : archive zip contenant un exemple de plug-in de protocole
                    JSON pouvant être développé dans IBM IoT MessageSight.
                    Cette archive contient les classes compilées (exemples de ressource)
                    de l'exemple de code de plug-in (composants source) fourni dans le
                    sous-répertoire samples/jsonmsgPlugin.

                jsonprotocol.jar : classes compilées (exemples de ressource) de
                l'exemple de plug-in. Ce fichier jar se trouve également dans le
                fichier jsonplugin.zip.
                
                restplugin.zip : archive zip contenant un exemple de plug-in de
                protocole REST pouvant être développé dans IBM IoT MessageSight.
                Cette archive contient les classes compilées (exemples de ressource)
                de l'exemple de code de plug-in (composants source) fourni dans
                le sous-répertoire samples/restmsgPlugin.

                restprotocol.jar : classes compilées (exemples de ressource) de
                l'exemple de plug-in de protocole REST. Ce fichier jar se trouve
                également dans le fichier restplugin.zip.
                
            samples/
                jsonmsgPlugin/
                    .classpath et .project : fichiers de configuration du projet Eclipse
                    qui permettent d'importer le sous-répertoire jsonmsgPlugin en tant
                    que projet Eclipse.
            	
            	    build_plugin_zip.xml : fichier de génération Ant que vous pouvez
                    utiliser pour créer une archive de plug-in.
            	    
                    src/ : contient le code source Java d'un exemple de plug-in
                        (Composants source) expliquant comment écrire un plug-in
                        de protocole à utiliser avec IBM IoT MessageSight

                    config/ : contient le fichier de descripteur JSON (Composants source)
                        requis pour déployer l'exemple de plug-in compilé dans IBM IoT MessageSight

                    doc/ : contient la documentation du code source de l'exemple de plug-in

                jsonmsgClient/
                    .classpath et .project : fichiers de configuration du projet Eclipse
                    qui permettent d'importer le sous-répertoire jsonmsgClient en tant
                    que projet Eclipse.
            	    
            	    JSONMsgWebClient.html : application client (Composants
            	        source) de l'exemple de plug-in fourni dans
            	        jsonmsgPlugin. Ce client dépends de la bibliothèque JavaScript
            	        dans le sous-répertoire js.
            	                
                    js/ : contient l'exemple de bibliothèque client JavaScript (Composants
                        source) à utiliser avec l'exemple de plug-in fourni dans
                        jsonmsgPlugin.
                    
                    css/ : contient les feuilles de style (Composants source) et les
                        fichiers image utilisés par l'exemple d'application client et
                        l'exemple de bibliothèque client.
                
                    doc/ : contient la documentation de l'exemple de
                        bibliothèque client JavaScript.
                    
                restmsgPlugin/
                    .classpath et .project : fichiers de configuration du projet Eclipse
                    qui permettent d'importer le sous-répertoire restmsgPlugin en tant
                    que projet Eclipse.
                
                    build_plugin_zip.xml : fichier de génération Ant que vous pouvez
                    utiliser pour créer une archive de plug-in.
                    
                    src/ : contient le code source Java d'un exemple de plug-in
                        (Composants source) expliquant comment écrire un plug-in
                        de protocole à utiliser avec IBM IoT MessageSight

                    config/ : contient le fichier de descripteur JSON (Composants source)
                        requis pour déployer l'exemple de plug-in compilé dans IBM IoT MessageSight

                    doc/ : contient la documentation du code source de l'exemple de plug-in
                       
                restmsgClient/
                    .classpath et .project : fichiers de configuration du projet Eclipse
                    qui permettent d'importer le sous-répertoire restmsgClient en tant
                    que projet Eclipse.
                    
                    RESTMsgWebClient.html : application client (Composants
            	        source) de l'exemple de plug-in fourni dans
            	        restmsgPlugin. Ce client dépends de la bibliothèque JavaScript
            	        dans le sous-répertoire js.
                                
                    js/ : contient l'exemple de bibliothèque client JavaScript (Composants
                        source) à utiliser avec l'exemple de plug-in fourni dans
                        restmsgPlugin.
                    
                    css/ : contient les feuilles de style (Composants source) et les
                        fichiers image utilisés par l'exemple d'application client et
                        l'exemple de bibliothèque client.
                
                    doc/ : contient la documentation de l'exemple de
                        bibliothèque client JavaScript.
                  
UTILISATION DE L'EXEMPLE DE PLUG-IN DE PROTOCOLE JSON ET D'APPLICATION CLIENT
====================================================================
L'application JSONMsgWebClient.html utilise la bibliothèque client JavaScript jsonmsg.js
pour communiquer avec le plug-in json_msg. L'application JSONMsgWebClient.html fournit
une interface Web pour envoyer les messages de qualité de service 0 au plug-in json_msg
et les recevoir de ce plug-in (au maximum une fois).

L'exemple de plug-in de protocole json_msg est composé de deux classes, JMPlugin et
JMConnection. JMPlugin implémente l'interface ImaPluginListener requise pour
initialiser et lancer un plug-in de protocole. JMConnection implémente l'interface
ImaConnectionListener, qui permet aux clients de se connecter à IBM IoT
MessageSight et d'envoyer et de recevoir des messages via le protocole json_message.
La classe JMConnection reçoit les objets JSON des clients de publication et utilise
la classe d'utilitaire ImaJson pour analyser ces objets dans les commandes et messages
qu'elle envoie au serveur IBM IoT MessageSight. De même, la classe JMConnection
utilise la classe d'utilitaire ImaJson pour convertir les messages IBM IoT MessageSight en
objets JSON pour les clients json_msg qui s'abonnent. Enfin, le plug-in de protocole json_msg
inclut un fichier de descripteur intitulé plugin.json. Ce fichier de descripteur est
requis dans l'archive zip utilisée pour déployer un plug-in de protocole et doit toujours
utiliser ce nom de fichier. Un exemple d'archive de plug-in pour le protocole
json_msg, jsonplugin.zip, est inclus dans le répertoire ImaTools/ImaPlugin/lib. Il contient
un fichier JAR (jsonprotocol.jar avec les classes JMPlugin
et JMConnection compilées) et le fichier de descripteur plugin.json.

Pour exécuter l'exemple d'application client JavaScript, vous devez au préalable déployer
l'exemple de plug-in dans IBM IoT MessageSight. Le plug-in doit être archivé dans un fichier zip
ou jar. Ce fichier doit contenir le ou les fichiers jar qui implémentent le
plug-in du protocole cible. Il doit également contenir un fichier de descripteur JSON
qui décrit le contenu du plug-in. Une archive intitulée jsonplugin.zip est
disponible dans ImaTools/ImaPlugin/lib et contient l'exemple de plug-in json_msg.
Une fois que l'exemple d'archive de plug-in a été déployé, vous pouvez exécuter l'exemple
d'application client en chargeant le fichier JSONMsgWebClient.html (dans
ImaTools/ImaPlugin/samples/jsonmsgClient) dans un navigateur Web. Vous devez conserver
la structure de sous-répertoires dans la quelle se trouve le fichier JSONMsgWebClient.html pour
charger l'exemple de bibliothèque client JavaScript et les feuilles de style de
l'exemple d'application.


Déploiement de l'exemple d'archive de plug-in dans IBM IoT MessageSight :
=======================================================================
Pour déployer le fichier jsonplugin.zip sur IBM IoT MessageSight, vous devez utiliser
la méthode PUT pour importer le fichier jsonmsg.zip du plug-in à l'aide de cURL :
	curl -X PUT -T jsonmsg.zip http://127.0.0.1:9089/ima/v1/file/jsonmsg.zip

Vous devez ensuite créer le plug-in à l'aide de la méthode POST pour créer un plug-in
de protocole intitulé jsonmsg, à l'aide de cURL :
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

Enfin, vous devez redémarrer le serveur de plug-in de protocole IBM IoT MessageSight
pour lancer le plug-in, à l'aide de cURL :
	curl -X POST -d  '{"Service":"Plugin"}' http://127.0.0.1:9089/ima/v1/service/restart

Pour confirmer que le plug-in a été correctement déployé à l'aide de cURL :
    curl -X GET http://127.0.0.1:9089/ima/v1/configuration/Plugin

Vous devez voir json_msg dans la liste résultante. Cette valeur correspond à
la propriété Name spécifiée dans le fichier de descripteur plugin.json.  

Pour confirmer que le nouveau protocole a été correctement enregistré à l'aide de cURL :
    curl -X GET http://127.0.0.1:9089/ima/v1/configuration/Protocol

Vous devez voir json_msg dans la liste résultante. Cette valeur correspond à la propriété
Protocol spécifiée dans le fichier de descripteur plugin.json.

Si vous utilisez le noeud final de démonstration, aucune autre étape n'est requise pour
accéder à l'exemple de plug-in de protocole json_msg. Si vous utilisez un autre noeud final, vous
devrez suivre les mêmes procédures pour spécifier les protocoles autorisés par un
noeud final, une règle de connexion ou une règle de messagerie. Le noeud final de démonstration
et ses règles sont préconfigurés pour autoriser tous les protocoles définis.

Exécution de l'exemple d'application client JavaScript :
======================================================
Ouvrez JSONMsgWebClient.html dans un navigateur Web. Spécifiez le nom d'hôte ou l'adresse IP
d'IBM IoT MessageSight dans la zone de texte Serveur. Si vous utilisez le noeud final de
démonstration, vous pouvez utiliser le port 16102 comme indiqué dans la zone de
texte Port. Sinon, spécifiez le numéro de port correct du noeud final IBM IoT
MessageSight configuré avec l'exemple de plug-in. Pour des options de connexion
supplémentaires, sélectionnez l'onglet Session. Une fois que vous avez fini de définir
les propriétés de connexion, cliquez sur le bouton Connecter pour vous connecter à
IBM IoT MessageSight. Utilisez ensuite l'onglet Rubrique
pour envoyer des messages à des destinations IBM IoT MessageSight ou en recevoir de
ces destinations, via l'exemple de plug-in.


IMPORTATION DU CLIENT JSON_MSG DANS ECLIPSE
===========================================
Pour importer le client IBM IoT MessageSight json_msg dans Eclipse, effectuez les
étapes suivantes :
1. Décompressez le contenu de ProtocolPluginV2.0.zip
2. Dans Eclipse, sélectionnez Fichier->Importer->Général->Projets existant
   dans l'espace de travail
3. Cliquez sur Suivant
4. Choisissez le bouton d'option "Sélectionner le répertoire racine"
5. Cliquez sur Parcourir
6. Accédez au sous-répertoire ImaTools/ImaPlugin/samples/jsonmsgClient
   et sélectionnez-le dans le contenu du fichier zip décompressé
7. Si la case permettant de copier les projets dans l'espace de travail est
   cochée, désélectionnez-la
8. Cliquez sur Terminer

Une fois que vous avez effectué ces étapes, vous pouvez exécuter l'exemple
d'application à partir d'Eclipse.

IMPORTATION DU PLUG-IN JSON_MSG DANS ECLIPSE
============================================
Pour importer le plug-in IBM IoT MessageSight json_msg dans Eclipse, effectuez les
étapes suivantes :
1. Décompressez le contenu de ProtocolPluginV2.0.zip
2. Dans Eclipse, sélectionnez Fichier->Importer->Général->Projets existant
   dans l'espace de travail
3. Cliquez sur Suivant
4. Choisissez le bouton d'option "Sélectionner le répertoire racine"
5. Cliquez sur Parcourir
6. Accédez au sous-répertoire ImaTools/ImaPlugin/samples/jsonmsgPlugin
   et sélectionnez-le dans le contenu du fichier zip décompressé
7. Si la case permettant de copier les projets dans l'espace de travail est
   cochée, désélectionnez-la
8. Cliquez sur Terminer

Une fois que vous avez effectué ces étapes, vous pouvez compiler l'exemple de
plug-in et vos propres plug-ins à partir d'Eclipse.

GENERATION DE L'ARCHIVE DE PLUG-IN DANS ECLIPSE
===============================================
Si vous mettez à jour l'exemple de plug-in json_msg et que vous souhaitez créer une archive
à déployer dans IBM IoT MessageSight, procédez comme suit :
REMARQUE : Ces étapes supposent que vous avez importé le projet jsonmsgPlugin dans
Eclipse et qu'Eclipse a déjà compilé les fichiers source du projet et placé les
fichiers de classe dans le sous-répertoire bin.
1. Dans Eclipse, ouvrez jsonmsgPlugin dans le navigateur ou l'explorateur de package.
2. Cliquez sur build_plugin_zip.xml à l'aide du bouton droit de la souris et
   sélectionnez Run As -> Ant build.
   Un sous-répertoire plugin est créé sous jsonmsgPlugin avec un nouveau
   fichier jsonplugin.zip. Vous pouvez actualiser le projet jsonmsgPlugin pour voir
   le sous-répertoire plugin dans l'interface utilisateur d'Eclipse. 


UTILISATION DE L'EXEMPLE DE PLUG-IN DE PROTOCOLE REST ET D'APPLICATION CLIENT
=============================================================================
L'application RESTMsgWebClient.html utilise la bibliothèque client JavaScript restmsg.js
pour communiquer avec le plug-in restmsg. L'application RESTMsgWebClient.html fournit
une interface Web pour envoyer les messages de qualité de service 0 au plug-in restmsg
et les recevoir de ce plug-in (au maximum une fois).

L'exemple de plug-in de protocole restmsg est composé de deux classes, RestMsgPlugin
et RestMsgConnection. RestMsgPlugin implémente l'interface ImaPluginListener requise pour
initialiser et lancer un plug-in de protocole. RestMsgConnection implémente l'interface
ImaConnectionListener, qui permet aux clients de se connecter à IBM IoT
MessageSight et d'envoyer et de recevoir des messages via le protocole restmsg.
La classe RestMsgConnection reçoit les objets REST des clients de publication et
analyse ces objets dans les commandes et messages qu'elle envoie au serveur IBM IoT MessageSight.
De même, la classe RestMsgConnection convertit les messages IBM IoT
MessageSight en objets REST pour les clients restmsg qui s'abonnent.
Enfin, le plug-in de protocole restmsg inclut un fichier de descripteur
intitulé plugin.json. Ce fichier de descripteur est
requis dans l'archive zip utilisée pour déployer un plug-in de protocole et doit toujours
utiliser ce nom de fichier. Un exemple d'archive de plug-in pour le protocole
restmsg, restplugin.zip, est inclus dans le répertoire ImaTools/ImaPlugin/lib. Il contient
un fichier JAR (restprotocol.jar avec les classes RestMsgPlugin
et RestMsgConnection compilées) et le fichier de descripteur plugin.json.

Pour exécuter l'exemple d'application client JavaScript, vous devez au préalable déployer
l'exemple de plug-in sur IBM IoT MessageSight. Le plug-in doit être archivé dans un fichier zip ou
jar. Ce fichier doit contenir le ou les fichiers jar qui implémentent le
plug-in du protocole cible. Il doit également contenir un fichier de descripteur JSON
qui décrit le contenu du plug-in. Une archive intitulée restplugin.zip est
disponible dans ImaTools/ImaPlugin/lib et contient l'exemple de plug-in restmsg.
Une fois que l'exemple d'archive de plug-in restmsg a été déployé, vous pouvez exécuter l'exemple
d'application client en chargeant le fichier RESTMsgWebClient.html (dans
ImaTools/ImaPlugin/samples/restmsgClient) dans un navigateur Web. Vous devez conserver
la structure de sous-répertoires dans la quelle se trouve le fichier RESTMsgWebClient.html pour
charger l'exemple de bibliothèque client JavaScript et les feuilles de style de
l'exemple d'application.


Déploiement de l'exemple d'archive de plug-in REST dans IBM IoT MessageSight :
============================================================================
Pour déployer le fichier restplugin.zip sur IBM IoT MessageSight, vous devez utiliser
la méthode PUT pour importer le fichier jsonmsg.zip du plug-in à l'aide de cURL :
	curl -X PUT -T restplugin.zip http://127.0.0.1:9089/ima/v1/file/restplugin.zip

Vous devez ensuite créer le plug-in à l'aide de la méthode POST pour créer un plug-in
de protocole intitulé jsonmsg, à l'aide de cURL :
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
   
Enfin, vous devez arrêter et redémarrer le serveur de plug-in de protocole
IBM IoT MessageSight pour lancer le plug-in.

Enfin, vous devez redémarrer le serveur de plug-in de protocole IBM IoT MessageSight
pour lancer le plug-in, à l'aide de cURL :
	curl -X POST -d  '{"Service":"Plugin"}' http://127.0.0.1:9089/ima/v1/service/restart

Pour confirmer que le plug-in a été correctement déployé à l'aide de cURL :
    curl -X GET http://127.0.0.1:9089/ima/v1/configuration/Plugin

Vous devez voir restmsg dans la liste résultante. Cette valeur correspond à
la propriété Name spécifiée dans le fichier de descripteur plugin.json.  

Pour confirmer que le nouveau protocole a été correctement enregistré à l'aide de cURL :
    curl -X GET http://127.0.0.1:9089/ima/v1/configuration/Protocol

Vous devez voir restmsg dans la liste résultante. Cette valeur correspond à la propriété
Protocol spécifiée dans le fichier de descripteur plugin.json.

Si vous utilisez le noeud final de démonstration, aucune autre étape n'est requise pour
accéder à l'exemple de plug-in de protocole restmsg. Si vous utilisez un autre noeud final,
vous devrez suivre les mêmes procédures pour spécifier les protocoles autorisés par un
noeud final, une règle de connexion ou une règle de messagerie. Le noeud final de
démonstration et ses règles sont préconfigurés pour autoriser tous les protocoles définis.

Exécution de l'exemple d'application client JavaScript :
======================================================
Ouvrez RESTMsgWebClient.html dans un navigateur Web. Spécifiez le nom d'hôte ou l'adresse IP
d'IBM IoT MessageSight dans la zone de texte Serveur. Si vous utilisez le noeud final de
démonstration, vous pouvez utiliser le port 16102 comme indiqué dans la zone de texte Port.
Sinon, spécifiez le numéro de port correct du noeud final IBM IoT MessageSight
configuré avec l'exemple de plug-in. Pour des options de connexion supplémentaires,
sélectionnez l'onglet Session. Une fois que vous avez fini de définir les propriétés
de connexion, cliquez sur le bouton Connecter pour vous connecter à
IBM IoT MessageSight. Utilisez ensuite l'onglet Rubrique
pour envoyer des messages à des destinations IBM IoT MessageSight ou en recevoir de
ces destinations, via l'exemple de plug-in.


IMPORTATION DU CLIENT RESTMSG DANS ECLIPSE
==========================================
Pour importer le client IBM IoT MessageSight restmsg dans Eclipse, effectuez les étapes
suivantes :
1. Décompressez le contenu de ProtocolPluginV2.0.zip
2. Dans Eclipse, sélectionnez Fichier->Importer->Général->Projets existant
   dans l'espace de travail
3. Cliquez sur Suivant
4. Choisissez le bouton d'option "Sélectionner le répertoire racine"
5. Cliquez sur Parcourir
6. Accédez au sous-répertoire ImaTools/ImaPlugin/samples/restmsgClient
   et sélectionnez-le dans le contenu du fichier zip décompressé
7. Si la case permettant de copier les projets dans l'espace de travail est
   cochée, désélectionnez-la
8. Cliquez sur Terminer

Une fois que vous avez effectué ces étapes, vous pouvez exécuter l'exemple
d'application à partir d'Eclipse.

IMPORTATION DU PLUG-IN RESTMSG DANS ECLIPSE
===========================================
Pour importer le plug-in IBM IoT MessageSight restmsg dans Eclipse, effectuez les étapes
suivantes :
1. Décompressez le contenu de ProtocolPluginV2.0.zip
2. Dans Eclipse, sélectionnez Fichier->Importer->Général->Projets existant
   dans l'espace de travail
3. Cliquez sur Suivant
4. Choisissez le bouton d'option "Sélectionner le répertoire racine"
5. Cliquez sur Parcourir
6. Accédez au sous-répertoire ImaTools/ImaPlugin/samples/restmsgPlugin
   et sélectionnez-le dans le contenu du fichier zip décompressé
7. Si la case permettant de copier les projets dans l'espace de travail est
   cochée, désélectionnez-la
8. Cliquez sur Terminer

Une fois que vous avez effectué ces étapes, vous pouvez compiler l'exemple de
plug-in et vos propres plug-ins à partir d'Eclipse.

GENERATION DE L'ARCHIVE DE PLUG-IN RESTMSG DANS ECLIPSE
=======================================================
Si vous mettez à jour l'exemple de plug-in restmsg et que vous souhaitez créer une archive
à déployer dans IBM IoT MessageSight, procédez comme suit :
REMARQUE : Ces étapes supposent que vous avez importé le projet restmsgPlugin dans
Eclipse et qu'Eclipse a déjà compilé les fichiers source du projet et placé les
fichiers de classe dans le sous-répertoire bin.
1. Dans Eclipse, ouvrez restmsgPlugin dans le navigateur ou l'explorateur de package.
2. Cliquez sur build_plugin_zip.xml à l'aide du bouton droit de la souris et
   sélectionnez Run As -> Ant build.
   Un sous-répertoire plugin est créé sous restmsgPlugin avec un nouveau
   fichier restplugin.zip. Vous pouvez actualiser le projet restmsgPlugin pour voir
   le sous-répertoire plugin dans l'interface utilisateur d'Eclipse. 
   
SURVEILLANCE DU PROCESSUS DE PLUG-IN
====================================

La taille de segment de mémoire, le débit de récupération de place et la quantité d'UC
utilisée par le processus de plug-in peuvent être surveillés en s'abonnant à la
rubrique $SYS/ResourceStatistics/Plugin.   

LIMITES ET PROBLEMES CONNUS
===========================
1. Aucune interface graphique Web MessageSight n'est prise en charge pour le déploiement
   des plug-ins de protocole. La ligne de commande doit être utilisée.
2. Les abonnements partagés ne sont pas pris en charge pour les plug-ins de protocole.
3. La méthode ImaPluginListener.onAuthenticate() n'est pas appelée par IBM IoT MessageSight.
4. Les configurations à haute disponibilité ne sont pas prises en charge.


REMARQUES
=========
1. IBM, le logo IBM, ibm.com, et MessageSight sont des marques d'International Business
Machines Corp., dans de nombreux pays. Les autres noms de produit et service peuvent
être des marques d'IBM ou d'autres sociétés. La liste actualisée de toutes les marques
d'IBM est disponible dans la page Web "Copyright and trademark information", à l'adresse
www.ibm.com/legal/copytrade.shtml. 

2. Java ainsi que tous les logos et toutes les marques incluant Java sont des marques
d'Oracle et/ou de ses filiales.
