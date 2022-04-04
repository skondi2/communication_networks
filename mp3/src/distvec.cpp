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

// global variables:
std::set<size_t> nodes;
std::map<size_t, std::map<std::pair<std::size_t,size_t>, int> > distance_vectors;
std::map<size_t, std::map<size_t, size_t> > nextHops;
std::map<std::pair<size_t, size_t>, int> edge_cost;


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

/*void printVector(std::vector<std::string> vec) {
  std::cout << "vector contents = ";
  for (int i = 0; i < vec.size(); i++) {
    std::cout << vec[i] << " ";
  }
  std::cout << "" << std::endl;
}
void printEdges(std::map<std::pair<std::string, std::string>, int> edge_cost) {
  std::cout << "PRINTING OUT EDGES" << std::endl;
  for (auto it : edge_cost) {
    std::cout << "pair = (" << it.first.first << ", " << it.first.second << ") = " << it.second << std::endl;
  }
}
void printDistanceVectors(std::map<std::string, std::map<std::pair<std::string,std::string>, int> > distance_vectors) {
  std::cout << "PRINTING OUT DISTANCE VECTORS" << std::endl;
  for (auto it : distance_vectors) {
    std::cout << "table for node " << it.first << std::endl;
    for (auto it1 : it.second) {
      std::cout << "[" << it1.first.first << "][" << it1.first.second << "] = " << it1.second << std::endl;
    }
  }
}
void printNextHops(std::map<std::string, std::map<std::string, std::string> >nextHops) {
  std::cout << "PRINTING OUT NEXT HOPS" << std::endl;
  for (auto it : nextHops) {
    std::cout << "src = Node " << it.first << std::endl;
    for (auto it1 : it.second) {
      std::cout << "dest = " << it1.first << ", next hop to reach dest = " << it1.second << std::endl;
    }
  }
}*/

void initDistanceVectors(size_t node) {

    std::map<std::pair<std::size_t,size_t>, int>& distance_vector = distance_vectors[node];
    std::map<size_t, size_t>& curr_next_hops = nextHops[node];

    std::set<size_t>::iterator it = nodes.begin();
    for (; it != nodes.end(); it++) {

      size_t neighbor = *it;
      std::pair<size_t, size_t> pair = std::make_pair(node, neighbor);

      if (node == neighbor) {
        distance_vector[pair] = 0;
        curr_next_hops[node] = node;
      }
      else {
        if (edge_cost.find(pair) != edge_cost.end()) {
          distance_vector[pair] = edge_cost[pair];
          curr_next_hops[neighbor] = neighbor;
        }
        else {
          distance_vector[pair] = INT_MAX;
          curr_next_hops[neighbor] = -1;
        }
      }

      std::set<size_t>::iterator it1 = nodes.begin();
      for (; it1 != nodes.end(); it1++) {
        size_t other_node = *it1;

        if (other_node != node) {
          std::map<std::pair<std::size_t,size_t>, int>& other_distance_vector = distance_vectors[other_node];
          other_distance_vector[pair] = distance_vector[pair];
          //distance_vectors[other_node] = other_distance_vector;
        }
      }
    }
    //distance_vectors[node] = distance_vector;
    //nextHops[node] = curr_next_hops;
}



