#Amlen Operator

The ansible operator for deploying Amlen systems. Currently under development.

## Quick Start

The operator requires a certificate manager such as cert-manager which can be deployed via:

kubectl apply -f https://github.com/cert-manager/cert-manager/releases/download/v1.8.0/cert-manager.yaml

Assuming you are using an image that already exists you just need to install and deploy it

```
export IMG=quay.io/ianboden/amlen-operator:v1.0.331
make install
make deploy
```

That will deploy the operator into the amlen-system

In config/samples are the yaml files need to deploy a simple setup start with a new project and the deploy the samples:

```
oc new-project amlen
oc apply -f config/samples/simple_password.yaml
oc apply -f config/samples/selfsigned.yaml
oc apply -f config/samples/config.yaml
oc apply -f config/samples/amlen_v1_amlen.yaml
```

This will deploy 2 HA pairs called amlen-0 and amlen-1 into the namespace. amlen-0 will have login details set to sysadmin:passw0rd
while amlen-1 will use a random password stored in the secret amlen-1-password. Both will have created certificates stored in amlen-0-cert-internal 
and amlen-1-cert-internal. (passwords and certificates are mounted in /secrets/ on the amlen pods)


## Default values

In roles/amlen/defaults/main.yml are the default values that are used, these can all be overriden in the Amlen CustomResourceDefinition

```
_amlen_state: present
_amlen_namespace: amlen
_amlen_name: amlen

_amlen_volume_size: 1Gi
_amlen_memory_request: 3Gi
_amlen_memory_limit: 3Gi
_amlen_cpu_request: 200m
_amlen_cpu_limit: 500m
_amlen_wait_for_init: true
_amlen_image: quay.io/ianboden/amlen
_amlen_image_tag: latest
_amlen_configurator_image: quay.io/ianboden/amlen-configurator
```

These defaults are suitable for creating demo systems only memory and cpu limits should be sized correctly for production systems
