./clean.sh
./generate.sh
cd generated
g++ lex.yy.c  rtsp.tab.c
a.out
