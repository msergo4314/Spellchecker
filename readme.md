Notes:
Some of the logic for delimiting the strings may be platform dependant
The number of threads is defined as a constant in the header and should be updated as needed if you choose
To check you many cores are available to you uncomment the code that uses openMP and run it. This code will introduce "memory leaks" that valgrind will track but they are not true memory leaks.
