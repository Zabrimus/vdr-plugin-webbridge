CXXFLAGS = -std=c++17 -Wall
LDLIBS = -lstdc++

export CFLAGS   = $(call PKGCFG,cflags)
export CXXFLAGS = $(call PKGCFG,cxxflags) -std=c++17 -fno-strict-aliasing -O3

.PHONY: all clean

all: testclient

testclient: testclient.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) testclient.cpp -o testclient

clean:
	-rm  testclient
