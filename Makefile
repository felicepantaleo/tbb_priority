clean:
	rm -rf test


all:
	g++ test_priority.cpp -std=c++20 -o test -O2 -I /cvmfs/cms.cern.ch/el8_amd64_gcc12/external/tbb/v2021.9.0-573155027234b8f945d29403a2749d52/include -L /cvmfs/cms.cern.ch/el8_amd64_gcc12/external/tbb/v2021.9.0-573155027234b8f945d29403a2749d52/lib/ -ltbb -pthread



run: all
	./test
