FROM quay.io/operator-framework/ansible-operator:v1.34.2

COPY requirements.yml ${HOME}/requirements.yml
RUN ansible-galaxy collection install -r ${HOME}/requirements.yml \
 && chmod -R ug+rwx ${HOME}/.ansible

COPY watches.yaml ${HOME}/watches.yaml
COPY roles/ ${HOME}/roles/

#There's a utility script that we use for communication with the amlen server
COPY ../server_main/scripts/server.py ${HOME}

USER root
RUN chmod -R g=u ${HOME}
RUN pip3 install jmespath openshift
USER ${USER_UID}
