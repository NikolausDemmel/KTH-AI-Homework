#! /bin/sh
name=$1
range=$2
/usr/bin/env gnuplot <<FOO
set datafile separator ';'
plot "$name" using $range
FOO
