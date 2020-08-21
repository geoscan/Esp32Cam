mkdir -p generated
cp rtsp.y generated
cp rtsp.l generated
cd generated
bison -d rtsp.y;
flex --header-file=lex.yy.h rtsp.l
rm ./rtsp.l ./rtsp.y