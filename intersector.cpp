/*
	intersector
	Read an .osm.pbf and output a list of junctions

	clang++ -o intersector osmformat.pb.cc intersector.cpp -std=c++11 -lz `pkg-config --cflags --libs protobuf`
	./intersector ~/Maps/planet/oxfordshire-latest.osm.pbf
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include "osmformat.pb.h"

using namespace std;
#include "helpers.cpp"
#include "pbf_blocks.cpp"

int main(int argc, char* argv[]) {

	unordered_map<unsigned long, vector<unsigned char>> junctionNodes;	// will need to become 'unsigned long long' soon
	string highwayKey = "highway";
	string areaKey = "yes";
	map<string, unsigned char> highwayCodes = {
		{ "motorway", 1 },
		{ "trunk", 2 },
		{ "primary", 3 },
		{ "secondary", 4 },
		{ "tertiary", 5 },
		{ "unclassified", 5 },
		{ "living_street", 5 },
		{ "residential", 5 },
		{ "pedestrian", 6 },
		{ "service", 7 },
		{ "cycleway", 8 },
		{ "motorway_link", 9 },
		{ "trunk_link", 9 },
		{ "primary_link", 9 },
		{ "secondary_link", 9 }
	};
	
	// First pass - read ways

	fstream infile(argv[1], ios::in | ios::binary);
	if (!infile) { cerr << "Couldn't open .pbf input file." << endl; return -1; }

	HeaderBlock block;
	readBlock(&block, &infile);

	PrimitiveBlock pb;
	PrimitiveGroup pg;
	DenseNodes dense;
	Way pbfWay;
	unsigned long nodeId;
	vector<string> strings(0);
	uint ct=0;
	while (!infile.eof()) {
		readBlock(&pb, &infile);

		// Read the string table
		readStringTable(&strings, &pb);
		map<string, int> tagMap;
		readStringMap(&tagMap, &pb);
		if (tagMap.find(highwayKey) == tagMap.end()) { ct++; continue; }

		// Read each group
		for (uint i=0; i<pb.primitivegroup_size(); i++) {
			pg = pb.primitivegroup(i);
			if (pg.ways_size() == 0) { continue; }

			for (uint j=0; j<pg.ways_size(); j++) {
				pbfWay = pg.ways(j);

				string highwayValue = getValue(&pbfWay, &tagMap, &strings, &highwayKey);
				if (highwayValue == "") { continue; }
				auto it = highwayCodes.find(highwayValue);
				if (it == highwayCodes.end()) { continue; }
				unsigned char code = it->second;

				string areaValue = getValue(&pbfWay, &tagMap, &strings, &areaKey);
				if (areaValue == "yes") { continue; }

				// Add to each node
				nodeId = 0;
				uint length = pbfWay.refs_size();
				for (uint k=0; k<length; k++) {
					nodeId += pbfWay.refs(k);
					junctionNodes[nodeId].push_back(code);
					if (k>0 && k<length-1) { junctionNodes[nodeId].push_back(code); }	// both 'in' and 'out'
				}
			}
		}
		ct++;
	}
	
	// Second pass - read nodes and write file
	
	ofstream outputFile;
	outputFile.open("junctions.csv");
	outputFile.precision(9);
	
	infile.clear();
	infile.seekg(0);
	while (!infile.eof()) {
		readBlock(&pb, &infile);
		for (uint i=0; i<pb.primitivegroup_size(); i++) {
			pg = pb.primitivegroup(i);
			if (!pg.has_dense()) { continue; }
			nodeId  = 0;
			int lon = 0;
			int lat = 0;
			dense = pg.dense();
			for (uint j=0; j<dense.id_size(); j++) {
				nodeId += dense.id(j);
				lon    += dense.lon(j);
				lat    += dense.lat(j);
				auto it = junctionNodes.find(nodeId);
				if (it==junctionNodes.end()) { continue; }
				vector<unsigned char> roads = it->second;
				if (roads.size()<3) { continue; }
				sort(roads.begin(), roads.end());

				outputFile << lat/10000000.0 << "," << lon/10000000.0 << ",";
				for (uint k=0; k<roads.size(); k++) {
					outputFile << roads[k]+0;
				}
				outputFile << endl;
			}
		}
	}
	outputFile.close();
	infile.close();
	google::protobuf::ShutdownProtobufLibrary();
}
