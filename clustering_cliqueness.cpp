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
#include "clustering_cliqueness.hpp"



using namespace std;

struct Node_hash {
    inline std::size_t operator()(const Node& n) const {
        return pow(n.index, 31) + n.degree;
    }
};


inline bool operator == (Node const& n1, Node const& n2)
{
    return (n1.index == n2.index); 
}


vector<uint> removeDuplicates(vector<uint>& vec){
	sort(vec.begin(), vec.end());
	vec.erase(unique(vec.begin(), vec.end()), vec.end());
	return vec;
}

vector<double> removeDuplicatesCC(vector<double>& vec){
	sort(vec.begin(), vec.end());
	vec.erase(unique(vec.begin(), vec.end()), vec.end());
	return vec;
}


void DFS(uint n, vector<Node>& vecNodes, unordered_set<uint>& visited, set<uint>& nodesInConnexComp, bool& above, double cutoff){
	if (not visited.count(n)){
		if (vecNodes[n].CC >= cutoff){
			above = true;
		}
		visited.insert(n);
		nodesInConnexComp.insert(n);
		for (auto&& neigh : vecNodes[n].neighbors){
			DFS(neigh, vecNodes, visited, nodesInConnexComp, above, cutoff);
		}
	}
}

vector<string> split(const string &s, char delim){
	stringstream ss(s);
	string item;
	vector<string> elems;
	while (getline(ss, item, delim)) {
		elems.push_back(move(item)); 
	}
	return elems;
}



void parsingSRC(ifstream & refFile, vector<Node>& vecNodes, bool weighted){
	string listNodes;
	// header
	getline(refFile, listNodes);
	getline(refFile, listNodes);
	getline(refFile, listNodes);
	vector<uint>  neighbs;
	vector<vector<uint>> clust;
	vector<string> splitted1, splitted2, splitted3;
	uint read, target;
	unordered_map <uint, uint> seenNodes;
	unordered_map <uint, double> neighbToWeight;
	double weight(1);
	while (not refFile.eof()){
		getline(refFile, listNodes);
		splitted1 = split(listNodes, ':');
		if (splitted1.size() > 1){
			splitted2 = split(splitted1[1], ' ');
			target = stoi(splitted1[0]);  // target read's index
			if (not splitted2.empty()){
				for (uint i(0); i < splitted2.size(); ++i){
					splitted3 = split(splitted2[i], '-');
					read = stoi(splitted3[0]);  // recruited read
					if (weighted){
						weight = 1/ (stof(splitted3[1]));
					}
					if (read != target){
						if (not seenNodes.count(target)){ // new node not already in the vector of nodes
							clust = {}; neighbs = {}; neighbToWeight = {};
							Node t({target, 0, 0, clust, neighbs, neighbToWeight});
							vecNodes.push_back({t});  // store in vecNodes
							vecNodes.back().neighbToWeight.insert({read, weight});
							seenNodes.insert({target, vecNodes.size() - 1}); // remember this node has been pushed index -> place in the vector
						}
						if (seenNodes.count(read)){ // this neighbour is already in the vector of nodes
							vecNodes[seenNodes[target]].neighbors.push_back(seenNodes[read]);  // add read as neighbor of target
							if (not vecNodes[seenNodes[target]].neighbToWeight.count(read)){
								vecNodes[seenNodes[target]].neighbToWeight.insert({read, weight});
							}
							vecNodes[seenNodes[target]].neighbors.push_back(seenNodes[read]);  // add read as neighbor of target
							vecNodes[seenNodes[read]].neighbors.push_back(seenNodes[target]);  // add target as neighbor of read
							if (not vecNodes[seenNodes[read]].neighbToWeight.count(target)){
								vecNodes[seenNodes[read]].neighbToWeight.insert({target, weight});
							}
						} else {  // new neighbor not already in the vector of nodes
							clust = {}; neighbs = {}; neighbToWeight = {};
							Node r({read, 0, 0, clust, neighbs, neighbToWeight});
							vecNodes.push_back({r});
							uint position(vecNodes.size() - 1);
							seenNodes.insert({read, position});
							vecNodes[seenNodes[target]].neighbors.push_back(vecNodes.size() - 1);  // store read as neighbor of target
							vecNodes[seenNodes[read]].neighbors.push_back(seenNodes[target]);  // add target as neighbor of read
							vecNodes[seenNodes[read]].neighbToWeight.insert({target, weight}); 
							vecNodes[seenNodes[target]].neighbToWeight.insert({read, weight});  
						}
					}
				}
			}
		}   
	}
}


