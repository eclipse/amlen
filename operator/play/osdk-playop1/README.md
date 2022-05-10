osdk-play1
==========

Resources created using 
https://docs.openshift.com/container-platform/4.10/operators/operator_sdk/ansible/osdk-ansible-tutorial.html#osdk-ansible-tutorial
As I'm using podman (not docker) I replaced all dockers with podman in the Makefile

I also had to update the requirements.yaml as the versions listed were older than the ones in the base image whhich caused the 
build to fail.

(Also, I'm not sure it's necessary - I didn't try without it but whilst building/pushing images I had the var:

```export BUILDAH_FORMAT=docker```

which apparently is useful to make podman use docker format images

With the changes, running the opator directly on my (F35) laptop did not work, ansible was creating a directory under tmp
and not copying the roles into it and expecting them to be there.

However the "Running as a deployment on the Cluster" method worked fine.
