#!/bin/sh


LINKS=""
for i in `ls *.bin`; do
    LINKS="${LINKS}<a href=\"$i\">$i</a>\n"
done
sed "s^<LINKS>^${LINKS}^g" < Tools/download.html
