// By default we build on the oldest supported distro (centos7)
// A Jenkinsfile used for other distros (with associated build container Dockerfiles)
// along with other jenkins files we use are all in server_build/buildcontainer
//
def distro = "centos7"
def buildImage = "1.0.0.9"

pipeline {
  agent none

  stages {
        stage("Pre") {
            when {
              not {
                branch 'main'
              }
            }
            agent any 
            steps {
                script {
                    echo "default values ${distro} ${buildImage}"
                    distro2 = sh (returnStdout: true, script: ''' 
                        x=0
                        message=`git log -1 --skip=$x --pretty=%B`
                        if [[ "$message" =~ [[]distro=([A-Za-z0-9]+)[]] ]]
                        then 
                            echo ${BASH_REMATCH[1]} 
                        else 
                            echo "'''+distro+'''"
                        fi
                    '''
                    ).trim()
                    buildImage2 = sh (returnStdout: true, script: ''' 
                        x=0
                        message=`git log -1 --skip=$x --pretty=%B`
                        if [[ "$message" =~ [[]buildImage=([A-Za-z0-9.-]+)[]] ]] 
                        then 
                            echo ${BASH_REMATCH[1]} 
                        else 
                            echo "'''+buildImage+'''"
                        fi
                    '''
                    ).trim()
                    echo "updating linux distribution: ${distro} -> ${distro2}."
                    echo "updating build image: ${buildImage} -> ${buildImage2}."
                    distro=distro2
                    buildImage=buildImage2
                    echo "selecting linux distribution: ${distro}."
                    echo "selecting build image: ${buildImage}."
                    env
                }
            }
        }
          
        stage("Run") {
            agent {
                kubernetes {
                label "amlen-${distro}-build-pod"
                yaml """
apiVersion: v1
kind: Pod
spec:
  containers:
  - name: amlen-${distro}-build
    image: quay.io/amlen/amlen-builder-${distro}:${buildImage}
    imagePullPolicy: Always
    command:
    - cat
    tty: true
    resources:
      limits:
        memory: "4Gi"
        cpu: "2"
      requests:
        memory: "4Gi"
        cpu: "2"
    volumeMounts:
    - mountPath: /dev/shm
      name: dshm
    - name: volume-known-hosts
      mountPath: /home/jenkins/.ssh
  - name: jnlp
    volumeMounts:
    - name: volume-known-hosts
      mountPath: /home/jenkins/.ssh
  volumes:
  - name: volume-known-hosts
    configMap:
      name: known-hosts
  - name: dshm
    emptyDir:
      medium: Memory
"""
                }
            } 
            stages{
                stage("init") {
                    steps {
                        container("amlen-${distro}-build") {
                            script {
                                if (env.BUILD_TIMESTAMP == null) {
                                    env.BUILD_TIMESTAMP = sh(script: "date +%Y%m%d-%H%M", returnStdout: true).toString().trim()
                                }
                                if (env.BUILD_LABEL == null ) {
                                    env.BUILD_LABEL = "${env.BUILD_TIMESTAMP}_eclipse${distro}"
                                }
                            }
                            echo "In Init, BUILD_LABEL is ${env.BUILD_LABEL}"    
                            echo "COMMIT: ${env.GIT_COMMIT}"
                        }
                    }
                }
                stage('Build') {
                    steps {
                        echo "In Build, BUILD_LABEL is ${env.BUILD_LABEL}"
                        container("amlen-${distro}-build") {
                           script {
                               try {
                                   sh '''
                                       set -e
                                       pwd 
                                       distro='''+distro+'''
                                       free -m 
                                       cd server_build 
                                       bash buildcontainer/build.sh

                                       cd ..
                                       pwd
                                       ls rpms
                                       if [ ! -e rpms/EclipseAmlenBridge-${distro}-*-${BUILD_LABEL}.tar.gz ]
                                       then 
                                         echo "Bridge Not built"
                                         exit 1
                                       fi
                                       if [ ! -e rpms/EclipseAmlenServer-${distro}-*-${BUILD_LABEL}.tar.gz ]
                                       then 
                                         echo "Server Not built"
                                         exit 1
                                       fi
                                       if [ ! -e rpms/EclipseAmlenWebUI-${distro}-*-${BUILD_LABEL}.tar.gz ]
                                       then 
                                         echo "WebUI not built"
                                         exit 1
                                       fi
                                      '''
                               }
                               catch (Exception e) {
                                   echo "Exception: " + e.toString()
                                   sshagent ( ['projects-storage.eclipse.org-bot-ssh']) {
                                       sh '''
                                           distro='''+distro+'''
                                           NOORIGIN_BRANCH=${GIT_BRANCH#origin/} # turns origin/master into master
                                           ssh -o BatchMode=yes genie.amlen@projects-storage.eclipse.org mkdir -p /home/data/httpd/download.eclipse.org/amlen/snapshots/${NOORIGIN_BRANCH}/${BUILD_LABEL}/${distro}/
                                           scp -o BatchMode=yes -r $BUILDLOG genie.amlen@projects-storage.eclipse.org:/home/data/httpd/download.eclipse.org/amlen/snapshots/${NOORIGIN_BRANCH}/${BUILD_LABEL}/${distro}/
                                       '''
                                   }
                                   currentBuild.result = 'FAILURE'
                               }
                           }
                        }
                    }
                }
                stage('Upload') {
                    steps {
                        container('jnlp') {
                            echo "In Upload, BUILD_LABEL is ${env.BUILD_LABEL}"
                              sshagent ( ['projects-storage.eclipse.org-bot-ssh']) {
                                  sh '''
                                      pwd
                                      distro='''+distro+'''
                                      NOORIGIN_BRANCH=${GIT_BRANCH#origin/} # turns origin/master into master
                                      ssh -o BatchMode=yes genie.amlen@projects-storage.eclipse.org mkdir -p /home/data/httpd/download.eclipse.org/amlen/snapshots/${NOORIGIN_BRANCH}/${BUILD_LABEL}/${distro}/
                                      scp -o BatchMode=yes -r rpms/*.tar.gz genie.amlen@projects-storage.eclipse.org:/home/data/httpd/download.eclipse.org/amlen/snapshots/${NOORIGIN_BRANCH}/${BUILD_LABEL}/${distro}/
                                  '''
                            }
                        }
                    }
                }
            } 
         }
	 stage('Deploy') {
             agent any
	     steps {
		 container('jnlp') {
		     echo "In Deploy, BUILD_LABEL is ${env.BUILD_LABEL}"
		     withCredentials([string(credentialsId: 'quay.io-token', variable: 'QUAYIO_TOKEN'),string(credentialsId:'github-bot-token',variable:'GITHUB_TOKEN')]) {
		       sshagent ( ['projects-storage.eclipse.org-bot-ssh']) {
			   sh '''
			       pwd
			       distro='''+distro+'''
			       NOORIGIN_BRANCH=${GIT_BRANCH#origin/} # turns origin/master into master
			   '''
                       }
                     }
                 }
	   }
	}
        stage("MakeBundle") {
            agent {
                kubernetes {
                label "amlen-${distro}-build-pod"
                yaml """
apiVersion: v1
kind: Pod
spec:
  containers:
  - name: amlen-${distro}-build
    image: quay.io/amlen/amlen-builder-${distro}:${buildImage}
    imagePullPolicy: Always
    command:
    - cat
    tty: true
    resources:
      limits:
        memory: "4Gi"
        cpu: "2"
      requests:
        memory: "4Gi"
        cpu: "2"
    volumeMounts:
    - mountPath: /dev/shm
      name: dshm
    - name: volume-known-hosts
      mountPath: /home/jenkins/.ssh
  - name: jnlp
    volumeMounts:
    - name: volume-known-hosts
      mountPath: /home/jenkins/.ssh
  volumes:
  - name: volume-known-hosts
    configMap:
      name: known-hosts
  - name: dshm
    emptyDir:
      medium: Memory
"""
                }
            } 
            stages{
                stage("init") {
                    steps {
                       container("amlen-${distro}-build") {
                           sshagent ( ['projects-storage.eclipse.org-bot-ssh']) {
                               sh '''
                                   set -e
                                   pwd 
                             '''
                          }
                      }
                   } 
                }
                stage("UploadBundle") {
                    steps {
                        container("jnlp") {
                              sshagent ( ['projects-storage.eclipse.org-bot-ssh']) {
                                   sh '''
                                       set -e
                                       distro='''+distro+'''
                                       pwd 
                                      '''
                               }
                        }
                    }
                }
            }
        }
        stage("BuildBundle") {
            agent any
	    steps {
		echo "In Bundle, BUILD_LABEL is ${env.BUILD_LABEL}"

		container("jnlp") {
                    withCredentials([string(credentialsId: 'quay.io-token', variable: 'QUAYIO_TOKEN')]) {
                      sshagent ( ['projects-storage.eclipse.org-bot-ssh']) {
			   sh '''
			       set -e
			       distro='''+distro+'''
			       pwd
			      '''
		      }
                    }
                }
            }
        }
    }
    post {
        // send a mail on unsuccessful and fixed builds
        unsuccessful { // means unstable || failure || aborted
            emailext subject: 'Build $BUILD_STATUS $PROJECT_NAME #$BUILD_NUMBER!', 
            body: '''Check console output at $BUILD_URL to view the results.''',
            recipientProviders: [culprits(), requestor()], 
            to: 'levell@uk.ibm.com'
        }
        fixed { // back to normal
            emailext subject: 'Build $BUILD_STATUS $PROJECT_NAME #$BUILD_NUMBER!', 
            body: '''Check console output at $BUILD_URL to view the results.''',
            recipientProviders: [culprits(), requestor()], 
            to: 'levell@uk.ibm.com'
        }
    }
}
