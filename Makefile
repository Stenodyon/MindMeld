CXX=g++
CXXFLAGS= -Wall -fexceptions --std=c++11

.PHONY: release debug clean

default: release

MindMeld: main.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) main.cpp -o MindMeld

release: CXXFLAGS+=-O2
release: MindMeld

debug: CXXFLAGS+=-g
debug: MindMeld

clean:
	rm MindMeld 2>/dev/null || true
