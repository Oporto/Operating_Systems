LIB=-lpthread
CCPP=g++
CFLAGS = -g -Wall

all : searchstrings doit
searchstrings : searchstrings.cpp
	$(CCPP) searchstrings.cpp -o searchstrings $(LIB)
doit : doit.cpp
	$(CCPP) doit.cpp -o doit $(LIB)
clean :
	rm -f searchstrings
	rm -f doit
