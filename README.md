Intersector
===========

A simple C++ utility to create a CSV listing junctions found in an .osm.pbf file.

A junction is any node with more than two highways calling at it. By default motorable highways, cycleways and pedestrian ways are considered, but you can trivially patch the code for different highway values. The CSV columns are lat, lon, and roads (a series of numeric characters representing the road types).

Building and running
--------------------

On OS X:

    clang++ -o intersector osmformat.pb.cc intersector.cpp -std=c++11 -lz `pkg-config --cflags --libs protobuf`

Requires the Google protobuf library (`brew install protobuf`).

Then simply run with

    ./intersector extract_name.osm.pbf

Output goes to junctions.csv.
