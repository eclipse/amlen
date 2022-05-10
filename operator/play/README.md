
Dockerfile/amlen.yaml
=======================
I don't expect this ever to get delivered to master in this form,
but I'm just having a play.

To fill in the variables in the file I'm doing:
```
python server_build/path_parser.py -m filereplace -i Dockerfile -o /tmp/build/Dockerfile 
```
Then I build the image (using an alma8 rpm) and pushed it
to `quay.io/jonquark/jontest1:latest`

Then I can run it using: 
```
oc run demo --image=quay.io/jonquark/jontest1:latest 
```

and then for port-forwarding to allow the WebUI to connect like:
```
 oc port-forward  demo 29089:9089
```
Then I can get a WebUI outside my cluster to connect to localhost:29089

osdk-playop1
============
Code following a tutorial to create a simple operator, see 
[README.md in the osdk-playop1 dir](osdk-playop1/README.md)

