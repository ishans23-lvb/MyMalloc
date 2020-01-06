all: test.out MyMalloc.so syst.so

test.out: MyMalloc.so Test.o
	g++ -std=c++11 MyMalloc.so Test.o -o test.out

MyMalloc.so: src/MyMalloc/*.c
	gcc -fPIC -shared $(CFLAGS) src/MyMalloc/*.c -o MyMalloc.so

Test.o: src/Test/main.cpp
	g++ -std=c++11 $(CFLAGS) -c src/Test/main.cpp -o Test.o

syst.so: src/Test/syst.c
	gcc -Wall -std=c99 -DRUNTIME -shared -fPIC src/Test/syst.c -o syst.so -ldl

clean:
	rm -rf *.o *.out *.so