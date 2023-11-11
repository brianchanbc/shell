#! /usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

# .. is used to point to the parent directory of the current directory
# -I is used to specify the directory to search for header files 
# in other words, where the shell.h file is located
gcc -I../include/ -o ../bin/msh ../src/*.c