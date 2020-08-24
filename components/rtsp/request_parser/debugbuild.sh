./clean.sh
./generate.sh
cd generated
g++ *.c *.cpp -I./../..
./a.out
