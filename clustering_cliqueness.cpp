
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

// awaits  input lines like this 0:50-25-50.000000 27-32-64.000000, ignores the positions information
void parsingSRC(ifstream & refFile, unordered_map <uint, unordered_set<uint>>& nodeToNeighbors){
	string listNodes;
	// header
	getline(refFile, listNodes);
	getline(refFile, listNodes);
	getline(refFile, listNodes);
	vector<string> splitted1, splitted2, splitted3;
	uint read, target;
	while (not refFile.eof()){
		getline(refFile, listNodes);
		splitted1 = split(listNodes, ':');
		if (splitted1.size() > 1){
			splitted2 = split(splitted1[1], ' ');
			unordered_set<uint> reads;
			target = stoi(splitted1[0]);  // target read
			if (not splitted2.empty()){
				for (uint i(0); i < splitted2.size(); ++i){
					splitted3 = split(splitted2[i], '-');
					read = stoi(splitted3[0]);  // recruited read
					if (read != target){
						reads.insert(read);
						if (nodeToNeighbors.count(read)){
							nodeToNeighbors[read].insert(target);
						} else {
							nodeToNeighbors.insert({read, {target}});
						}
					}
				}
			}
			if (nodeToNeighbors.count(target)){
				for (auto&& n : reads){
					nodeToNeighbors[target].insert(n);
				}
			} else {
				nodeToNeighbors.insert({target, reads});
			}
		}   
	}
}

void computeCCandDeg(unordered_map <uint, unordered_set<uint>>& nodeToNeighbors, unordered_map <uint, pair<float, uint>>& nodeToMetrics, map <uint, vector<uint>, std::greater<uint>>& degToNode, set<float, std::greater<float>>& CC){
	uint deg;
	float clusteringCoef(0), pairs(0);
	for (auto node(nodeToNeighbors.begin()); node != nodeToNeighbors.end(); ++node){
		unordered_set <uint> neighbors(node->second.begin(), node->second.end());
		float clusteringCoef(0);
		float pairs(0);
		deg = neighbors.size();
		if (deg > 1){
			for (auto&& neigh : node->second){  // for each neighbor of the node
				for (auto&& neigh2 : nodeToNeighbors.at(neigh)){ // for each neighbor of a neighbor
					if (neighbors.count(neigh2)) {  // if a neighbor of a neighbor is also a neighbor of the current node = pair of connected neighbors
						++pairs;
					}
				}
			}
		}
		uint totalPairs(deg * (deg - 1));
		if (totalPairs > 0){
			clusteringCoef = pairs/totalPairs;
		}
		nodeToMetrics.insert({node->first, {clusteringCoef, deg}});
		if (degToNode.count(deg)){
			degToNode[deg].push_back(node->first);
		} else {
			degToNode.insert({deg, {node->first}});
		}
		CC.insert(clusteringCoef);
	}
}


void computePseudoCliques(float cutoff, map <uint, vector<uint>, std::greater<uint>>& degToNode, unordered_map <uint, set<uint>>& pcliqueToNodes, unordered_map <uint, unordered_set<uint>>& nodeToPCliques, unordered_map <uint, pair<float, uint>>& nodeToMetrics, uint& nbPCliques, unordered_map <uint, unordered_set<uint>>& nodeToNeighbors, unordered_set<uint>& pCliquesAboveThresh){
	unordered_set<uint> neighbors;
	for (auto deg(degToNode.begin()); deg != degToNode.end(); ++deg){
		for (auto&& node : deg->second){  // for all nodes by decreasing degrees
			if (nodeToMetrics[node].first >= cutoff){  // do a new pseudo clique from this node
				if (nodeToPCliques.count(node)){ 
					nodeToPCliques[node].insert(nbPCliques);
				} else {
					nodeToPCliques.insert({node, {nbPCliques}});
				}
				pcliqueToNodes.insert({nbPCliques, {node}});
				neighbors = nodeToNeighbors[node];
				for (auto&& neigh: neighbors){  // include neighbours in the pseudo clique
					if (nodeToPCliques.count(neigh)){
						nodeToPCliques[neigh].insert(nbPCliques);
					} else {
						nodeToPCliques.insert({neigh, {nbPCliques}});
					}
					pcliqueToNodes[nbPCliques].insert(neigh);
				}
				 pCliquesAboveThresh.insert(nbPCliques);
				++ nbPCliques;
			} else {  // nodes under the threshold
				if (not nodeToPCliques.count(node)){  // if the node is not already in a cluster create a singleton
					nodeToPCliques.insert({node, {nbPCliques}});
					pcliqueToNodes[nbPCliques].insert(node);
					++ nbPCliques;
				}
			}
		}
	}
}



