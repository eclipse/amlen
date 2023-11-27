# Amlen Operator

The ansible operator for deploying Amlen systems. 

## Release process

The release process is currently a bit awkward what needs to happen is:

push a change that moves the version from the current alpha version to the actual version, ie if the version is 1.0.3a then submit a PR
to change it to version 1.0.3

When that has merged into main and completed the build:
- Tag the operator-bundle:main-d as version-pre eg 1.0.3-pre
- Tag the operator:main as v1.0.3

Push a change that moves the version in main to the next alpha channel eg 1.0.4a

When that has merged into main and completed the build:
- Tag the operator-bundle:(version)-pre as (version) eg add 1.0.3 tag to the 1.0.3-pre tag

This results in:
- operator-bundle:main which points to operator-bundle:main with a version of 1.0.4a (can be used in development environments)
- operator-bundle:main-d which points to the digest of operator-bundle:main with a version of 1.0.4a
- operator-bundle:1.0.3 which points to the digest of operator:v1.0.3 with a version of 1.0.3 (can be used in production environments)

## Quick Start

The operator requires a certificate manager such as cert-manager which can be deployed via:

kubectl apply -f https://github.com/cert-manager/cert-manager/releases/download/v1.8.0/cert-manager.yaml

Assuming you are using an image that already exists you just need to install and deploy it

```
export IMG=quay.io/amlen/operator:main
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

_amlen_volume_size: 10Gi
_amlen_memory_request: 3Gi
_amlen_memory_limit: 3Gi
_amlen_cpu_request: 200m
_amlen_cpu_limit: 500m
_amlen_wait_for_init: true
_amlen_image: quay.io/amlen/amlen-server
_amlen_image_tag: main
```

These defaults are suitable for creating demo systems only volume size, memory and cpu limits should be sized correctly for production systems

## LDAP

By adding:

```
deploy_ldap: true
```

to the Amlen CustomResourceDefinition an LDAP server will be created in the namespace and all amlen instances will be configured to use it. This means that authentication can be done via LDAP. This requires an ldap-config configmap to exist, one that works can be found in config/samples/config-ldap.yaml many of the values used to configure amlen are hardcoded so changing the config is likely to break authentication but you can add users to the config.

Once LDAP has been deployed it is possible to add users using ldapadd here is an example ldif for adding 2 users:

```
dn: cn=msgUser1,ou=users,dc=amleninternal,dc=auth
changetype: add
objectclass: inetOrgPerson
cn: msgUser1
givenname: msgUser1
sn: msgUser1
displayname: Messaging User 1
userpassword: msgUser1_pass

dn: cn=msgUser2,ou=users,dc=amleninternal,dc=auth
changetype: add
objectclass: inetOrgPerson
cn: msgUser2
givenname: msgUser2
sn: msgUser2
displayname: Messaging User 2
userpassword: msgUser2_pass

dn: cn=msgUsers,ou=groups,dc=amleninternal,dc=auth
changetype: add
cn: msgUsers
objectclass: groupOfUniqueNames
uniqueMember: cn=msgUser1,ou=users,dc=amleninternal,dc=auth
uniqueMember: cn=msgUser2,ou=users,dc=amleninternal,dc=auth
```

If exec'd into the LDAP container this could be added via:
```
ldapadd -H ldap://localhost:1389 -D "cn=admin,dc=amleninternal,dc=auth" -w adminpassword -f /tmp/users.ldif
```

This assumes using the default config and passwords. Exposing the ldap-service it is possible to run it externally.

To enable clients to connect via passwords the amlen config will need to be changed, the UsePasswordAuthentication of the security profile will need to be set to true.


## Limitations

This is still in development and so is mainly focused on installation as such there are a number of limitations:

* Reducing the size does not result in the number of StatefulSets being reduced, they must be removed manually
* Cleanup of PVCs must be done manually
* Updates of Amlen (either by changing the _amlen_image_tag in the CRD or issuing a roll out of the statefulset) is likely to cause double disconnects in HA pairs. After a fresh install instance 0 in the pair will be primary but a subsequent update will make instance 1 to be primary, if instance 1 is the primary then an update will cause double disconnects due to always updating instance 1 first which will mean devices failover to instance 0 then fail back when instance 0 is updated.
* Changing the admin password is dangerous, in theory you can update the secret to the new password then run the rest API command to change the admin password then issue a rolling restart of the statefulset, in practice it may well fall over as the readiness script will fail during this process.
* once LDAP has been deployed it will not get undeployed by editting the CRD and Amlen will not be deconfigured from using LDAP 

## Testing

A basic molecule test has been written that requires openshift (it uses openshift's ability to expose ports for the test to run).

The test uses some of the sample files to create 2 HA pairs (4 pods in total) using self signed certificates with a config that requires clients to use certificates. It then checks it's possible to publish and subscribe to the second HA pair.

## Samples

### amlen_v1_amlen.yaml

Creates 2 HA pair systems using a simple config and certificate and deploys LDAP

### _v1alpha_amlen.yaml

Used in the osdk-test namespace for use in molecule testing

### config.yaml

Creates a simple config based on the default config but requires clients to use certificates for connection

### self_signed.yaml

Create a ClusterIssuer that produces self signed certificates

### simple_password.yaml

An example of how to pre-set the admin password for a system given the system name (the name that matches the stateful set that is created) and the namespace.

### config-ldap.yaml

An example configuration if using the operator to deploy LDAP

### small_amlen.yaml

A config for producing a single Amlen instance with LDAP deployed
