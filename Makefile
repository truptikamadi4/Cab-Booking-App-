CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

cab: main.cpp utils.h models.h fare.h database.h auth.h booking.h driver_module.h admin.h
	$(CXX) $(CXXFLAGS) -o cab main.cpp

run: cab
	./cab

clean:
	rm -f cab