void mergeProcedure(unordered_map <uint, set<uint>>& pcliqueToNodes, uint indexC1, uint indexC2, unordered_map <uint, uint>& nodeToCluster, bool merginFirst=false){
	 
	uint nb(0);
	for (auto p(pcliqueToNodes.begin()); p != pcliqueToNodes.end(); ++p){
		if (not p->second.empty()){
			++nb;
		}
	}
	
	if (pcliqueToNodes[indexC1].size() >= pcliqueToNodes[indexC2].size() or merginFirst){  // merge c2 in c1
		pcliqueToNodes[indexC1].insert(pcliqueToNodes[indexC2].begin(), pcliqueToNodes[indexC2].end());
		pcliqueToNodes[indexC2].clear();
		for (auto&& node : pcliqueToNodes[indexC1]){
			if (nodeToCluster.count(node)){
				nodeToCluster[node] = indexC1;
			} else {
				nodeToCluster.insert({node, indexC1});
			}
		}
	} else { // merge c1 in c2
		pcliqueToNodes[indexC2].insert(pcliqueToNodes[indexC1].begin(), pcliqueToNodes[indexC1].end());
		pcliqueToNodes[indexC1].clear();
		for (auto&& node : pcliqueToNodes[indexC2]){
			if (nodeToCluster.count(node)){
				nodeToCluster[node] = indexC2;
			} else {
				nodeToCluster.insert({node, indexC2});
			}
		}
	}
	nb = 0;

}


void assignClusterDFS(uint node, unordered_map <uint, set<uint>>& temporaryClusters, unordered_map <uint, uint>& temporaryNodesToClusters, uint& indexCluster, unordered_map <uint, unordered_set<uint>>& nodeToNeighbors, set<uint>& pcliqueOut, set<uint>& pcliqueIn, unordered_set<uint>& pCliquesAboveThresh, unordered_map <uint, pair<float, uint>>& nodeToMetrics, float cutoff){
		//~ cout << "in" << endl;
		bool above(false);
		if (temporaryClusters.count(indexCluster)){
			temporaryClusters[indexCluster].insert(node);
			temporaryNodesToClusters.insert({node, indexCluster});
		} else {
			temporaryClusters.insert({indexCluster, {node}});
			temporaryNodesToClusters.insert({node, indexCluster});
			if (nodeToMetrics[node].first >= cutoff){
				pCliquesAboveThresh.insert(indexCluster);
			}
		}
		for (auto&& neigh : nodeToNeighbors[node]){
			
			if (not temporaryNodesToClusters.count(neigh)){
				if (not (pcliqueOut.count(neigh)) and pcliqueIn.count(neigh)){
					assignClusterDFS(neigh, temporaryClusters, temporaryNodesToClusters, indexCluster, nodeToNeighbors, pcliqueOut, pcliqueIn, pCliquesAboveThresh, nodeToMetrics, cutoff);
				}
			}
		}
	//~ cout << "out" << endl;
}

