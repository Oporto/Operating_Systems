LIB=-lpthread
CCPP=g++
CFLAGS = -g -Wall

all : life addem
life : life.cpp
	$(CCPP) life.cpp -o life $(LIB)
addem : addem.cpp
	$(CCPP) addem.cpp -o addem $(LIB)
clean :
	rm -f life
	rm -f addem
