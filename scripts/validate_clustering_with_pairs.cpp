#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iterator>
#include <ctime>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <unordered_set>
#include <set>
#include <algorithm>
#include <map>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sstream>
#include <mutex>
#include <functional>
#include <utility>



using namespace std;




vector<string> split(const string &s, char delim){
	stringstream ss(s);
	string item;
	vector<string> elems;
	while (getline(ss, item, delim)) {
		elems.push_back(move(item)); 
	}
	return elems;
}

void parsing(ifstream & file, vector<vector<uint>>& clustersOfReads, uint& nbClusters, uint& nbReads){
	string cluster;
	vector<string> vecClustS;
	vector<uint> vecClust;
	nbClusters = 0;
	nbReads = 0;
	uint i(0), nr(0);
	while (not file.eof()){
		getline(file, cluster);
		vecClustS = split(cluster, ' ');
		for (uint i(0); i < vecClustS.size(); ++i){
			vecClust.push_back(stoi(vecClustS[i]));
		}
		if (not vecClust.empty()){
			clustersOfReads.push_back(vecClust);
			nr += vecClust.size();
			++i;
		}
	}
	nbClusters = i;
	nbReads = nr;
}




void makePairs(vector<vector<uint>>& clustersOfReads, set<pair<uint, uint>>& pairsOfReads, unordered_map<uint, uint>& nbPairsPerCluster){
	uint nbPairs;
	for (uint i(0); i < clustersOfReads.size(); ++i){
		nbPairs = 0;
		for (uint read1(0); read1 < clustersOfReads[i].size() - 1; ++read1){
			for (uint read2(1); read2 < clustersOfReads[i].size(); ++read2){
				if (clustersOfReads[read1] < clustersOfReads[read2]){
					pair<uint, uint> p({read1, read2});
					pairsOfReads.insert(p);
					++nbPairs;
				}
			}
		}
		nbPairsPerCluster[i] = nbPairs;
	}
}


void checkPairs(vector<vector<uint>>& resultClustersOfReads, set<pair<uint, uint>>& truePairsOfReads, unordered_map<uint, uint>& nbPairsPerCluster, vector<double>& results){
	uint tp, fp, fn;
	uint TP(0), FP(0), FN(0);
	for (uint i(0); i < resultClustersOfReads.size(); ++i){
		tp =0; fp =0; fn =0;
		for (uint read1(0); read1 < resultClustersOfReads[i].size() - 1; ++read1){
			for (uint read2(1); read2 < resultClustersOfReads[i].size(); ++read2){
				if (resultClustersOfReads[read1] < resultClustersOfReads[read2]){
					pair<uint, uint> p({read1, read2});
					if (truePairsOfReads.count(p)){
						++tp;
					} else {
						++fp;
					}
				}
			}
		}
		fn = nbPairsPerCluster[i] - tp;
		TP += tp;
		FP += fp;
		FN += fn;
	}
	results.push_back(tp), results.push_back(fp), results.push_back(fn);
}


int main(int argc, char** argv){
	if (argc > 2){
		ifstream trueClustersFile(argv[1]);
		ifstream resultClustersFile(argv[2]);
		vector<vector<uint>> trueClustersOfReads, resultClustersOfReads;
		// parse true cluster file and result cluster file
		vector<uint> trueClusterToRead, resultClusterToRead;
		unordered_map<uint, uint> nbPairsPerCluster;
		uint nbTrueClusters, nbResultClusters, nbReadsTrue, nbReadsResult, unassignedReads;
		parsing(trueClustersFile, trueClustersOfReads, nbTrueClusters, nbReadsTrue);
		parsing(resultClustersFile, resultClustersOfReads, nbResultClusters, nbReadsResult);
		set<pair<uint, uint>> truePairsOfReads;
		makePairs(trueClustersOfReads, truePairsOfReads, nbPairsPerCluster);
		vector<double> results;
		checkPairs(resultClustersOfReads, truePairsOfReads,  nbPairsPerCluster,  results);

		
		float recall(0), precision(0);
		if (not results.empty()){
			if (results[0]> 0){
				recall = results[0]/(results[0] + results[2]);
				precision = results[1]/(results[0]);
			} else {
				recall = 0;
				precision = 0;
			}
		}
		cout << "Recall " << recall << endl;
		cout << "Precision " << precision << endl;
		cout << "F-measure " <<  2 * precision * recall / (precision + recall) << endl;
	}
}