uint cutProcedure(set<uint>& interC, unordered_map <uint, unordered_set<uint>>& nodeToNeighbors, unordered_map <uint, set<uint>>& pcliqueToNodes, uint indexC1, uint indexC2, unordered_map <uint, uint>& nodeToCluster, unordered_set<uint>& pCliquesAboveThresh, unordered_map <uint, pair<float, uint>>& nodeToMetrics, float cutoff, bool& modif){
	uint cut1(0), cut2(0), cut(0);
	for (auto&& node : interC){
		for (auto&& neigh : nodeToNeighbors[node]){
			if (pcliqueToNodes[indexC1].count(neigh)){
				++cut1;
			} else if (pcliqueToNodes[indexC2].count(neigh)){
				++cut2;
			}
		}
	}
	if (cut1 <= cut2){  // transfer nodes in cluster 2
		for (auto&& node : interC){
			pcliqueToNodes[indexC1].erase(node);
			if (nodeToCluster.count(node)){
				nodeToCluster[node] = indexC2;
			} else {
				nodeToCluster.insert({node, indexC2});
			}
		}
		// nodes from the cluster that transferred can be disjoint
		unordered_map <uint, set<uint>> temporaryClusters;
		unordered_map <uint,uint> temporaryNodesToClusters;
		uint indexCluster(pcliqueToNodes.size());
		//~ cout << "in" << endl;
		for (auto&& node : pcliqueToNodes[indexC1]){
			if (not temporaryNodesToClusters.count(node)){
				
				assignClusterDFS(node, temporaryClusters, temporaryNodesToClusters, indexCluster, nodeToNeighbors, interC, pcliqueToNodes[indexC1], pCliquesAboveThresh,nodeToMetrics, cutoff);
				++indexCluster;
			}
		}
		//~ cout << "out" << endl;
		if (temporaryClusters.size() > 1){  // several connected component in the remaining c1
			modif = true;
			pcliqueToNodes[indexC1] = {};
			for (auto newClust(temporaryClusters.begin()); newClust != temporaryClusters.end(); ++newClust){
				pcliqueToNodes.insert({newClust->first , newClust->second});
			}
		} else {
			pCliquesAboveThresh.erase(pcliqueToNodes.size());
		}
		cut += cut2;
	} else {  // transfer nodes in cluster 1
		for (auto&& node : interC){
			pcliqueToNodes[indexC2].erase(node);
			if (nodeToCluster.count(node)){
				nodeToCluster[node] = indexC1;
			} else {
				nodeToCluster.insert({node, indexC1});
			}
		}

		unordered_map <uint, set<uint>> temporaryClusters;
		unordered_map <uint, uint> temporaryNodesToClusters;
		uint indexCluster(pcliqueToNodes.size());
		//~ cout << "in" << endl;

		for (auto&& node : pcliqueToNodes[indexC2]){
			if (not temporaryNodesToClusters.count(node)){
				assignClusterDFS(node, temporaryClusters, temporaryNodesToClusters, indexCluster, nodeToNeighbors, interC, pcliqueToNodes[indexC2], pCliquesAboveThresh, nodeToMetrics, cutoff);
				++indexCluster;
			}
		}
		//~ cout << "out" << endl;

		if (temporaryClusters.size() > 1){  // several connected component in the remaining c2
			modif = true;
			pcliqueToNodes[indexC2] = {};
			for (auto newClust(temporaryClusters.begin()); newClust != temporaryClusters.end(); ++newClust){
				pcliqueToNodes.insert({newClust->first , newClust->second});
			}
		} else {
			pCliquesAboveThresh.erase(pcliqueToNodes.size());
		}
		cut += cut1;
	}
	return cut;
}