double getCC(unordered_set<uint>& neighbors, vector<Node>& vecNodes){
	double pairs(0), clusteringCoef(0);
	uint totalPairs;
	for (auto&& neigh : neighbors){  // for each neighbor of the node
		for (auto&& neigh2 : vecNodes[neigh].neighbors){ // for each neighbor of a neighbor
			if (neighbors.count(neigh2)){  // if a neighbor of a neighbor is also a neighbor of the current node = pair of connected neighbors
				++pairs;
			}
		}
	}
	totalPairs = neighbors.size() * (neighbors.size() - 1);
	if (totalPairs > 0){
		clusteringCoef = pairs/totalPairs;
	}
	return clusteringCoef;
}


int getDeltaCC(set<uint>& toRemove, set<uint>& clust1, vector<Node>& vecNodes, double cutoff){
	int deltaCC(0);
	unordered_set<uint> clust1Without;
	for (auto&& i : clust1){
		if (not toRemove.count(i)){
			clust1Without.insert(i);
		}
	}
	double CC1(getCC(clust1Without, vecNodes));
	deltaCC = cutoff - CC1;
	return deltaCC;
}


void computeCCandDeg(vector<Node>& vecNodes, vector<double>& ClCo, vector<uint>& degrees){
	double clusteringCoef;
	unordered_set<uint> neighbors;
	// start by removing double occurrences in neighbors
	for (uint n(0); n < vecNodes.size(); ++n){
		vecNodes[n].neighbors = removeDuplicates(vecNodes[n].neighbors);
		vecNodes[n].degree = vecNodes[n].neighbors.size();
		degrees.push_back(vecNodes[n].degree);
	}
	for (uint n(0); n < vecNodes.size(); ++n){
		if (vecNodes[n].neighbors.size() > 1){
			neighbors = {};
			copy(vecNodes[n].neighbors.begin(), vecNodes[n].neighbors.end(), inserter(neighbors, neighbors.end()));
			clusteringCoef = getCC(neighbors, vecNodes);
			if (clusteringCoef != 0){
				 vecNodes[n].CC = clusteringCoef;
				ClCo.push_back(clusteringCoef);
			}
		} else {
			ClCo.push_back(0);
		}
	}
	ClCo = removeDuplicatesCC(ClCo);
	sort(ClCo.begin(), ClCo.end());
	reverse(ClCo.begin(), ClCo.end());
}


void sortVecNodes(vector<Node>& vecNodes){
	unordered_map <uint, uint> indexReadsBef;
	for (uint i(0); i < vecNodes.size(); ++i){
		indexReadsBef.insert({vecNodes[i].index, i});
	}
	sort(vecNodes.begin(), vecNodes.end());
	reverse(vecNodes.begin(), vecNodes.end());
	unordered_map <uint, uint> indexReadsAf;
	for (uint i(0); i < vecNodes.size(); ++i){
		indexReadsAf.insert({indexReadsBef[vecNodes[i].index], i});  // former and new index in vecNodes
	}
	
	vector<uint> vec;
	
	for (uint i(0); i < vecNodes.size(); ++i){
		vec = {};
		for (auto&& n : vecNodes[i].neighbors){
			vec.push_back(indexReadsAf[n]);
		}
		vecNodes[i].neighbors = vec;
	}
}


