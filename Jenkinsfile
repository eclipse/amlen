// By default we build on the oldest supported distro (alma8)
// A Jenkinsfile used for other distros (with associated build container Dockerfiles)
// along with other jenkins files we use are all in server_build/buildcontainer
//
def distro = "almalinux8"
def buildImage = "1.0.0.7"

pipeline {
  agent none

  stages {
        stage("Pre") {
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
                    echo "In Init, BUILD_LABEL is ${env.BUILD_LABEL}"    
                    echo "COMMIT: ${env.GIT_COMMIT}"
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
                                       free -m 
                                       cd server_build 
                                       if [[ "$BRANCH_NAME" == "main" ]] ; then
                                           export BUILD_TYPE=fvtbuild
                                       fi
                                       bash buildcontainer/build.sh
                                       cd ../operator
                                       NOORIGIN_BRANCH=${GIT_BRANCH#origin/} # turns origin/master into master
                                       export IMG=quay.io/amlen/operator:$NOORIGIN_BRANCH
                                       make bundle
                                       make produce-deployment
                                       pylint --fail-under=5 build/scripts/*.py
                                       cd ../Documentation/doc_infocenter
                                       ant
                                       cd ../../
                                       tar -c client_ship -f client_ship.tar.gz
                                       tar -c server_ship -f server_ship.tar.gz
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
                                      scp -o BatchMode=yes -r Documentation/docs genie.amlen@projects-storage.eclipse.org:/home/data/httpd/download.eclipse.org/amlen/snapshots/${NOORIGIN_BRANCH}/${BUILD_LABEL}/${distro}/
                                      scp -o BatchMode=yes -r client_ship.tar.gz genie.amlen@projects-storage.eclipse.org:/home/data/httpd/download.eclipse.org/amlen/snapshots/${NOORIGIN_BRANCH}/${BUILD_LABEL}/${distro}/
                                      scp -o BatchMode=yes -r server_ship.tar.gz genie.amlen@projects-storage.eclipse.org:/home/data/httpd/download.eclipse.org/amlen/snapshots/${NOORIGIN_BRANCH}/${BUILD_LABEL}/${distro}/
          
                                      sed -i "s/IMG_TAG/$NOORIGIN_BRANCH/" operator/roles/amlen/defaults/main.yml
                                      cd operator
                                      tar -czf operator.tar.gz Dockerfile requirements.yml roles watches.yaml
                                      scp -o BatchMode=yes -r operator.tar.gz genie.amlen@projects-storage.eclipse.org:/home/data/httpd/download.eclipse.org/amlen/snapshots/${NOORIGIN_BRANCH}/${BUILD_LABEL}/${distro}/
                                      mv bundle.Dockerfile Dockerfile
          
                                      tar -czf operator_bundle.tar.gz Dockerfile bundle
                                      scp -o BatchMode=yes -r operator_bundle.tar.gz genie.amlen@projects-storage.eclipse.org:/home/data/httpd/download.eclipse.org/amlen/snapshots/${NOORIGIN_BRANCH}/${BUILD_LABEL}/${distro}/
          
                                      scp -o BatchMode=yes -r eclipse-amlen-operator.yaml genie.amlen@projects-storage.eclipse.org:/home/data/httpd/download.eclipse.org/amlen/snapshots/${NOORIGIN_BRANCH}/${BUILD_LABEL}/${distro}/
                                  '''
                            }
                        }
                    }
                }
            } 
         }
	 stage('Deploy') {
	     steps {
		 container('jnlp') {
		     echo "In Deploy, BUILD_LABEL is ${env.BUILD_LABEL}"
		     withCredentials([string(credentialsId: 'quay.io-token', variable: 'QUAYIO_TOKEN'),string(credentialsId: 'bvt-token', variable: 'BVT_KEY'),string(credentialsId:'github-bot-token',variable:'GITHUB_TOKEN')]) {
		       sshagent ( ['projects-storage.eclipse.org-bot-ssh']) {
			   sh '''
			       pwd
			       distro='''+distro+'''
			       NOORIGIN_BRANCH=${GIT_BRANCH#origin/} # turns origin/master into master
 
			       c1=$(curl -X POST https://quay.io/api/v1/repository/amlen/amlen-server/build/ -H \"Authorization: Bearer ${QUAYIO_TOKEN}\" -H \"Content-Type: application/json\" -d \"{ \\\"archive_url\\\":\\\"https://download.eclipse.org/amlen/snapshots/${NOORIGIN_BRANCH}/${BUILD_LABEL}/${distro}/EclipseAmlenServer-${distro}-1.1dev-${BUILD_LABEL}.tar.gz\\\", \\\"docker_tags\\\":[\\\"${NOORIGIN_BRANCH}\\\"] }\" )
			       uid1=$(echo ${c1} | grep -oP '(?<=\"id\": \")[^\"]*\')
			       sleep 60
   
			       c2=$(curl -X POST https://quay.io/api/v1/repository/amlen/operator/build/ -H \"Authorization: Bearer ${QUAYIO_TOKEN}\" -H \"Content-Type: application/json\" -d \"{ \\\"archive_url\\\":\\\"https://download.eclipse.org/amlen/snapshots/${NOORIGIN_BRANCH}/${BUILD_LABEL}/${distro}/operator.tar.gz\\\", \\\"docker_tags\\\":[\\\"${NOORIGIN_BRANCH}\\\"] }\") 
			       uid2=$(echo ${c2} | grep -oP '(?<=\"id\": \")[^\"]*\')
			       sleep 60
   
			       c3=$(curl -X POST https://quay.io/api/v1/repository/amlen/operator-bundle/build/ -H \"Authorization: Bearer ${QUAYIO_TOKEN}\" -H \"Content-Type: application/json\" -d \"{ \\\"archive_url\\\":\\\"https://download.eclipse.org/amlen/snapshots/${NOORIGIN_BRANCH}/${BUILD_LABEL}/${distro}/operator_bundle.tar.gz\\\", \\\"docker_tags\\\":[\\\"${NOORIGIN_BRANCH}\\\"] }\")
			       uid3=$(echo ${c3} | grep -oP '(?<=\"id\": \")[^\"]*\')
                                      sleep 60
        
			       for uid in "$uid1 amlen-server" "$uid2 operator" "$uid3 operator-bundle" 
			       do
				 set -- $uid
				 for i in {1..45}
				 do
				   phase=$(curl -s https://quay.io/api/v1/repository/amlen/$2/build/$1)
				   phase=$(echo $phase | grep -oP '(?<=\"phase\": \")[^\"]*')
				   if [[ 'complete' == $phase ]]
				   then
				     break
				   fi
				   sleep 10
				 done
			       
				 phase=$(curl -s https://quay.io/api/v1/repository/amlen/$2/build/$1)
				 phase=$(echo $phase | grep -oP '(?<=\"phase\": \")[^\"]*')
				 if [[ 'complete' != $phase ]]
				 then
				   echo $2 phase is $phase
				   exit 1
				 fi
			       done
 
			       if [[ "$BRANCH_NAME" == "main" || ! -z "$CHANGE_ID" ]] ; then
				 curl -H "Accept: application/vnd.github+json" -H "Authorization: Bearer ${GITHUB_TOKEN}" https://api.github.com/repos/eclipse/amlen/statuses/${GIT_COMMIT} -d "{\\\"state\\\":\\\"pending\\\",\\\"target_url\\\":\\\"https://example.com/build/status\\\",\\\"description\\\":\\\"PR=${NOORIGIN_BRANCH} DISTRO=${DISTRO} BUILD=${BUILD_LABEL}\\\",\\\"context\\\":\\\"bvt\\\"}"
			       fi
   
			   '''
                       }
                     }
                 }
	   }
	}
        stage("MakeBundle") {
	    steps {
		echo "In Bundle, BUILD_LABEL is ${env.BUILD_LABEL}"

                container('amlen-centos7-build') {
                      sshagent ( ['projects-storage.eclipse.org-bot-ssh']) {
			   sh '''
			       set -e
			       pwd 
			       cd operator
                               NOORIGIN_BRANCH=${GIT_BRANCH#origin/} # turns origin/master into master
			       IMG=quay.io/amlen/operator:$NOORIGIN_BRANCH
                               echo $NOORIGIN_BRANCH
                               #SHA=`curl "https://quay.io/api/v1/repository/amlen/operator/tag/" | jq -r ".tags[] | select(.\"expiration\"==null) | select(.\"name\"==\\\"$NOORIGIN_BRANCH\\\") | .manifest_digest"`
                               SHA=`python3 find_sha.py $NOORIGIN_BRANCH`
                               echo $SHA
			       export IMG=quay.io/amlen/operator@$SHA
			       make bundle
			       mv bundle.Dockerfile Dockerfile
  
			       tar -czf operator_bundle_digest.tar.gz Dockerfile bundle
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
			       pwd 
			       cd operator
                               NOORIGIN_BRANCH=${GIT_BRANCH#origin/} # turns origin/master into master
			       scp -o BatchMode=yes -r operator_bundle_digest.tar.gz genie.amlen@projects-storage.eclipse.org:/home/data/httpd/download.eclipse.org/amlen/snapshots/${NOORIGIN_BRANCH}/${BUILD_LABEL}/centos7/
			      '''
		       }
                }
            }
        }
        stage("BuildBundle") {
	    steps {
		echo "In Bundle, BUILD_LABEL is ${env.BUILD_LABEL}"

		container("jnlp") {
                    withCredentials([string(credentialsId: 'quay.io-token', variable: 'QUAYIO_TOKEN')]) {
                      sshagent ( ['projects-storage.eclipse.org-bot-ssh']) {
			   sh '''
			       set -e
			       pwd 
			       cd operator
                               NOORIGIN_BRANCH=${GIT_BRANCH#origin/} # turns origin/master into master
			       c=$(curl -X POST https://quay.io/api/v1/repository/amlen/operator-bundle/build/ -H \"Authorization: Bearer ${QUAYIO_TOKEN}\" -H \"Content-Type: application/json\" -d \"{ \\\"archive_url\\\":\\\"https://download.eclipse.org/amlen/snapshots/${NOORIGIN_BRANCH}/${BUILD_LABEL}/centos7/operator_bundle_digest.tar.gz\\\", \\\"docker_tags\\\":[\\\"${NOORIGIN_BRANCH}-d\\\"] }\")
                               echo $c
                               uid=$(echo ${c} | grep -oP '(?<=\"id\": \")[^\"]*\')
                               sleep 240

                               set -- $uid
                               for i in {1..60}
                               do
                                 phase=$(curl -s https://quay.io/api/v1/repository/amlen/operator-bundle/build/$uid)
                                 phase=$(echo $phase | grep -oP '(?<=\"phase\": \")[^\"]*')
                                 if [[ 'complete' == $phase ]]
                                 then
                                   break
                                 fi
                                 sleep 10
                               done
                              
                               phase=$(curl -s https://quay.io/api/v1/repository/amlen/operator-bundle/build/$uid)
                               phase=$(echo $phase | grep -oP '(?<=\"phase\": \")[^\"]*')
                               if [[ 'complete' != $phase ]]
                               then
                                 echo phase is $phase
                                 exit 1
                               fi
          
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