uint  computeClustersAndCut(unordered_map <uint, set<uint>>& pcliqueToNodes, unordered_map <uint, unordered_set<uint>>& nodeToNeighbors, float cutoff, unordered_set<uint>& nodeSingletons, unordered_set<uint>& pCliquesAboveThresh,  unordered_map <uint, pair<float, uint>>& nodeToMetrics){
	float unionCC(0);
	uint cut(0);
	bool modif(false), modif1(true);
	unordered_map <uint, uint> nodeToCluster;
	cout << "nb clusters:" << pcliqueToNodes.size() << endl;
	while (modif1){
		modif1 = false;
		for (auto clust1(pcliqueToNodes.begin()); clust1 != pcliqueToNodes.end(); ){
			//~ cout << "i:" << clust1->first << endl;
			//~ if (clust1->first == 7515){
				//~ cin.get();
			//~ }
			if (not clust1->second.empty()){
				//~ if (clust1->first == 7515){
					//~ cout << "not empty" << endl;
				//~ }
				for (auto clust2(pcliqueToNodes.begin()); clust2 != pcliqueToNodes.end(); ){
					//~ if (clust1->first == 7515){
						//~ cout << "j:" << clust2->first << endl;
					//~ }
					modif = false;
					if (clust1->first != clust2->first){
						if (not (clust2->second.empty() and pCliquesAboveThresh.count(clust1->first) and pCliquesAboveThresh.count(clust2->first))){
							if (clust1 != clust2){
								set<uint> unionC;
								set<uint> interC;
								unionCC = 0;
								set_intersection(clust1->second.begin(), clust1->second.end(), clust2->second.begin(), clust2->second.end(), inserter(interC, interC.begin()));
								if (not interC.empty()){  // some nodes belong to both clusters
									modif = true;
									if (interC.size() == clust1->second.size() and interC.size() == clust2->second.size()){
										clust2->second = {};
									} else {
										set_union(clust1->second.begin(), clust1->second.end(), clust2->second.begin(), clust2->second.end(), inserter(unionC, unionC.begin()));
										float cardUnion(0);
										for (auto&& node : unionC){
											for (auto&& neigh : nodeToNeighbors[node]){
												if (unionC.count(neigh)){
													++cardUnion;
												}
											}
										}
										unionCC = cardUnion / (unionC.size() * (unionC.size() - 1));
										if (unionCC >= 100 * cutoff/100){
											//~ cout << "merge" << endl;
											mergeProcedure(pcliqueToNodes, clust1->first, clust2->first, nodeToCluster);
											modif1 = true;
										} else {
											//~ cout << "cut" << endl;
											cutProcedure(interC, nodeToNeighbors,  pcliqueToNodes, clust1->first, clust2->first, nodeToCluster, pCliquesAboveThresh, nodeToMetrics, cutoff, modif1);
								
										}
									}
								}
							}
						} else if (not (clust2->second.empty() and pCliquesAboveThresh.count(clust1->first))){
							if (clust1 != clust2){
								
								set<uint> interC;
								set_intersection(clust1->second.begin(), clust1->second.end(), clust2->second.begin(), clust2->second.end(), inserter(interC, interC.begin()));
								if (not interC.empty()){
									modif = true;
									//~ cout << "merge" << endl;
									mergeProcedure(pcliqueToNodes, clust1->first, clust2->first, nodeToCluster, true);
									modif1 = true;
								}
							}
						} else if (not (clust2->second.empty() and pCliquesAboveThresh.count(clust2->first))){
							if (clust1 != clust2){
								set<uint> interC;
								set_intersection(clust1->second.begin(), clust1->second.end(), clust2->second.begin(), clust2->second.end(), inserter(interC, interC.begin()));
								if (not interC.empty()){
									modif = true;
									//~ cout << "merge" << endl;
									mergeProcedure(pcliqueToNodes, clust2->first, clust1->first, nodeToCluster, true);
									modif1 = true;
								}
							}
						}
					} else {
						//~ if (clust1->first == 7515){
						//~ cout << "same" << endl;
						//~ }
					}
					if (not modif){
						++clust2;
					}
				}
			} else {
				//~ if (clust1->first == 7515){
					//~ cout <<  "empty" << endl;
				//~ }

				modif = false;
			}
			if (not modif){
				++clust1;
			}
		}
	}
	cut = 0;
	cout << "final cut" << endl;
	//~ for (auto clust1(pcliqueToNodes.begin()); clust1 != pcliqueToNodes.end(); ++clust1){
		//~ cout << "i:" << clust1->first << endl;
		//~ for (auto clust2(pcliqueToNodes.begin()); clust2 != pcliqueToNodes.end(); ++clust2){
			//~ if ((clust1->first > clust2->first) and not (clust1->second.empty() or clust2->second.empty())){
				//~ if (clust1->second.size() < clust2->second.size()){
					//~ for (auto && node : clust1->second){
						//~ for (auto&& n : nodeToNeighbors[node]){
							//~ if (clust2->second.count(n)){
								//~ ++cut;
							//~ }
						//~ }
					//~ }
				//~ } else {
					//~ for (auto && node : clust2->second){
						//~ for (auto&& n : nodeToNeighbors[node]){
							//~ if (clust1->second.count(n)){
								//~ ++cut;
							//~ }
						//~ }
					//~ }
				//~ }
			//~ }
		//~ }
	//~ }
	for (auto clust1(pcliqueToNodes.begin()); clust1 != pcliqueToNodes.end(); ++clust1){
		for (auto && node : clust1->second){
			for (auto&& n : nodeToNeighbors[node]){
				if (not (clust1->second.count(n))){
					++cut;
				}
			}
		}
	}
	//~ return cut;
	return cut/2;
}


