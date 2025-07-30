CXX = g++
CXXFLAGS = -std=c++11 -O3 -Wall -Wextra
TARGET = reconstruction_adarsh
SOURCE = orderbook_reconstructor.cpp

$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCE)

clean:
	rm -f $(TARGET) mbp_output.csv

.PHONY: clean 