void updateDistanceVector(size_t curr_node) {

    std::map<std::pair<std::size_t,size_t>, int>& curr_distance_vector = distance_vectors[curr_node];
    std::map<size_t, size_t>& curr_next_hops = nextHops[curr_node];


    for (int i = 0; i < nodes.size() - 1; i++) {

      std::map<std::pair<size_t, size_t>, int>::iterator it_edge = edge_cost.begin();
      for (; it_edge != edge_cost.end(); it_edge++) {

        std::pair<size_t, size_t> edge = it_edge->first;

        std::pair<size_t, size_t> v = std::make_pair(curr_node, edge.second);
        std::pair<size_t, size_t> u = std::make_pair(curr_node, edge.first);

        if (curr_distance_vector[v] > curr_distance_vector[u] + it_edge->second && curr_distance_vector[u] != INT_MAX) {
          //std::cout << "u = " << edge.first << ", curr_distance_vector[u] = " << curr_distance_vector[u] << std::endl;
          //std::cout << "v = " << edge.second << ", curr_distance_vector[v] = " << curr_distance_vector[v] << std::endl;
          //std::cout << "cost(u, v) = " << it_edge->second << std::endl;

          curr_distance_vector[v] = curr_distance_vector[u] + it_edge->second;

          std::set<size_t>::iterator it = nodes.begin();
          for (; it != nodes.end(); it++) {
            size_t other_node = *it;

            if (other_node != curr_node) {
              std::map<std::pair<std::size_t,size_t>, int>& other_distance_vector = distance_vectors[other_node];
              other_distance_vector[v] = curr_distance_vector[v];
            }
          }

          curr_next_hops[edge.second] = curr_next_hops[edge.first];
          //std::cout << "next hop for = " << edge.second << ", is next hop of = " << edge.first << ", which is = " << curr_next_hops[edge.first] << std::endl;
        }
      }
    }
}


int getShortestPath(size_t src, size_t dest, std::vector<size_t>& path) {

    std::pair<std::size_t,size_t> src_dest = std::make_pair(src, dest);
    std::map<std::pair<std::size_t,size_t>, int> curr_distance_vector = distance_vectors[src];
    int path_cost = curr_distance_vector[src_dest];

    std::map<size_t, size_t> curr_next_hops = nextHops[src];
    size_t curr = src;

    if (path_cost == INT_MAX) return path_cost;

    while (curr != dest) {
      path.push_back(curr);
      curr_next_hops = nextHops[curr];
      curr = curr_next_hops[dest];
    }

    return path_cost;
}


std::string getHops(std::vector<std::size_t> hops) {
  std::string hops_str = "";
  for (int i = 0; i < hops.size(); i++) {
    std::string hop = convertSize_tToString(hops[i]);
    hops_str += (hop + " ");
  }
  return hops_str;
}



void storeMessageOutput(int path_cost, std::vector<size_t> shortest_path,
  size_t src, size_t dest, std::string content, FILE* fpOut) {

    std::string line = "";
    std::string src_str = convertSize_tToString(src);
    std::string dest_str = convertSize_tToString(dest);
    std::string path_cost_str = convertIntToString(path_cost);

    if (path_cost == INT_MAX) {
      line = "from " + src_str + " to " + dest_str + " cost infinite hops unreachable message " + content;

      //std::cout << "original line = " << line;

      const char* line_cstr = line.c_str();

      //fprintf(stderr, "%s", line_cstr);

      fwrite(line_cstr, 1, line.size() - 1, fpOut);
      //fwrite("\n", 1, 1, fpOut);
    }
    else {
      line = "from " + src_str + " to " + dest_str + " cost " + path_cost_str + " hops " + getHops(shortest_path) + "message " + content;
      const char* line_cstr = line.c_str();

      //std::cout << "original line = " << line;
      //fprintf(stderr, "%s", line_cstr);
      //std::cout << "line = " << line << ", line size = " << line.size() << std::endl;
      fwrite(line_cstr, 1, line.size() - 1, fpOut);
      //fwrite("\n", 1, 1, fpOut);
    }

}



