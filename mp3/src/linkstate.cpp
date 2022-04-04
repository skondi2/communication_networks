#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<map>
#include<string>
#include<fstream>
#include<vector>
#include<iostream>
#include<algorithm>
#include<climits>
#include<cstring>
#include<sstream>
#include<set>
using namespace std;

std::map<size_t, std::set<size_t> > adj_list;
std::map<std::pair<size_t, size_t>, int> edge_cost;
std::set<size_t> nodes;
//std::map<size_t, std::map<size_t, std::pair<size_t,int> > > forwarding_tables;
std::map<size_t, std::map<size_t, int> > distance_vectors;
std::map<size_t, std::map<size_t, size_t> > nextHops;





std::string convertSize_tToString(size_t num) {
  std::stringstream ss;
  ss << num;
  return ss.str();
}

size_t convertStringtoSize_t(std::string str) {
  std::stringstream ss(str);
  size_t result;
  ss >> result;
  return result;
}

int convertStringtoInt(std::string str) {
  std::stringstream ss(str);
  int result;
  ss >> result;
  return result;
}

std::string convertIntToString(int num) {
  std::stringstream ss;
  ss << num;
  return ss.str();
}
/*
void printAdjList() {
  for (auto it : adj_list) {
    std::cout << "adj list for node " << it.first << " : " << std::endl;

    auto it1 = it.second.begin();
    for (; it1 != it.second.end(); it1++) {
      std::cout << *it1 << " ";
    }
    std::cout << "" << std::endl;
  }
}

void printEdges() {
  std::cout << "PRINTING OUT EDGES" << std::endl;
  for (auto it : edge_cost) {
    std::cout << "pair = (" << it.first.first << ", " << it.first.second << ") = " << it.second << std::endl;
  }
}

void printNextHops() {
  std::cout << "PRINTING OUT NEXT HOPS" << std::endl;
  for (auto it : nextHops) {
    std::cout << "src node = " << it.first << std::endl;

    for (auto it1 : it.second) {
      std::cout << "dest node = " << it1.first << ", next hop = " << it1.second << std::endl;
    }
    std::cout << "" << std::endl;
  }
}

void printDistanceVectors() {
  for (auto it : distance_vectors) {
    std::cout << "current node = " << it.first << std::endl;

    for (auto it1 : it.second) {
      std::cout << "dest node = " << it1.first << ", cost = " << it1.second << std::endl;
    }
  }
}


void printVector(std::vector<std::string> vec) {
  std::cout << "vector contents = ";
  for (int i = 0; i < vec.size(); i++) {
    std::cout << vec[i] << " ";
  }
  std::cout << "" << std::endl;
}
*/

std::string getHops(std::vector<std::size_t> hops) {
  std::string hops_str = "";
  for (int i = 0; i < hops.size(); i++) {
    std::string hop = convertSize_tToString(hops[i]);
    hops_str += (hop + " ");
  }
  return hops_str;
}






size_t findLowestCostNode(std::vector<size_t> N, std::map<size_t, int> distance_vector) {
  int min_cost = INT_MAX;
  size_t min_cost_node;

  std::set<size_t>::iterator it = nodes.begin();
  for (; it != nodes.end(); it++) {
    size_t node = *it;

    if (std::find(N.begin(), N.end(), node) == N.end()) { // the node is not in N
      int cost = distance_vector[node];

      if (cost < min_cost) {
        min_cost = cost;
        min_cost_node = node;
      }

      if (cost == min_cost && node < min_cost_node) {
        min_cost_node = node;
      }

    }
  }

  return min_cost_node;
}




void storeForwardingTable(FILE* fpOut) {

  std::set<size_t>::iterator it = nodes.begin();
  for (; it != nodes.end(); it++) {
    std::map<size_t, int>& distance_vector = distance_vectors[*it];
    std::map<size_t, size_t>& next_hops = nextHops[*it];

    std::set<size_t>::iterator it1 = nodes.begin();
    for (; it1 != nodes.end(); it1++) {

      std::string dest = convertSize_tToString(*it1);
      std::string next_hop = convertSize_tToString(next_hops[*it1]);
      std::string cost = convertIntToString(distance_vector[*it1]);

      if (distance_vector[*it1] != INT_MAX) {
        std::string line = dest + " " + next_hop + " " + cost;
        const char* line_cstr = line.c_str();

        fwrite(line_cstr, 1, line.size(), fpOut);
        fwrite("\n", 1, 1, fpOut);
      }
    }
  }
}






