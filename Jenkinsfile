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
    image: quay.io/jonquark/jontest1:latest
    command:
    - cat
    tty: true
"""
    }
  }
    stages {
        stage('Build') {
            steps {
                container('amlen-centos7-build') {
                   sh 'ls -ltr && cd amlen/server_build && ls -ltr'
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