int main(int argc, char** argv){

	if (argc > 1){
		string outFileName("final_g_clusters.txt");
		if (argc > 2){
			outFileName = argv[2];
		}
		ofstream out(outFileName);
		string fileName(argv[1]);
        ifstream refFile(fileName);
        unordered_map <uint, unordered_set<uint>> nodeToNeighbors;
        // parse SRC's output, for each line we get a node and its neighbors and fill the map nodeToNeighbors
        cout << "Parsing..." << endl;
		parsingSRC(refFile, nodeToNeighbors);
		unordered_map <uint, pair<float, uint>> nodeToMetrics;
		map <uint, vector<uint>, std::greater<uint>> degToNode;
		set <float, std::greater<float>> CC;
		// get CC and degree for each node
		cout << "Computing graph..." << endl;
		computeCCandDeg(nodeToNeighbors, nodeToMetrics, degToNode, CC);
		ofstream outm("nodes_metrics.txt");
		for (auto node(nodeToMetrics.begin()); node != nodeToMetrics.end(); ++node){
			outm << node->first << " " << node->second.first << " " << node->second.second << endl; 
		}
		unordered_map <uint, set<uint>> pcliqueToNodes;
		unordered_map <uint, unordered_set<uint>> nodeToPCliques;
		uint nbPCliques(0), cut(0);
		vector<uint> globalCut;
		// compute min cut for each clustering coef value
		unordered_set<uint> nodeSingletons;
		unordered_set<uint> pCliquesAboveThresh;
		cout << "Testing cutoff values..." << endl;
		float prev(1.1), cutoffTrunc;
		for (auto&& cutoff: CC){
			cutoffTrunc = trunc(cutoff * 100)/100;
			if (cutoffTrunc < prev){
				prev = cutoffTrunc;
				cout << "cutoff:" << cutoffTrunc << endl;
				nbPCliques = 0;
				pcliqueToNodes = {};
				nodeToPCliques = {};
				nodeSingletons = {};
				pCliquesAboveThresh = {};
				cout << "...Compute all pseudo cliques..." << endl;
				computePseudoCliques(cutoffTrunc, degToNode, pcliqueToNodes, nodeToPCliques, nodeToMetrics, nbPCliques, nodeToNeighbors, pCliquesAboveThresh);
				cout << "...Make clusters..." << endl;
				cut = computeClustersAndCut(pcliqueToNodes, nodeToNeighbors, cutoffTrunc, nodeSingletons, pCliquesAboveThresh, nodeToMetrics);
				globalCut.push_back(cut);
				
			} else {
				globalCut.push_back(globalCut.back());
			}
		}
		uint i(0), minCut(0), val(0);
		// get the min cut over all cc values
		for (auto&& cut : globalCut){
			if (i == 0){  // if the graph is composed only of clique the mincut will be zeros since the first cc
				minCut = cut;
				val = i;
			} else {
				if (cut < minCut and cut > 0){
					val = i;
					minCut = cut;
				}
			}
			++i;
		}
		
		uint index(0);
		
		for (auto&& cutoff: CC){
				if (index == val){
					cout <<"computing final cluster with cutoff:" <<  cutoffTrunc << " and min cut:" << minCut << endl;
					pcliqueToNodes = {};
					nodeToPCliques = {};
					nbPCliques = 0;
					nodeSingletons = {};
					pCliquesAboveThresh = {};
					computePseudoCliques(cutoffTrunc, degToNode, pcliqueToNodes, nodeToPCliques, nodeToMetrics, nbPCliques, nodeToNeighbors, pCliquesAboveThresh);
					computeClustersAndCut(pcliqueToNodes, nodeToNeighbors, cutoffTrunc, nodeSingletons, pCliquesAboveThresh, nodeToMetrics);
					break;
				}
				++index;
			
		}
		for (auto p(pcliqueToNodes.begin()); p != pcliqueToNodes.end(); ++p){
			if (not p->second.empty()){
				for (auto&& node : p->second){
					out << node << " " ;
				}
				out << endl;
			}
		}
	} else {
		cout << "Usage : ./clustering_cliqueness src_output" << endl;
		cout << "Output written in final_g_clusters.txt" << endl;
	}
}