void storeMessageOutput(int path_cost, std::vector<size_t> shortest_path,
  size_t src, size_t dest, std::string content, FILE* fpOut) {

    std::string line = "";
    std::string src_str = convertSize_tToString(src);
    std::string dest_str = convertSize_tToString(dest);
    std::string path_cost_str = convertIntToString(path_cost);

    if (path_cost == INT_MAX) {
      line = "from " + src_str + " to " + dest_str + " cost infinite hops unreachable message " + content;

      const char* line_cstr = line.c_str();
      fwrite(line_cstr, 1, line.size() - 1, fpOut);
    }
    else {
      line = "from " + src_str + " to " + dest_str + " cost " + path_cost_str + " hops " + getHops(shortest_path) + "message " + content;

      const char* line_cstr = line.c_str();
      fwrite(line_cstr, 1, line.size() - 1, fpOut);
    }

}


int getShortestPathHelper(size_t src, size_t dest, std::vector<size_t>& path) {
  std::map<size_t, int> curr_distance_vector = distance_vectors[src];
  int path_cost = curr_distance_vector[dest];

  //std::cout << "path cost that is stored = " << path_cost << std::endl;

  std::map<size_t, size_t> curr_next_hops = nextHops[src];
  //printNextHops();
  size_t curr = src;


  if (path_cost == INT_MAX) return path_cost;

  while (curr != dest) {
    if (curr == 0) return -1;
    if (std::find(path.begin(), path.end(), curr) != path.end()) return -1;

    path.push_back(curr);
    curr_next_hops = nextHops[curr];
    curr = curr_next_hops[dest];
    //std::cout << "curr is now set to = " << curr << std::endl;
  }

  return path_cost;
}



int getShortestPath(size_t src, size_t dest, std::vector<size_t>& path) {
    //std::cout << "GET SHORTEST PATH" << std::endl;
    //std::cout << "src = " << src << ", dest = " << dest << std::endl;

    std::map<size_t, int> curr_distance_vector = distance_vectors[src];
    int path_cost = curr_distance_vector[dest];

    //std::cout << "path cost that is stored = " << path_cost << std::endl;

    std::map<size_t, size_t> curr_next_hops = nextHops[src];
    //printNextHops();
    size_t curr = src;

    if (path_cost == INT_MAX) return path_cost;

    while (curr != dest) {
    //for (int i = 0; i < 5; i++) {
      path.push_back(curr);
      curr_next_hops = nextHops[curr];
      curr = curr_next_hops[dest];
      //std::cout << "curr is now set to = " << curr << std::endl;
    }

    return path_cost;
}




void changeTopology(size_t src, size_t dest, int cost_int) {

    std::pair<size_t, size_t> src_dest_edge = std::make_pair(src, dest);
    std::pair<size_t, size_t> dest_src_edge = std::make_pair(dest, src);

    if (cost_int == -999) { // removing edge
      if (edge_cost.find(src_dest_edge) != edge_cost.end()) {
        edge_cost.erase(src_dest_edge);
      }
      if (edge_cost.find(dest_src_edge) != edge_cost.end()) {
        edge_cost.erase(dest_src_edge);
      }

      std::set<size_t>& adj_nodes = adj_list[src];
      std::set<size_t>::iterator it2 = adj_nodes.find(dest);
      adj_nodes.erase(it2);

      std::set<size_t>& adj_nodes1 = adj_list[dest];
      std::set<size_t>::iterator it1 = adj_nodes1.find(src);
      adj_nodes1.erase(it1);
    }

    else { // editing edge
      if (edge_cost.find(src_dest_edge) != edge_cost.end()) {
        edge_cost.erase(src_dest_edge);
      }
      if (edge_cost.find(dest_src_edge) != edge_cost.end()) {
        edge_cost.erase(dest_src_edge);
      }
      edge_cost[src_dest_edge] = cost_int;
      edge_cost[dest_src_edge] = cost_int;

      std::set<size_t>& adj_nodes = adj_list[src];
      std::set<size_t>::iterator it2 = adj_nodes.find(dest);
      if (it2 == adj_nodes.end()) {
        adj_nodes.insert(dest);
      }

      std::set<size_t>& adj_nodes1 = adj_list[dest];
      std::set<size_t>::iterator it1 = adj_nodes1.find(src);
      if (it1 == adj_nodes1.end()) {
        adj_nodes1.insert(src);
      }
    }
}