void computePseudoCliques(vector<double>& cutoffs, vector<Node>& vecNodes, uint nbThreads, vector<uint>& nodesInOrderOfCC, uint higherDegree, float lowerCC){
	vector<uint> v;
	vector<vector<uint>> vec(cutoffs.size());
	for (uint i(0); i < vecNodes.size(); ++i){
		vecNodes[i].cluster = vec;
	}
	uint c(0);
	vector<unordered_set<uint>> temp(cutoffs.size());
	#pragma omp parallel num_threads(nbThreads)
	{
		#pragma omp for
		for (c = 0; c < cutoffs.size(); ++c){  // descending cutoffs
			unordered_set<uint> s;
			double cutoff = cutoffs[c];
			for (uint i(0); i < vecNodes.size(); ++i){
				if (vecNodes[i].CC >= cutoff and not (vecNodes[i].degree >= higherDegree and vecNodes[i].CC <= lowerCC)){
							vecNodes[i].cluster[c].push_back(i);
							s.insert(i);
							for (auto&& neigh : vecNodes[i].neighbors){
								vecNodes[neigh].cluster[c].push_back(i);
								s.insert(neigh);
							}
				}
			}
			temp[c] = s;
		}
	}
	unordered_set<uint> s;
	for (uint i(0); i < temp.size(); ++i){
		for (auto&& n : temp[i]){
			if (not s.count(n)){
				s.insert(n);
				nodesInOrderOfCC.push_back(n);  // nodes are ordered from belonging to a cluster of higher CC to lower
			}
		}
	}
}


double computeUnionCC(set<uint>& unionC, vector<Node>& vecNodes){
	double cardUnion(0);
	for (auto&& n : unionC){
		for (auto&& neigh : vecNodes[n].neighbors){
			if (unionC.count(neigh)){
				++cardUnion;
			}
		}
	}
	return cardUnion / ( unionC.size() * (unionC.size() - 1));
}


void transfer(uint tf, uint te, set<uint>& toFill, set<uint>& toEmpty, vector<Node>& vecNodes, vector<set<uint>>& clusters, uint ind){
	vector<uint> vec;
	for (auto&& index : toEmpty){
		vec = {};
		for (auto && clust : vecNodes[index].cluster[ind]){
			if (not (clust == te)){
				vec.push_back(clust);
			}
		}
		vec.push_back(tf);
		vecNodes[index].cluster[ind] = removeDuplicates(vec);
	}
	
	for (auto&& index : toFill){
		vec = {};
		for (auto && clust : vecNodes[index].cluster[ind]){
			if (not (clust == te)){
				vec.push_back(clust);
			}
		}
		vecNodes[index].cluster[ind] = removeDuplicates(vec);
	}
}


void merge(uint i1, uint i2, set<uint>& clust1, set<uint>& clust2, vector<set<uint>>& clusters,  vector<Node>& vecNodes, uint ind){
	if (clust1.size() > clust2.size()){  // merge in clust1
		clusters[i1].insert(clusters[i2].begin(), clusters[i2].end());
		transfer(i1, i2, clust1, clust2, vecNodes, clusters, ind);
		clusters[i2] = {};
	} else {  // merge in clust2
		clusters[i2].insert(clusters[i1].begin(), clusters[i1].end());
		transfer(i2, i1, clust2, clust1, vecNodes, clusters, ind);
		clusters[i1] = {};
	}
}



vector<set<uint>> assignNewClusters(set<uint>& clust, vector<Node>& vecNodes, double cutoff){
	bool above(false);
	unordered_set<uint> visited;
	set<uint> nodesInConnexComp;
	vector<set<uint>> newClust;
	for (auto&& node : clust){
		if (not visited.count(node)){
			above = false;
			nodesInConnexComp = {};
			DFS(node, vecNodes, visited, nodesInConnexComp, above, cutoff);
			if (above){
				newClust.push_back(nodesInConnexComp);
			}
		}
	}
	return newClust;
}


void removeSplittedElements(uint index, vector<set<uint>>& clusters, set<uint>& interC){
	set<uint> clust;
	for (auto && elt : clusters[index]){
		if (not interC.count(elt)){
			clust.insert(elt);
		}
	}
	clusters[index] = clust;
}


