clang -emit-llvm -S -o resources/example1.ll resources/example1.c
clang -emit-llvm -c -o resources/example1.bc resources/example1.c

clang -emit-llvm -S -o resources/example2.ll resources/example2.c
clang -emit-llvm -c -o resources/example2.bc resources/example2.c

clang -emit-llvm -S -o resources/example3.ll resources/example3.c
clang -emit-llvm -c -o resources/example3.bc resources/example3.c