void initNextHops() {
  std::set<size_t>::iterator it = nodes.begin();
  for (; it != nodes.end(); it++) {
    std::map<size_t, size_t>& next_hops = nextHops[*it];
    std::set<size_t> adj_nodes = adj_list[*it];

    std::set<size_t>::iterator it1 = nodes.begin();
    for (; it1 != nodes.end(); it1++) {
      if (adj_nodes.find(*it1) != adj_nodes.end()) {
        next_hops[*it1] = *it1;
      }
      else {
        next_hops[*it1] = 0;
      }
    }
    next_hops[*it] = *it;
  }
}


void dijkstra(size_t node) {
  std::map<size_t, int>& distance_vector = distance_vectors[node];
  std::map<size_t, size_t>& next_hops = nextHops[node];
  std::set<size_t> adj_nodes = adj_list[node];

  std::vector<size_t> N;
  N.push_back(node);

  std::set<size_t>::iterator it = nodes.begin();
  for (; it != nodes.end(); it++) {

    std::pair<size_t, size_t> edge = std::make_pair(node, *it);

    if (adj_nodes.find(*it) != adj_nodes.end()) {
      distance_vector[*it] = edge_cost[edge];
    }
    else {
      distance_vector[*it] = INT_MAX;
    }
  }
  distance_vector[node] = 0;

  while (N.size() != nodes.size()) {
    size_t w = findLowestCostNode(N, distance_vector);
    N.push_back(w);

    std::set<size_t> w_adj = adj_list[w];
    std::set<size_t>::iterator it1 = w_adj.begin();

    for (; it1 != w_adj.end(); it1++) {
      size_t v = *it1;
      if (std::find(N.begin(), N.end(), v) == N.end()) {

        std::pair<size_t, size_t> wv = std::make_pair(w, v);
        int new_cost = distance_vector[w] + edge_cost[wv];

        if (new_cost < distance_vector[v] && new_cost > 0) {
          distance_vector[v] = new_cost;
          next_hops[v] = next_hops[w];
        }

        if (new_cost == distance_vector[v] && new_cost > 0) {
          std::vector<size_t> path;
          //std::cout << "src = " << node << ", v = " << v << ", w = " << w << std::endl;
          int path_to_v = getShortestPathHelper(node, v, path);
          //std::cout << "src = " << node << ", v = " << v << ", w = " << w <<", path costs = " << path_to_v << std::endl;
          if (path_to_v == -1 || path[path.size() - 1] >= w) {
            next_hops[v] = next_hops[w];
          }
        }
      }
    }
  }

}






