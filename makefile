all: ssort

ssort:
	mpicc ssort.c -o ssort

clean:
	rm *.txt ssort



