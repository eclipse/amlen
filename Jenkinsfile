// By default we build on the oldest supported distro (alma8)
// A Jenkinsfile used for other distros (with associated build container Dockerfiles)
// along with other jenkins files we use are all in server_build/buildcontainer
//
def distro = "almalinux8"
def customBuildFile = null
def startQuayBuild(QUAYIO_TOKEN,GIT_BRANCH,BUILD_LABEL,repo,distro,filename,tag){
    output = sh (returnStdout: true, script: '''
        repo='''+repo+'''
        distro='''+distro+'''
        filename='''+filename+'''
        tag='''+tag+'''
        NOORIGIN_BRANCH=${GIT_BRANCH#origin/} # turns origin/master into master
        while [ true ]
        do
            c1=$(curl -X POST https://quay.io/api/v1/repository/amlen/${repo}/build/ -H \"Authorization: Bearer ${QUAYIO_TOKEN}\" -H \"Content-Type: application/json\" -d \"{ \\\"archive_url\\\":\\\"https://download.eclipse.org/amlen/snapshots/${NOORIGIN_BRANCH}/${BUILD_LABEL}/${distro}/${filename}\\\", \\\"docker_tags\\\":[\\\"$tag\\\"]}\" )
            if [[ $c1 == *"phase"* ]]
            then
                c1=$(echo $c1 | jq -r '.["id"]' )
                echo $c1
                break
            else
                message=$(echo $c1 | jq -r '.["message"]')
                echo $message
                if [ "$message" == "Maximum queued build rate exceeded." ]
                then
                    echo "sleeping"
                    sleep 120
                else
                    echo "failure : $message"
                    break
                fi
            fi
        done
    ''')
    lastLine = output.split("\n")[-1]
    return lastLine
}

