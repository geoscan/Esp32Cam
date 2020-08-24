./clean.sh
./generate.sh
cd generated
echo "#define PARSER_DEBUG 1" > parser_debug_flag.hpp
g++ *.cpp -I./../..
./a.out
echo "#define PARSER_DEBUG 0" > parser_debug_flag.hpp