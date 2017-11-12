#!/bin/bash

# XLang
# -- A parser framework for language modeling
# Copyright (C) 2011 onlyuser <mailto:onlyuser@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

show_help()
{
    echo "Usage: `basename $0` <INPUT_FILE> <OUTPUT_FILE_STEM> [LINT_FLAGS..]"
}

if [ $# -lt 2 ]; then
    echo "fail! -- expect at least 2 arguments! ==> $@"
    show_help
    exit 1
fi

INPUT_FILE=$1
OUTPUT_FILE_STEM=$2
PASS_FILE=${OUTPUT_FILE_STEM}.pass
FAIL_FILE=${OUTPUT_FILE_STEM}.fail

if [ ! -f $INPUT_FILE ]; then
    echo "fail! -- INPUT_FILE not found! ==> $INPUT_FILE"
    exit 1
fi

COUNT=0
for ARG in "$@"; do
    ((COUNT+=1))
    if [ $COUNT -gt 2 ]; then
        INCLUDE_PATH_FLAGS="$INCLUDE_PATH_FLAGS $ARG"
    fi
done

LINT_TOOL="cppcheck"
LINT_FLAGS="-v --enable=all"
FILE_DATA=`$LINT_TOOL $LINT_FLAGS $INPUT_FILE $INCLUDE_PATH_FLAGS |& grep -v "not found\|Checking" |& sed "/^$/d"`
if [ -n "$FILE_DATA" ]; then
    echo "fail!"
    echo -e "$FILE_DATA" > $FAIL_FILE
    exit 1
fi

echo "success!" | tee $PASS_FILE
