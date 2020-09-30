#!/bin/sh

if [ "$#" -ne 1 ]; then
target_branch="master"
else
target_branch=$1
fi
echo "git diff ${target_branch}... --name-only"
filelist=`git diff ${target_branch}... --name-only`


# function to check if C++ file (based on suffix)
# can probably be done much shorter
checkCPP(){
    if [ "${1##*.}" = "cc" ];then
	return 0
    elif [ "${1##*.}" = "cpp" ];then
	return 0
    elif [ "${1##*.}" = "cxx" ];then
	return 0
    elif [ "${1##*.}" = "C" ];then
	return 0
    elif [ "${1##*.}" = "c++" ];then
	return 0
    elif [ "${1##*.}" = "c" ];then
	return 0
    elif [ "${1##*.}" = "CPP" ];then
	return 0
	# header files
    elif [ "${1##*.}" = "h" ];then
	return 0
    elif [ "${1##*.}" = "hpp" ];then
	return 0
    elif [ "${1##*.}" = "hh" ];then
	return 0
    elif [ "${1##*.}" = "icc" ];then
	return 0
    fi
    return 1
}

# check list of files
for f in $filelist; do
    if checkCPP $f; then
	echo "CHECKING MATCHING FILE ${f}"
	# apply the clang-format script
	clang-format -i ${f}
    fi
done

# check if something was modified
notcorrectlist=`git diff --name-only`
# if nothing changed ok
if [  -z "$notcorrectlist" ]; then
  # send a negative message to gitlab
  echo "Excellent. **VERY GOOD FORMATTING!** :thumbsup:"
  exit 0;
else
  echo "The following files have clang-format problems (showing patches)";
  for f in $notcorrectlist; do
      echo $f
      git --no-pager diff $f
  done
fi

git reset HEAD --hard
exit 1