def waitForQuayBuild(build,repo,QUAYIO_TOKEN){
    output = sh (returnStdout:true, script: '''
    repo='''+repo+''' 
    build='''+build+'''
    phase="waiting"
    while [ $phase == \"waiting\" -o $phase == \"build-scheduled\" -o $phase == \"running\" -o $phase == \"pulling\" -o $phase == \"building\" -o $phase == \"pushing\" ]
    do 
        echo "Waiting for 30 seconds"
        sleep 30 
        phase=$(curl -s https://quay.io/api/v1/repository/amlen/${repo}/build/$build | jq -r '.["phase"]' )
        echo "$phase"
    done
    ''')
    echo output
    lastLine=output.split("\n")[-1]
    return lastLine
}

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
                container('jnlp') {
                    withCredentials([string(credentialsId: 'quay.io-token', variable: 'QUAYIO_TOKEN')]) {
                        script {
                            if (env.BUILD_TIMESTAMP == null) {
                                env.BUILD_TIMESTAMP = sh(script: "date +%Y%m%d-%H%M", returnStdout: true).toString().trim()
                            }
                            echo "default distro ${distro}"
                            distro2 = sh (returnStdout: true, script: ''' 
                                x=0
                                message=`git log -1 --skip=$x --pretty=%B`
                                if [[ "$message" =~ [[]distro=([A-Za-z0-9]+)[]] ]]
                                then 
                                    echo ${BASH_REMATCH[1]} 
                                else 
                                    echo "'''+distro+'''"
                                fi
                            ''').trim()
                            echo "updating linux distribution: ${distro} -> ${distro2}."
                            distro=distro2
                    
                            if (env.BUILD_LABEL == null ) {
                                env.BUILD_LABEL = "${env.BUILD_TIMESTAMP}_eclipse${distro}"
                            }

                    changedFiles = sh ( returnStdout: true, script: '''
                        git fetch --force --progress -- https://github.com/eclipse/amlen.git +refs/heads/main:refs/remotes/origin/main
                        if [ $(git tag -l '''+GIT_BRANCH+'''-builder) ] ; then
                            git diff --name-only '''+GIT_BRANCH+'''-builder
                        else
                            git diff --name-only origin/main
                        fi
                    ''' )

                    latestBuilder = sh ( returnStdout: true, script: '''curl https://quay.io/api/v1/repository/amlen/amlen-builder-almalinux8/tag/?onlyActiveTags=true -H "Authorization: Bearer $QUAYIO_TOKEN" -H "Content-Type: application/json"  | jq -r '.["tags"]|map(select(.name? | match("'''+GIT_BRANCH+'''-"))) | sort_by(.name?)|reverse[0].name // "'''+GIT_BRANCH+'''-1.0.0.0"' ''').trim()

                    echo "Files ${changedFiles}"
                    echo "Latest: ${latestBuilder}"
                    switch(distro) {
                        case "almalinux8": filename = "Dockerfile.alma8"
                            break
                        case "almalinux9": filename = "Dockerfile.alma9"
                            break
                        default: filename = "Dockerfile.${distro}"
                    } 
                    echo "selecting linux distribution: ${distro}."
                    tag = sh ( returnStdout: true, script: '''git tag -l '''+GIT_BRANCH+'''-builder''' ).trim()
                    echo tag
                    if (changedFiles.contains(filename)) {
                        echo "New build image required"
                        customBuildFile = filename
                        latestBuilder = sh ( returnStdout: true, script: '''curl https://quay.io/api/v1/repository/amlen/amlen-builder-almalinux8/tag/?onlyActiveTags=true -H "Authorization: Bearer 3ZfjzxygqPZUa0JXB6KGxvA7qeDxlztWW9e8l3Rq" -H "Content-Type: application/json"  | jq -r '.["tags"]|map(select(.name? | match("'''+GIT_BRANCH+'''-"))) | sort_by(.name?)|reverse[0].name // "'''+GIT_BRANCH+'''-1.0.0.0"' ''').trim()
                        if (latestBuilder == null) {
                            latestBuilder="${GIT_BRANCH}-1.0.0.0"
                        }
                    }
                    else {
                        if ( tag == "" ) {
                            latestBuilder = sh ( returnStdout: true, script: '''curl https://quay.io/api/v1/repository/amlen/amlen-builder-almalinux8/tag/?onlyActiveTags=true -H "Authorization: Bearer 3ZfjzxygqPZUa0JXB6KGxvA7qeDxlztWW9e8l3Rq" -H "Content-Type: application/json"  | jq -r '.["tags"]|map(select(.name? | match("main-"))) | sort_by(.name?)|reverse[0].name // "main-1.0.0.0"' ''').trim()
                        } else {
                            latestBuilder = sh ( returnStdout: true, script: '''curl https://quay.io/api/v1/repository/amlen/amlen-builder-almalinux8/tag/?onlyActiveTags=true -H "Authorization: Bearer 3ZfjzxygqPZUa0JXB6KGxvA7qeDxlztWW9e8l3Rq" -H "Content-Type: application/json"  | jq -r '.["tags"]|map(select(.name? | match("'''+GIT_BRANCH+'''-"))) | sort_by(.name?)|reverse[0].name // "'''+GIT_BRANCH+''''-1.0.0.0"' ''').trim()
                        }
                        echo "selecting build image: ${latestBuilder}."
                        buildImage = latestBuilder
                    }
                  }
                }
              }
            }
        }

        stage("Create Build Container Image") {
            when {
              expression { customBuildFile != null }
            }
            agent any 
            steps {
                container('jnlp') {
                    withCredentials([string(credentialsId: 'quay.io-token', variable: 'QUAYIO_TOKEN'),string(credentialsId:'github-bot-token',variable:'GITHUB_TOKEN')]) {
                        script {
                            sshagent ( ['projects-storage.eclipse.org-bot-ssh']) {
                                sh '''
                                    distro='''+distro+'''
                                    filename='''+customBuildFile+'''
                                    NOORIGIN_BRANCH=${GIT_BRANCH#origin/} # turns origin/master into master
                                    ssh -o BatchMode=yes genie.amlen@projects-storage.eclipse.org mkdir -p /home/data/httpd/download.eclipse.org/amlen/snapshots/${NOORIGIN_BRANCH}/${BUILD_LABEL}/${distro}/
                                    tar -czf buildcontainer.tar.gz -C server_build/buildcontainer --transform="flags=r;s|${filename}|Dockerfile|" ${filename}
                                    scp -o BatchMode=yes -r buildcontainer.tar.gz genie.amlen@projects-storage.eclipse.org:/home/data/httpd/download.eclipse.org/amlen/snapshots/${NOORIGIN_BRANCH}/${BUILD_LABEL}/${distro}/
                                '''
                            }
                            pattern = ~"(.*\\-\\d+\\.\\d+\\.\\d+\\.)(\\d+)"
                            regex = latestBuilder =~ pattern
                            regex.matches()
                            newBuilder = regex.group(1)+((regex.group(2) as int) + 1)
                            build_uuid=startQuayBuild(QUAYIO_TOKEN,GIT_BRANCH,BUILD_LABEL,"amlen-builder-${distro}",distro,"buildcontainer.tar.gz",newBuilder)
                            echo build_uuid
                            if ( waitForQuayBuild(build_uuid,"amlen-builder-${distro}",QUAYIO_TOKEN) == "complete" ) {
                                sh ( returnStdout: true, script: '''
                                    echo ' { "tag": "ib.containers.builder-update", "object": "'''+env.GIT_COMMIT+'''", "message": "creating a tag", "tagger": { "name": "Jenkins", "email": "noone@nowhere.com" }, "type": "commit" } ' > tag.json
                                    sha=$(curl -v -X POST -d @tag.json --header "Content-Type:application/json" -H "Authorization: Bearer ${GITHUB_TOKEN}" "https://api.github.com/repos/eclipse/amlen/git/tags" | jq -r '.["object"]["sha"]')
                                    echo "{\\\"ref\\\":\\\"refs/tags/ib.containers.builder-update\\\",\\\"sha\\\":\\\"$sha\\\"}" > tagref.json
                                    curl -v -X POST -d @tagref.json --header "Content-Type:application/json" -H "Authorization: Bearer ${GITHUB_TOKEN}" "https://api.github.com/repos/eclipse/amlen/git/refs" 
                                ''' )
                            }
                            else {
                                error "QuayIO build didn't complete"
                            }
                            echo "linux distribution: ${distro}."
                            echo "new build image: ${newBuilder}."
                            buildImage = newBuilder
                        }
                    }
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
                stage('Build') {
                    steps {
                        echo "In Build, BUILD_LABEL is ${env.BUILD_LABEL}"
                        container("amlen-${distro}-build") {
                            withCredentials([string(credentialsId: 'quay.io-token', variable: 'QUAYIO_TOKEN'),string(credentialsId:'github-bot-token',variable:'GITHUB_TOKEN')]) {
                                script {
                                    try {
                                        sh '''
                                            set -e
                                            pwd 
                                            distro='''+distro+'''
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
                                            pwd
                                            if [ ! -e rpms/EclipseAmlenBridge-${distro}-1.1dev-${BUILD_LABEL}.tar.gz ]
                                            then 
                                                echo "Bridge Not built"
                                                exit 1
                                            fi
                                            if [ ! -e rpms/EclipseAmlenServer-${distro}-1.1dev-${BUILD_LABEL}.tar.gz ]
                                            then 
                                                echo "Server Not built"
                                                exit 1
                                            fi
                                            if [ ! -e rpms/EclipseAmlenWebUI-${distro}-1.1dev-${BUILD_LABEL}.tar.gz ]
                                            then 
                                                echo "WebUI not built"
                                                exit 1
                                            fi
                                            if [ $BRANCH_NAME == "main" -a ! -e rpms/EclipseAmlenProxy-${distro}-1.1dev-${BUILD_LABEL}.tar.gz ]
                                            then 
                                                echo "Main build but Proxy Not built"
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
                                    sh ( returnStdout: true, script: '''
                                        echo "{\\\"body\\\":\\\"Built with quay.io/amlen/amlen-builder-${distro}:${buildImage}\\\"}"
                                        curl -X POST --header "Content-Type:application/json" -H "Authorization: Bearer ${GITHUB_TOKEN}" "https://api.github.com/repos/eclipse/amlen/commits/${GIT_COMMIT}/comments" -d "{\\\"body\\\":\\\"Built with quay.io/amlen/amlen-builder-'''+distro+''':'''+buildImage+'''\\\"}"
                                    ''' )
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
             agent any
             steps {
                 container('jnlp') {
                     echo "In Deploy, BUILD_LABEL is ${env.BUILD_LABEL}"
                     withCredentials([string(credentialsId: 'quay.io-token', variable: 'QUAYIO_TOKEN'),string(credentialsId:'github-bot-token',variable:'GITHUB_TOKEN')]) {
                         script {
                             server_build_uuid=startQuayBuild(QUAYIO_TOKEN,GIT_BRANCH,BUILD_LABEL,"amlen-server",distro,"EclipseAmlenServer-${distro}-1.1dev-${BUILD_LABEL}.tar.gz",GIT_BRANCH)
                             operator_build_uuid=startQuayBuild(QUAYIO_TOKEN,GIT_BRANCH,BUILD_LABEL,"operator",distro,"operator.tar.gz",GIT_BRANCH)
                             operator_bundle_build_uuid=startQuayBuild(QUAYIO_TOKEN,GIT_BRANCH,BUILD_LABEL,"operator-bundle",distro,"operator_bundle.tar.gz",GIT_BRANCH)
                             waitForQuayBuild(server_build_uuid,"amlen-server",QUAYIO_TOKEN)
                             waitForQuayBuild(operator_build_uuid,"operator",QUAYIO_TOKEN)
                             waitForQuayBuild(operator_build_uuid,"operator_bundle",QUAYIO_TOKEN)
                             sshagent ( ['projects-storage.eclipse.org-bot-ssh']) {
                                 sh '''
                                     pwd
                                     distro='''+distro+'''
                                     NOORIGIN_BRANCH=${GIT_BRANCH#origin/} # turns origin/master into master

                                     if [[ "$BRANCH_NAME" == "main" ]] ; then
                                         curl -H "Accept: application/vnd.github+json" -H "Authorization: Bearer ${GITHUB_TOKEN}" https://api.github.com/repos/eclipse/amlen/statuses/${GIT_COMMIT} -d "{\\\"state\\\":\\\"pending\\\",\\\"target_url\\\":\\\"https://example.com/build/status\\\",\\\"description\\\":\\\"PR=${NOORIGIN_BRANCH} DISTRO=${distro} BUILD=${BUILD_LABEL}\\\",\\\"context\\\":\\\"bvt\\\"}"
                                     elif [[ ! -z "$CHANGE_ID" ]] ; then
                                         commit=$(curl -H "Accept: application/vnd.github+json" -H "Authorization: Bearer ${GITHUB_TOKEN}" https://api.github.com/repos/eclipse/amlen/pulls/$CHANGE_ID/commits | jq '.[-1].sha')
                                         curl -H "Accept: application/vnd.github+json" -H "Authorization: Bearer ${GITHUB_TOKEN}" https://api.github.com/repos/eclipse/amlen/statuses/${commit//\\\"/} -d "{\\\"state\\\":\\\"pending\\\",\\\"target_url\\\":\\\"https://example.com/build/status\\\",\\\"description\\\":\\\"PR=${NOORIGIN_BRANCH} DISTRO=${distro} BUILD=${BUILD_LABEL}\\\",\\\"context\\\":\\\"bvt\\\"}"
                                     fi   
                                 '''
                             }
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
                                    cd operator
                                    NOORIGIN_BRANCH=${GIT_BRANCH#origin/} # turns origin/master into master
                                    IMG=quay.io/amlen/operator:$NOORIGIN_BRANCH
                                    echo $NOORIGIN_BRANCH
                                    SHA=`python3 find_sha.py $NOORIGIN_BRANCH`
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
                                    distro='''+distro+'''
                                    pwd 
                                    cd operator
                                    NOORIGIN_BRANCH=${GIT_BRANCH#origin/} # turns origin/master into master
                                    scp -o BatchMode=yes -r operator_bundle_digest.tar.gz genie.amlen@projects-storage.eclipse.org:/home/data/httpd/download.eclipse.org/amlen/snapshots/${NOORIGIN_BRANCH}/${BUILD_LABEL}/${distro}/
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
                                cd operator
                                NOORIGIN_BRANCH=${GIT_BRANCH#origin/} # turns origin/master into master
                                uuid=startQuayBuild(QUAYIO_TOKEN,GIT_BRANCH,BUILD_LABEL,"operator-bundle",distro,"operator_bundle_digest.tar.gz","${GIT_BRANCH}-d")
                                waitForQuayBuild(uuid,"operator-bundle",QUAYIO_TOKEN)
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
