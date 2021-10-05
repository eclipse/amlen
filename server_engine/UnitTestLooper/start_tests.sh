#
# Create a screen session and run a config against a stream
# requires the config file name and optional takes a stream name
# if no stream given will use Experimental.engine.main
#
# example invocations:
# ./start_tests.sh standard_config
# ./start_tests.sh standard_config "IMADev Stream"
#
export TEST_STREAM=$2
screen -S looper -c ./$1
