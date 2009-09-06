#!/bin/bash

log_file=`tempfile`
echo "log file = $log_file"
./build/test $log_file

num_line=`cat $log_file | wc -l`

if [ $num_line != 65538 ]; then
	echo "wrong line number ($num_line)" 1>&2
	exit 1
fi

for i in `seq 1 64`; do
	if [ `egrep "thread $i " $log_file | wc -l` != 1024 ]; then
		echo "thread $i wrong log entries count" 1>&2
		exit 1
	fi
done

exit 0
