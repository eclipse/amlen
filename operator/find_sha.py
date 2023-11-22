import requests
import sys

name=sys.argv[1]
thing = requests.get("https://quay.io/api/v1/repository/amlen/operator/tag/").json()
for k in thing['tags']:
  if k['name'] == name:
    if "expiration" not in k:
      print(k['manifest_digest'])
      exit()
  
