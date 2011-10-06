#! /bin/sh
name=$1
/usr/bin/env gnuplot <<FOO
set datafile separator ';'
plot "$name" matrix w image
FOO
