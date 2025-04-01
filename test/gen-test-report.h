#!/bin/sh

rm -Rf ./coverage
mkdir -p ./coverage
mkdir -p ./coverage/report

lcov -c --rc lcov_branch_coverage=1 -d . -o ./coverage/testcoverage.info
genhtml --rc lcov_branch_coverage=1 -o ./coverage/report -f ./coverage/testcoverage.info --num-spaces 4

