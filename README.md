# cle-assignement-2

How to compile and execute with the MPI library:

## Prob 1

```
mpicc -Wall -O3 -o main main.c utils.c
mpiexec -n 5 ./main -f text0.txt text1.txt text2.txt text3.txt text4.txt
```

## Prob 2

```
mpicc -Wall -O3 -o main main.c utils.c
mpiexec -n 4 ./main -f datSeq32.bin datSeq256K.bin datSeq1M.bin datSeq16M.bin
```