double splitClust(uint i1, uint i2, set<uint>& clust1, set<uint>& clust2, vector<set<uint>>& clusters,  vector<Node>& vecNodes, set<uint>& interC, uint cutoff, uint ind){
	double cut1(0), cut2(0), cut(0);
	for (auto&& node : interC){
		for (auto&& neigh : vecNodes[node].neighbors){
			if (clust1.count(neigh)){
				cut1 += vecNodes[node].neighbToWeight[vecNodes[neigh].index];
			}
			if (clust2.count(neigh)){
				cut2 += vecNodes[node].neighbToWeight[vecNodes[neigh].index];
			}
		}
	}

	
	if (clust1.size() == interC.size()){
		transfer(i1, i2, clust1, interC, vecNodes, clusters, ind);
		removeSplittedElements(i2, clusters, interC);
		cut = cut2;
	} else if (clust2.size() == interC.size()){
		transfer(i2, i1, clust2, interC, vecNodes, clusters, ind);
		removeSplittedElements(i1, clusters, interC);
		cut = cut1;
	} else {
		unordered_set <uint> neighbors;
		vector<set<uint>> newClust;

		if (cut1 == cut2){
			int deltaCC1(getDeltaCC(interC, clust1, vecNodes, cutoff));
			int deltaCC2(getDeltaCC(interC, clust2, vecNodes, cutoff));
			if (deltaCC1 <= deltaCC2){  // keep the intersection in clust1
				transfer(i1, i2, clust1, interC, vecNodes, clusters, ind);
				removeSplittedElements(i2, clusters, interC);
				cut = cut2;
			} else {  // keep the intersection in clust2
				transfer(i2, i1, clust2, interC, vecNodes, clusters, ind);
				removeSplittedElements(i1, clusters, interC);
				cut = cut1;
			}
		} else if (cut1 > cut2){  // split clust 2
			bool more(findArticulPoint(clust2, vecNodes, interC));
			transfer(i1, i2, clust1, interC, vecNodes, clusters, ind);
			removeSplittedElements(i2, clusters, interC);
			cut = cut2;   // todo * 2 ?
			if (more){
				newClust = assignNewClusters(clust2, vecNodes, cutoff);
				if (newClust.size() > 1){
					for (uint i(0); i < newClust.size(); ++i){
						clusters.push_back(newClust[i]);
						for (auto&& nodes: newClust[i]){
							vecNodes[nodes].cluster[ind].push_back(clusters.size() - 1);
						}
						transfer(clusters.size() - 1, i2, newClust[i], clust2, vecNodes, clusters, ind);
					}
					clusters[i2].clear();
				}
			}
		} else {
			// split clust1
			bool more(findArticulPoint(clust1, vecNodes, interC));
			transfer(i2, i1, clust2, interC, vecNodes, clusters, ind);
			removeSplittedElements(i1, clusters, interC);
			cut = cut1;
			if (more){
				newClust = assignNewClusters(clust1, vecNodes, cutoff);
				if (newClust.size() > 1){
					for (uint i(0); i < newClust.size(); ++i){
						clusters.push_back(newClust[i]);
						for (auto&& nodes: newClust[i]){
							vecNodes[nodes].cluster[ind].push_back(clusters.size() - 1);
						}
						transfer(clusters.size() - 1, i1, newClust[i], clust1, vecNodes, clusters, ind);
					}
					clusters[i1].clear();
				}
			}
		}
	}
	return cut;
}


