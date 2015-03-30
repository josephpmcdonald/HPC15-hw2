all: ssort mpi_bug

ssort:
	mpicc ssort.c -o ssort

mpi_bug:
	mpicc mpi_bug1.c -o mpi_bug1
	mpicc mpi_bug2.c -o mpi_bug2
	mpicc mpi_bug3.c -o mpi_bug3
	mpicc mpi_bug4.c -o mpi_bug4
	mpicc mpi_bug5.c -o mpi_bug5
	mpicc mpi_bug6.c -o mpi_bug6
	mpicc mpi_bug7.c -o mpi_bug7

clean:
	rm rank*.txt ssort