void storeForwardingTable(size_t node, FILE* fpOut) {

    std::map<size_t, size_t> curr_next_hops = nextHops[node];
    std::map<std::pair<std::size_t,size_t>, int> curr_distance_vector = distance_vectors[node];

    /*std::cout << "current node = " << node << std::endl;
    for (auto it : curr_next_hops) {
      std::cout << "it->first (destination) = " << it.first << std::endl;
      std::cout << "it->second (nexthop) = " << it.second << std::endl;
      std::cout << "" << std::endl;
    }*/

    std::map<size_t, size_t>::iterator it = curr_next_hops.begin();
    for (; it != curr_next_hops.end(); it++) {
      std::pair<std::size_t,size_t> edge = std::make_pair(node, it->first);

      if (curr_distance_vector[edge] != INT_MAX) {

        //std::cout << "node = " << node << std::endl;
        //std::cout << "dest = " << it->first << endl;

        std::string dest_str = convertSize_tToString(it->first);
        std::string nexthop = convertSize_tToString(it->second);
        std::string path_cost_str = convertIntToString(curr_distance_vector[edge]);

        std::string line = dest_str + " " + nexthop + " " + path_cost_str;

        const char* line_cstr = line.c_str();

        //std::cout << "line = " << line << ", line size = " << line.size() << std::endl;
        fwrite(line_cstr, 1, line.size(), fpOut);
        fwrite("\n", 1, 1, fpOut);
      }
    }

    //fwrite("\n", 1, 1, fpOut);
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
    }
}





int main(int argc, char** argv) {
    if (argc != 4) {
        printf("Usage: ./distvec topofile messagefile changesfile\n");
        return -1;
    }

    FILE *fpOut;
    fpOut = fopen("output_linkstate.txt", "w");


    char* topofile_title = argv[1];
    char* messagefile_title = argv[2];
    char* changesfile_title = argv[3];

    FILE* topofile_ptr = fopen(topofile_title, "r");
    if (topofile_ptr == NULL) {
      fprintf(stderr, "cannot open topofile");
    }

    char current_line[100];
    while (fgets(current_line, sizeof(current_line), topofile_ptr) != NULL) {

      char delim[] = " ";

      std::string src(strtok(current_line, delim));
      std::string dest(strtok(NULL, delim));
      std::string cost(strtok(NULL, delim));

      size_t src_num = convertStringtoSize_t(src);
      size_t dest_num = convertStringtoSize_t(dest);
      int cost_num = convertStringtoInt(cost);

      if (nodes.find(src_num) == nodes.end()) nodes.insert(src_num);
      if (nodes.find(dest_num) == nodes.end()) nodes.insert(dest_num);

      std::pair<size_t, size_t> src_dest_edge = std::make_pair(src_num, dest_num);
      std::pair<size_t, size_t> dest_src_edge = std::make_pair(dest_num, src_num);

      edge_cost[src_dest_edge] = cost_num;
      edge_cost[dest_src_edge] = cost_num;

    }

    std::set<size_t>::iterator it = nodes.begin();
    for (; it != nodes.end(); it++) {
      size_t node = *it;
      initDistanceVectors(node);
    }

    it = nodes.begin();
    for (; it != nodes.end(); it++) {
      size_t node = *it;
      updateDistanceVector(node);
    }

    it = nodes.begin();
    for (; it != nodes.end(); it++) {
      size_t node = *it;
      storeForwardingTable(node, fpOut);
    }
    fclose(topofile_ptr);








    // send all the messages
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
    //std::cout << changes_file_size << std::endl;

    if (changes_file_size != 0) {

      rewind(changesfile_ptr);
      char* res;
      while ((res = fgets(current_line2, sizeof(current_line2), changesfile_ptr)) != NULL) {

        if (res == NULL || current_line2 == NULL) break;

        char delim[] = " ";

        std::string src(strtok(current_line2, delim));
        std::string dest(strtok(NULL, delim));
        std::string cost(strtok(NULL, delim));

        size_t src_num = convertStringtoSize_t(src);
        size_t dest_num = convertStringtoSize_t(dest);
        int cost_num = convertStringtoInt(cost);

        changeTopology(src_num, dest_num, cost_num);

        it = nodes.begin();
        for (; it != nodes.end(); it++) {
          size_t node = *it;
          initDistanceVectors(node);
        }

        it = nodes.begin();
        for (; it != nodes.end(); it++) {
          size_t node = *it;
          updateDistanceVector(node);
        }

        it = nodes.begin();
        fwrite("\n", 1, 1, fpOut);
        for (; it != nodes.end(); it++) {
          size_t node = *it;
          storeForwardingTable(node, fpOut);
        }

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
