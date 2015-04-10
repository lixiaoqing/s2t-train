set -x
make
mv a unit-test/ 
cd unit-test
./a > log
cat log
cd -
