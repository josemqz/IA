compile:
		g++ start.cpp -o "start.o"
run:
		./start.o
.PHONY : clean
clean :
		-rm *.o $(objects) data.txt
