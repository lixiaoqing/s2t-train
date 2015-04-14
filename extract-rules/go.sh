set -x
make
mv a unit-test/ 
cd unit-test
./a en.parse ch al lex.e2f lex.f2e > log
cat log
cd -
