cd build
./assembler ../test/main.s ../result/temp1.s ../result/main.o
./assembler ../test/add.s ../result/temp2.s ../result/add.o
./linker ../result/main.o ../result/add.o ../result/a.out
cd ..