double computeClustersAndCut(double cutoff, vector<Node>& vecNodes, vector<set<uint>>& clusters, uint ind, double prevCut, vector<uint>& nodesInOrderOfCC){
	double cut(0);
	set<uint> clust1, clust2, unionC, interC;
	uint i1, i2;
	double unionCC;
	for (uint n(0); n < vecNodes.size(); ++n){
		if (vecNodes[n].cluster[ind].size() > 0){
			for (auto&& c : vecNodes[n].cluster[ind]){
				clusters[c].insert(n);  // node at index n is in cluster c
			}
		}
	}
	double sCut(0);
	for (auto&& i : nodesInOrderOfCC){
		
		cut = 0;
		if (vecNodes[i].cluster[ind].size() > 1){  // node is in several clusters
			
			while (vecNodes[i].cluster[ind].size() > 1){
				i1 = vecNodes[i].cluster[ind][0];
				i2 = vecNodes[i].cluster[ind][1];
				clust1 = clusters[i1];
				clust2 = clusters[i2];
				interC = {};
				set_intersection(clust1.begin(), clust1.end(), clust2.begin(), clust2.end(), inserter(interC, interC.begin()));
				if (interC.size() == clust1.size() and clust1.size() == clust2.size()){  // clust1 and clust2 are the same
					transfer(i1, i2, clust1, interC,  vecNodes, clusters, ind);
					clusters[i2] = {};
				} else {
					unionC = {};
					set_union(clust1.begin(), clust1.end(), clust2.begin(), clust2.end(), inserter(unionC, unionC.begin()));
					unionCC = computeUnionCC(unionC, vecNodes);
					if (unionCC >= cutoff){  // merge
						merge(i1, i2, clust1, clust2, clusters, vecNodes, ind);
					} else {  // split
						cut += splitClust(i1, i2, clust1, clust2, clusters, vecNodes, interC, cutoff, ind);
					}
				}
			}
		} else {
				sCut += cut;
			if (ind > 0 and sCut > prevCut){
				return sCut;
			}
		}
	}

	cut = 0;
	vector <uint> seen(vecNodes.size(), 0);
	for (uint i(0); i < vecNodes.size(); ++i){
		if (vecNodes[i].cluster[ind].empty() and (not vecNodes[i].neighbors.empty())){
			for (auto&& ne : vecNodes[i].neighbors){
				cut += vecNodes[i].neighbToWeight[vecNodes[ne].index];
			}
		} else {
			if (not vecNodes[i].cluster[ind].empty()){
				for (auto&& ne : vecNodes[i].neighbors){
					if (not (clusters[vecNodes[i].cluster[ind][0]].count(ne))){
						cut += vecNodes[i].neighbToWeight[vecNodes[ne].index];
					}
				}
			} 
		}
	}
				
				
	return cut;
}

void getVecNodes(vector<Node>& vecNodes, vector<Node>& vecNodesGlobal, set<uint>& nodesInConnexComp){
	uint ii(0);
	unordered_map<uint, uint> indexReads;
	for (auto&& val: nodesInConnexComp){
		vecNodes.push_back(vecNodesGlobal[val]);
		indexReads.insert({val, ii});
		++ii;
	}
	vector<uint> vec;
	for (uint i(0); i < vecNodes.size(); ++i){
		vec = {};
		for (auto&& n : vecNodes[i].neighbors){
			vec.push_back(indexReads[n]);
		}
		vecNodes[i].neighbors = vec;
	}
}






bool findArticulPoint(set<uint>& cluster, vector<Node>& vecNodes, set<uint>& interC){
	Graph graph(vecNodes.size());
	unordered_set<uint> visited;
	for (auto&& i : cluster){
		visited.insert(i);
		for (auto&& neigh : vecNodes[i].neighbors){
			if ((not visited.count(neigh)) and cluster.count(neigh)){
				graph.addEdge((int)i, neigh);
			}
		}
	}
	vector<bool> ap; // To store articulation points
    bool b(graph.APBool(ap, interC));
    return b;
}


void preProcessGraph(vector<Node>& vecNodes, double cutoff=1.1){
	Graph graph(vecNodes.size());
	unordered_set<uint> visited;
	for (uint i(0); i < vecNodes.size(); ++i){
		visited.insert(i);
		for (auto&& neigh : vecNodes[i].neighbors){
			if (not visited.count(neigh)){
				graph.addEdge((int)i, neigh);
			}
		}
	}
	vector<uint> vec;
	vector<bool> ap; // To store articulation points
    graph.AP(ap);
    for (uint i = 0; i < vecNodes.size(); i++){
        if (ap[i] == true and vecNodes[i].CC < cutoff){
            for (auto&& j : vecNodes[i].neighbors){
				vec = {};
				for (auto&& jj : vecNodes[j].neighbors){
					if (i != jj){
						vec.push_back(jj);
					}
				}
				vecNodes[j].neighbors = vec;
				vecNodes[j].degree = vecNodes[j].neighbors.size();
			}
			vecNodes[i].neighbors = {};
			vecNodes[i].degree = 0;
		}
	}
}



