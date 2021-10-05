all: lib

setOS::
	echo 'OS =' `uname` | tr '/-' '__' > setOS

lib::	setOS
	make -f	makelib

clean::	setOS
	make OS_ST=32 -f makelib clean
	make OS_ST=64 -f makelib clean

beam:: setOS
	make -f makelib beam


beamJ:: setOS
	make OS_ST=64 -f makelib beamJ

beamR:: setOS
	make OS_ST=64 -f makelib beamR


