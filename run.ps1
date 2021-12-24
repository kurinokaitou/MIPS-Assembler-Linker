cd build
cmake ..
make assembler
make linker
./assembler ../test/main.s ../test/temp1.s ../test/main.o
./assembler ../test/add.s ../test/temp2.s ../test/add.o
./linker ../test/main.o ../test/add.o ../test/a.out
cd ..