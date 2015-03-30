all: ssort mpi_bug

ssort:
	mpicc ssort.c -o ssort

mpi_bug:
	mpicc mpi_bug1.c

clean:
	rm rank*.txt ssort



