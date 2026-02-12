#!/bin/bash

if [ $# -lt 2 ]
then
	echo "Do not have enough arguments"
	exit 1
fi

writefile=$1
writestr=$2

mkdir -p "$(dirname "$writefile")"
echo "${writestr}" > "${writefile}"

#if [ -f "${writefile}" ]
#then
#    echo "${writestr}" > "${writefile}"
#else
#    touch ${writefile}
#    echo "${writestr}" > "${writefile}"
#    exit 1
#fi

#if [ $? == 1 ]
#then
#    echo "${writefile} is not writable"
#    exit 1
#fi
