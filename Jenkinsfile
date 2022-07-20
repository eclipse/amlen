// By default we build on the oldest supported distro (centos7)
// A Jenkinsfile used for other distros (with associated build container Dockerfiles)
// along with other jenkins files we use are all in server_build/buildcontainer
//

pipeline {
  agent {
    kubernetes {
      label 'amlen-centos7-build-pod'
      yaml """
apiVersion: v1
kind: Pod
spec:
  containers:
  - name: amlen-centos7-build
    image: quay.io/amlen/amlen-builder-centos7:1.0.0.2
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

  stages {
        stage('Init') {
            steps {
                container('amlen-centos7-build') {
                    script {
                        if (env.BUILD_LABEL == null ) {
                            env.BUILD_LABEL = sh(script: "date +%Y%m%d-%H%M", returnStdout: true).toString().trim() +"_eclipsecentos7"
                        }
                    }
                    echo "In Init, BUILD_LABEL is ${env.BUILD_LABEL}"    
                }
            }
        }
        stage('Build') {
            steps {
                echo "In Build, BUILD_LABEL is ${env.BUILD_LABEL}"

                container('amlen-centos7-build') {
                   sh '''
                       pwd 
                       free -m 
                       cd server_build 
                       bash buildcontainer/build.sh
                       cd ../operator
                       NOORIGIN_BRANCH=${GIT_BRANCH#origin/} # turns origin/master into master
                       export IMG=quay.io/amlen/operator:$NOORIGIN_BRANCH
                       make bundle
                       make produce-deployment
                      '''
                }
            }
        }
        stage('Deploy') {
            steps {
                container('jnlp') {
                    echo "In Deploy, BUILD_LABEL is ${env.BUILD_LABEL}"
                    sshagent ( ['projects-storage.eclipse.org-bot-ssh']) {
                        sh '''
                            pwd
                            echo ${GIT_BRANCH}
                            echo "BUILD_LABEL is ${BUILD_LABEL}"
                            NOORIGIN_BRANCH=${GIT_BRANCH#origin/} # turns origin/master into master
                            ssh -o BatchMode=yes genie.amlen@projects-storage.eclipse.org mkdir -p /home/data/httpd/download.eclipse.org/amlen/snapshots/${NOORIGIN_BRANCH}/${BUILD_LABEL}/centos7/
                            scp -o BatchMode=yes -r rpms/*.tar.gz genie.amlen@projects-storage.eclipse.org:/home/data/httpd/download.eclipse.org/amlen/snapshots/${NOORIGIN_BRANCH}/${BUILD_LABEL}/centos7/
                            curl -X POST https://quay.io/api/v1/repository/amlen/amlen-server/build/ -H "Authorization: Bearer ${QUAYIO_TOKEN_NEEDS_RENAME}" -H 'Content-Type: application/json' -d "{ \"archive_url\":\"https://download.eclipse.org/amlen/snapshots/${NOORIGIN_BRANCH}/${BUILD_LABEL}/centos7/EclipseAmlenServer-centos7-1.1dev-${BUILD_LABEL}.tar.gz\", \"docker_tags\":[\"${NOORIGIN_BRANCH}\"] }"

                            sed -i "s/IMG_TAG/$NOORIGIN_BRANCH/" operator/roles/amlen/defaults/main.yml
                            cd operator
                            tar -czf operator.tar.gz Dockerfile requirements.yml roles watches.yaml
                            scp -o BatchMode=yes -r operator.tar.gz genie.amlen@projects-storage.eclipse.org:/home/data/httpd/download.eclipse.org/amlen/snapshots/${NOORIGIN_BRANCH}/${BUILD_LABEL}/centos7/
                            curl -X POST https://quay.io/api/v1/repository/amlen/operator/build/ -H "Authorization: Bearer ${QUAYIO_TOKEN_NEEDS_RENAME}" -H 'Content-Type: application/json' -d "{ \"archive_url\":\"https://download.eclipse.org/amlen/snapshots/${NOORIGIN_BRANCH}/${BUILD_LABEL}/centos7/operator.tar.gz\", \"docker_tags\":[\"${NOORIGIN_BRANCH}\"] }"
                            mv bundle.Dockerfile Dockerfile

                            tar -czf operator_bundle.tar.gz Dockerfile bundle
                            scp -o BatchMode=yes -r operator_bundle.tar.gz genie.amlen@projects-storage.eclipse.org:/home/data/httpd/download.eclipse.org/amlen/snapshots/${NOORIGIN_BRANCH}/${BUILD_LABEL}/centos7/
                            curl -X POST https://quay.io/api/v1/repository/amlen/operator-bundle/build/ -H "Authorization: Bearer ${QUAYIO_TOKEN_NEEDS_RENAME}" -H 'Content-Type: application/json' -d "{ \"archive_url\":\"https://download.eclipse.org/amlen/snapshots/${NOORIGIN_BRANCH}/${BUILD_LABEL}/centos7/operator_bundle.tar.gz\", \"docker_tags\":[\"${NOORIGIN_BRANCH}\"] }"

                            scp -o BatchMode=yes -r amlen-operator.yaml genie.amlen@projects-storage.eclipse.org:/home/data/httpd/download.eclipse.org/amlen/snapshots/${NOORIGIN_BRANCH}/${BUILD_LABEL}/centos7/
                        '''
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
