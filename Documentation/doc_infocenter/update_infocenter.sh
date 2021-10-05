#!/bin/ksh

set -x
PN=$0	# program name

plugin1=com.ibm.ism.doc
plugin2=com.ibm.ism.messages.doc
plugin3=WelcomePage
ant_bld_top=ism_build
kc_taxonomy_id=SSWMAJ_5.0.0.1
brand=tivoli
product_subdir=ism/v5r0m01
kc_ditamap=master_map

nfs_bld_top=${NFS_SERVER}:${NFS_BLD_TOP}

dir1=${ant_bld_top}/dist/en/kc/${plugin1}
dir2=${ant_bld_top}/dist/en/kc/${plugin2}
dir3=${ant_bld_top}/dist/en/kc/${plugin3}

# ---------------------- functions ------------------
# Print given error message on stderr, then exit 1
err_exit()
{
	print -u2 "${PN}: ${1}"
	exit 1
}

# ---------------------- main ------------------

# Verify NFS server with build trees is defined ( in BF environment ).
[ -z "${NFS_SERVER}" ] &&
	err_exit "Shell variable NFS_SERVER not defined."

# update Knowledge Center

echo "[ `date` ]"

scp -r ${nfs_bld_top}/${dir1} lib2@pokgsa.ibm.com:/gsa/pokgsa/projects/a/ahe01_kc/projects/wwwtest/content/knwctr/${brand}/${product_subdir}/ #> /dev/null 2>&1
scp -r ${nfs_bld_top}/${dir2} lib2@pokgsa.ibm.com:/gsa/pokgsa/projects/a/ahe01_kc/projects/wwwtest/content/knwctr/${brand}/${product_subdir}/ #> /dev/null 2>&1
scp -r ${nfs_bld_top}/${dir3} lib2@pokgsa.ibm.com:/gsa/pokgsa/projects/a/ahe01_kc/projects/wwwtest/content/knwctr/${brand}/${product_subdir}/ #> /dev/null 2>&1
scp ${nfs_bld_top}/SSWMAJ_5.0.0_ism/mapfiles/${kc_ditamap}.ditamap lib2@pokgsa.ibm.com:/gsa/pokgsa/projects/a/ahe01_kc/projects/wwwtest/content/knwctr/${brand}/${product_subdir}/KC_ditamaps/${kc_ditamap}.ditamap #> /dev/null 2>&1
scp ${nfs_bld_top}/com.ibm.ism.doc/${kc_taxonomy_id}.properties lib2@pokgsa.ibm.com:/gsa/pokgsa/projects/a/ahe01_kc/projects/wwwtest/content/knwctr/${brand}/_content_config/ism/${kc_taxonomy_id}.properties #> /dev/null 2>&1

java -jar /usr/local/lib/jenkins-cli.jar -s http://kcpbnrm1.ahe.pok.ibm.com:8080/ -ssh -user lib2 -i $HOME/.ssh/id_rsa build normalize_preprod_product -p PRODUCT_CONFIG=/gsa/pokgsa/projects/a/ahe01_kc/projects/wwwtest/content/knwctr/${brand}/_content_config/ism/${kc_taxonomy_id}.properties -s -v 

# update eden2 Knowledge Center

# scp -r ${nfs_bld_top}/${dir1} /KCHOME/${product_subdir}/ #> /dev/null 2>&1
# scp ${nfs_bld_top}/mavm/com.ibm.mavm.doc/${kc_ditamap}.ditamap /KCHOME/${product_subdir}/KC_ditamaps/${kc_ditamap}.ditamap #> /dev/null 2>&1
# scp ${nfs_bld_top}/mavm/com.ibm.mavm.doc/${kc_taxonomy_id}.properties /KCHOME/conf/maximo/${kc_taxonomy_id}.properties #> /dev/null 2>&1

echo "[ `date` ]"

echo "Knowledge Center files  updated"