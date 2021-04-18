#!/bin/sh

# This script will run LifeLines with valgrind with suitable suppressions.
# Tested on Ubuntu; changes to this script and the valgrind suppression file
# will be needed on other platforms.

# Determine root of repository
if [ ! -f configure.ac ]
then
  echo "ERROR: Must be run from the root of the source tree!"
  exit 1
else
  ROOTDIR=`pwd`
fi

# Check if LifeLines has been built
if [ ! -f src/liflines/llines ]
then
  echo "ERROR: LifeLines has not been built!"
  exit 1
fi

valgrind --leak-check=full --show-leak-kinds=all --log-file=valgrind.log --suppressions=build/valgrind.supp.ubuntu src/liflines/llines $@
