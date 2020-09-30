#!/bin/sh

if test "$#" != "1" ; then
  echo "Not enough arguments"
  exit 1
fi

VARIABLE="firmwareVersion"
DIR="$1"

#
# Check whether this is a valid git repository.
# Set ISGIT to 1=true or 0=false.
#

ISGIT=0
COMMIT_COUNT=`git rev-list HEAD --count 2>/dev/null`
if test "x$?" = "x0" ; then
  ISGIT=1
fi

if test $ISGIT = 0 ; then
  echo "Not a GIT repository"
  exit 1
fi

HASH=`git show --abbrev-commit HEAD | grep '^commit' | sed -e 's/commit //'`
NEWREV=`git describe --abbrev=0 --tags`

#
# Check new revision from "git describe". However, if this is no valid
# git-repository, return success and do nothing.
#

if test "$2" != "" ; then
  PREFIX="$2 "
else
  PREFIX="$2"
fi

UPPER=`echo "$VARIABLE" | tr "[:lower:]" "[:upper:]"`
SOURCE_FILE="const char *$VARIABLE = \"$PREFIX$NEWREV.$COMMIT_COUNT-$HASH\";"
echo "$SOURCE_FILE" > "$DIR/Version.cpp"
echo "Prject version: $PREFIX$NEWREV.$COMMIT_COUNT-$HASH"