void preProcessGraphQuantiles(vector<Node>& vecNodes, double cutoffCC, uint cutoffEdges){
	vector<uint> vec;
	for (uint i = 0; i < vecNodes.size(); i++){
        if (vecNodes[i].degree >= cutoffEdges and vecNodes[i].CC <= cutoffCC){
            for (auto&& j : vecNodes[i].neighbors){
				vec = {};
				for (auto&& jj : vecNodes[j].neighbors){
					if (i != jj){
						vec.push_back(jj);
					}
				}
				vecNodes[j].neighbors = vec;
				vecNodes[j].degree = vecNodes[j].neighbors.size();
			}
			vecNodes[i].neighbors = {};
			vecNodes[i].degree = 0;
		}
	}
}



uint quantileEdges(vector<uint>&degrees, uint no, uint q){
	double e;
	e = degrees.size()*((double)no/q);
	return (uint)e;
}


double quantileCC(vector<double>&CC, uint no, uint q){
	double cc;
	cc = CC.size()*((float)no/q);
	return cc;
}





int main(int argc, char** argv){
	bool printHelp(false);
	if (argc > 1){
		bool approx(false), preprocessing(false), weighted(false);
		string outFileName("final_g_clusters.txt"), fileName("");
		uint nbThreads(2);
		int c;
		uint granularity(10);

		while ((c = getopt (argc, argv, "f:o:c:i:pw")) != -1){
			switch(c){
				case 'o':
					outFileName=optarg;
					break;
				case 'f':
					fileName=optarg;
					break;
				case 'c':
					nbThreads=stoi(optarg);
					break;
				case 'i':
					approx = true;
					granularity=stoi(optarg);
					break;
				case 'p':
					preprocessing = true;
					break;
				case 'w':
					weighted = true;
					break;
			}
		}
		if (not (fileName.empty())){
			cout << "Command line was: " ;
			for (int a(0);  a < argc; ++a){
				cout << argv[a] << " ";
			}
			

			cout << endl;
			ifstream refFile(fileName);
			vector<Node> vecNodesGlobal;
			cout << "Parsing..." << endl;
			parsingSRC(refFile, vecNodesGlobal, weighted);
			if (preprocessing){
				cout << "preprocessing" << endl;
				preProcessGraph(vecNodesGlobal);
			}
			unordered_set<uint> visited;
			vector<set<uint>> nodesInConnexComp;
			bool b(false);
			for (uint n(0); n < vecNodesGlobal.size(); ++n){
				if (not (visited.count(n))){
					set<uint> s;
					DFS(n, vecNodesGlobal, visited, s, b, 0);
					nodesInConnexComp.push_back(s);
				}
			}
			cout << "Connected components: " << nodesInConnexComp.size() << endl;

			ofstream out(outFileName);
			mutex mm;

			vector<vector<uint>> finalClusters;
			ofstream outm("nodes_metrics.txt");
			vector<Node> vecNodes;
			vector<double>ClCo;
			vector<uint> degrees;
			vector<double>vecCC;
			double prev(1.1), cutoffTrunc;
			uint value;
			double minCut(0);
			vector<set<uint>> clustersToKeep;
			vector<uint> nodesInOrderOfCC;
			uint ccc(0);
			uint round(0);
			float lowerCC(0);
			uint higherDegree;
			for (uint c(0); c < nodesInConnexComp.size(); ++c){
				cout << "Connected Component " << c << " size " << nodesInConnexComp[c].size() << endl;
				vecNodes = {};
				getVecNodes(vecNodes, vecNodesGlobal, nodesInConnexComp[c]);

				cout << "Pre-processing of the graph" << endl;
				if (preprocessing){
					preProcessGraph(vecNodes);
				}

				ClCo = {};
				computeCCandDeg(vecNodes, ClCo, degrees);
				lowerCC = quantileCC(ClCo, 1, 1000);
				higherDegree = quantileEdges(degrees, 999, 1000);
				for (auto&& node : vecNodes){
					outm << node.index << " " << node.CC << " " << node.neighbors.size() << endl;
				}
				vecCC = {};
				if (approx){
					prev = 1.1;
					if (ClCo.size() > 100){
						value = granularity;
					} else {
						value = 0;
					}
					for (auto&& cutoff: ClCo){
						if (value != 0){
							cutoffTrunc = trunc(cutoff * value)/value;
						} else {
							cutoffTrunc = cutoff;
						}
						if (cutoffTrunc < prev){
							prev = cutoffTrunc;
							vecCC.push_back(cutoffTrunc);
						}
					}
				} else {
					for (auto&& cutoff: ClCo){
						vecCC.push_back(cutoff);
					}
				}
				minCut = 0;
				clustersToKeep = {};
				ccc = 0;
				cout << "Computing pseudo cliques" << endl;
				nodesInOrderOfCC = {};
				computePseudoCliques(vecCC, vecNodes, nbThreads, nodesInOrderOfCC, higherDegree, lowerCC);
				round = 0;
				
				cout <<  vecCC.size() << " clustering coefficients to check" << endl;
				bool compute(true);
				#pragma omp parallel num_threads(nbThreads)
				{
					#pragma omp for
					for (ccc = 0; ccc < vecCC.size(); ++ccc){
						double cut, prevCut;
						double cutoff(vecCC[ccc]);
						if (ccc != 0){
							if (approx and cutoff == 0 and ccc == vecCC.size() - 1){
								mm.lock();
								compute = false;
								mm.unlock();
							}
							if (cut > prevCut){
								prevCut = cut;
							}
						}
						if (compute){
							vector<Node> vecNodesCpy = vecNodes;
							vector<set<uint>> clusters(vecNodesCpy.size());
							vector<uint> nodesInOrderOfCCcpy = nodesInOrderOfCC;
							cout << "Computing clusters" << endl;
							cut = computeClustersAndCut(cutoff, vecNodesCpy, clusters, ccc, prevCut, nodesInOrderOfCCcpy);
							mm.lock();
							cout << round + 1 << "/" << vecCC.size() << " cutoff " << cutoff << " cut " << cut << endl;
							++round;
							mm.unlock();
							if (ccc == 0){
								mm.lock();
								minCut = cut;
								clustersToKeep = clusters;
								if (not weighted){
									if (minCut == 0 and cutoff == 1){
										compute = false;  // clique => stop
									}
								}
								mm.unlock();
							} else {
								if (not weighted){
									if (cut < minCut and cut > 0){
										mm.lock();
										minCut = cut;
										clustersToKeep = clusters;
										mm.unlock();
									}
								} else {
									if (cut < minCut){
										mm.lock();
										minCut = cut;
										clustersToKeep = clusters;
										mm.unlock();
									}
								}
							}
						}
					}
				}
				for (uint i(0); i < clustersToKeep.size(); ++i){
					if (not clustersToKeep[i].empty()){
						for (auto&& n : clustersToKeep[i]){
							out << vecNodes[n].index << " " ;
						}
						out << endl;
					}
				}
			}
			cout << "Done." << endl;
		} else {
			printHelp = true;
		}
	} else {
		printHelp = true;
	}
	if (printHelp){
		cout << "Usage : ./clustering_cliqueness -f input_file (-o output_file -i -p -c nb_cores)" << endl;
		cout << "-f is mandatory"  << endl << "-i performs inexact and speeder research" << endl << "-p performs pre processing step" << endl << "-c gets the number of threads (default 2)" << endl;
		cout << "Output written in final_g_clusters.txt" << endl;
	}
}
