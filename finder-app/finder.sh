#!/bin/bash

if [ $# -lt 2 ]
then
	echo "Do not have enough arguments"
	exit 1
fi

filesdir=$1
if [ ! -d "${filesdir}" ]
then
	echo "Do not have this directory"
	exit 1
fi

searchstr=$2

x=$(find ${filesdir} -type f | wc -l)
y=$(grep -r "$searchstr" ${filesdir} | wc -l)

echo "The number of files are ${x} and the number of matching lines are ${y}"