int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Usage: ./linkstate topofile messagefile changesfile\n");
        return -1;
    }

    char* topofile_title = argv[1];
    char* messagefile_title = argv[2];
    char* changesfile_title = argv[3];

    FILE *fpOut;
    fpOut = fopen("output.txt", "w");

    FILE* topofile_ptr = fopen(topofile_title, "r");
    if (topofile_ptr == NULL) {
      fprintf(stderr, "cannot open topofile");
    }


    char current_line[100];
    while (fgets(current_line, sizeof(current_line), topofile_ptr) != NULL) {

      char delim[] = " ";

      // get source node:
      std::string src(strtok(current_line, delim));
      std::string dest(strtok(NULL, delim));
      std::string cost(strtok(NULL, delim));

      size_t src_num = convertStringtoSize_t(src);
      size_t dest_num = convertStringtoSize_t(dest);
      int cost_num = convertStringtoInt(cost);

      if (nodes.find(src_num) == nodes.end()) nodes.insert(src_num); // add to nodes
      if (nodes.find(dest_num) == nodes.end()) nodes.insert(dest_num);

      std::pair<size_t, size_t> src_dest_edge = std::make_pair(src_num, dest_num);
      std::pair<size_t, size_t> dest_src_edge = std::make_pair(dest_num, src_num);

      edge_cost[src_dest_edge] = cost_num; // add to edge_cost
      edge_cost[dest_src_edge] = cost_num;

      if (adj_list.find(src_num) == adj_list.end()) {
        std::set<size_t> src_adj;
        src_adj.insert(dest_num);
        adj_list[src_num] = src_adj;
      }
      else {
        std::set<size_t>& src_adj = adj_list[src_num];
        src_adj.insert(dest_num);
      }

      if (adj_list.find(dest_num) == adj_list.end()) {
        std::set<size_t> dest_adj;
        dest_adj.insert(src_num);
        adj_list[dest_num] = dest_adj;
      }
      else {
        std::set<size_t>& src_adj = adj_list[dest_num];
        src_adj.insert(src_num);
      }
    }
    //printAdjList();
    //printEdges();

    initNextHops();
    distance_vectors.clear();

    std::set<size_t>::iterator it = nodes.begin();
    for (; it != nodes.end(); it++) {
      size_t node = *it;
      dijkstra(node);
    }

    //printDistanceVectors();
    //std::cout << "-----------------------" << std::endl;
    //printNextHops();
    storeForwardingTable(fpOut);


    FILE* messagefile_ptr = fopen(messagefile_title, "r");
    if (messagefile_ptr == NULL) {
      fprintf(stderr, "cannot open messagefile\n");
    }

    std::vector<std::vector<std::string> > message_file;
    char current_line1[100];
    fseek(messagefile_ptr, 0L, SEEK_END);
    size_t msg_file_size = ftell(messagefile_ptr);
    if (msg_file_size != 0) {
      rewind(messagefile_ptr);

      char* res;
      while ((res = fgets(current_line1, sizeof(current_line1), messagefile_ptr)) != NULL) {

        if (res == NULL || current_line1 == NULL) break;

        char delim[] = " ";

        char* ptr = strtok(current_line1, delim);
        std::string src(ptr);

        ptr = strtok(NULL, delim);
        std::string dest(ptr);

        std::string content(ptr + 2);

        std::vector<std::string> message_line;
        message_line.push_back(src);
        message_line.push_back(dest);
        message_line.push_back(content);
        message_file.push_back(message_line);


        size_t src_num = convertStringtoSize_t(src);
        size_t dest_num = convertStringtoSize_t(dest);


        std::vector<size_t> shortest_path;
        int path_cost = getShortestPath(src_num, dest_num, shortest_path);

        storeMessageOutput(path_cost, shortest_path, src_num, dest_num, content, fpOut);
        fwrite("\n", 1, 1, fpOut);
      }
    }
    fclose(messagefile_ptr);




    FILE* changesfile_ptr = fopen(changesfile_title, "r");
    if (changesfile_ptr == NULL) {
      fprintf(stderr, "cannot open changes file\n");
    }

    char current_line2[100];
    fseek(changesfile_ptr, 0L, SEEK_END);
    size_t changes_file_size = ftell(changesfile_ptr);

    if (changes_file_size != 0) {

      rewind(changesfile_ptr);
      char* res;
      while ((res = fgets(current_line2, sizeof(current_line2), changesfile_ptr)) != NULL) {
        //fprintf(stderr, "%s\n", current_line2);
        if (res == NULL || current_line2 == NULL) break;

        char delim[] = " ";

        std::string src(strtok(current_line2, delim));
        std::string dest(strtok(NULL, delim));
        std::string cost(strtok(NULL, delim));

        size_t src_num = convertStringtoSize_t(src);
        size_t dest_num = convertStringtoSize_t(dest);
        int cost_num = convertStringtoInt(cost);

        changeTopology(src_num, dest_num, cost_num);

        initNextHops();

        it = nodes.begin();
        for (; it != nodes.end(); it++) {
          size_t node = *it;
          dijkstra(node);
        }

        fwrite("\n", 1, 1, fpOut);
        storeForwardingTable(fpOut);


        //std::cout << "message_file.size() = " << message_file.size() << std::endl;
        for (int i = 0; i < message_file.size(); i++) {
          std::string msg_src = message_file[i][0];
          std::string msg_dest = message_file[i][1];
          std::string msg_content = message_file[i][2];

          size_t msg_src_num = convertStringtoSize_t(msg_src);
          size_t msg_dest_num = convertStringtoSize_t(msg_dest);

          std::vector<size_t> shortest_path;
          int path_cost = getShortestPath(msg_src_num, msg_dest_num, shortest_path);

          storeMessageOutput(path_cost, shortest_path, msg_src_num, msg_dest_num, msg_content, fpOut);
          fwrite("\n", 1, 1, fpOut);
        }
      }
    }
    fclose(changesfile_ptr);


    fclose(fpOut);


    return 0;
}
