using namespace std;
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <stdlib.h>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <math.h>
#include <map>

int convertStringtoInt(string str) {
  stringstream ss(str);
  int result;
  ss >> result;
  return result;
}




void printVector(vector<int> vec) {
  std::cout << "vector contents = ";
  for (int i = 0; i < vec.size(); i++) {
    cout << vec[i] << " ";
  }
  cout << "" << endl;
}






vector<int> checkReadyNodes(vector<int> backoffs) {
  vector<int> readyNodes;
  for (int i = 0; i < backoffs.size(); i++) {
    if (backoffs[i] == 0) {
      readyNodes.push_back(i);
    }
  }
  return readyNodes;
}





void decrementBackoffs(vector<int>& backoffs) {
  for (int i = 0; i < backoffs.size(); i++) {
    backoffs[i]--;
  }
}






double calcVariance(vector<int> vec) {
  double mean = 0;
  for (int i = 0; i < vec.size(); i++) {
    mean += vec[i];
  }
  mean = mean / vec.size();

  double var = 0;
  for (int i = 0; i < vec.size(); i++) {
    var += pow(vec[i] - mean, 2);
  }
  return var / vec.size();
}






int main() {

  // 1. read in the input file and parse it for info
  FILE* fp = fopen("input.txt", "r");
  if (fp == NULL) {
    cout << "cannot find input.txt" << endl;
    return 0;
  }

  int n = 0;
  int l = 0;
  vector<int> r;
  int m = 0;
  int t = 0;

  char current_line[100];
  while (fgets(current_line, sizeof(current_line), fp) != NULL) {

    stringstream ss(current_line);

    string s;
    bool set = false;
    string line_type = "";
    while (getline(ss, s, ' ')) {
        if (s == "N" || s == "R" || s == "L" || s == "M" || s == "T") {
          set = true;
          line_type = s;
        }
        else if (set) {
          if (line_type == "N") n = convertStringtoInt(s);
          if (line_type == "R") r.push_back(convertStringtoInt(s));
          if (line_type == "L") l = convertStringtoInt(s);
          if (line_type == "M") m = convertStringtoInt(s);
          if (line_type == "T") t = convertStringtoInt(s);
        }
    }
  }

  /*cout << "N = " << n << endl;
  cout << "L = " << l << endl;
  printVector(r);
  cout << "M = " << m << endl;
  cout << "T = " << t << endl;*/

  ofstream utilizedFile("utilized.txt");
  ofstream idleFile("idle.txt");
  ofstream collisionFile("collisions.txt");

  ofstream L_20("L=20.txt");
  ofstream L_40("L=40.txt");
  ofstream L_60("L=60.txt");
  ofstream L_80("L=80.txt");
  ofstream L_100("L=100.txt");

  ofstream R_1("R=1.txt");
  ofstream R_2("R=2.txt");
  ofstream R_4("R=4.txt");
  ofstream R_8("R=8.txt");
  ofstream R_16("R=16.txt");

  // variables for output.txt
//for (l = 20; l <= 100; l += 20) {
//for (int init_r = 1; init_r <= 16; init_r *= 2) {
  //for (n = 5; n <= 500; n++) {

    map<int, double> utilized_data;
    map<int, double> idle_data;
    map<int, double> collision_data;

      int utilized = 0;
      int idle = 0;
      int total_collisions = 0;

      vector<int> good_transmissions(n,0);
      vector<int> total_transmissions(n, 0);
      vector<int> collision_counts(n, 0);
      srand(time(0));
      vector<int> collisions_per_node(n,0);
      vector<int> nodes_r_idx(n, 0);

      vector<int> backoffs(n);
      for (int i = 0; i < backoffs.size(); i++) {
        int curr_r = r[nodes_r_idx[i]];

        //int curr_r = init_r;
        //nodes_r_idx[i] = init_r;
        backoffs[i] = rand() % (curr_r + 1);
      }

      bool channelBusy = false;
      int pkt_bits_sent = 0;
      int node_sending = -1;

      for (int i = 0; i < t; i++) {

        if (pkt_bits_sent == l) { // finished transmitting --> successful transmission:
          good_transmissions[node_sending]++;

          pkt_bits_sent = 0;
          channelBusy = false;

          collision_counts[node_sending] = 0;

          nodes_r_idx[node_sending] = 0;
          int curr_r = r[0];
          //nodes_r_idx[node_sending] = init_r;
          //int curr_r = init_r;

          int backoff = rand() % (curr_r + 1);
          backoffs[node_sending] = backoff;

          node_sending = -1;
        }

        if (channelBusy) { // BUSY CHANNEL
          pkt_bits_sent++;
          utilized++;
        }

        if (!channelBusy) { // IDLE CHANNEL

          vector<int> nodeReady = checkReadyNodes(backoffs);

          if (nodeReady.size() > 1) { // collision
            total_collisions++;

            for (int i = 0; i < nodeReady.size(); i++) {
              int node = nodeReady[i];

              collisions_per_node[node]++;
              collision_counts[node]++;


              nodes_r_idx[node] = (nodes_r_idx[node] + 1) % m;
              //nodes_r_idx[node] *= 2;


              if (collision_counts[node] >= m) {
                nodes_r_idx[node] = 0;
                //nodes_r_idx[node] = init_r;
                collision_counts[node] = 0;
              }

              int curr_r = r[nodes_r_idx[node]];
              //int curr_r = nodes_r_idx[node];
              //cout << "r index = " << nodes_r_idx[node] << endl;
              //cout << "curr_r = " << curr_r << endl;

              int random_num = rand() % (curr_r + 1);
              backoffs[node] = random_num;
            }

          }

          if (nodeReady.size() == 0) { // no one ready to transmit, decrement everyone's backoff
            decrementBackoffs(backoffs);
            idle++;
          }

          if (nodeReady.size() == 1) {
            channelBusy = true;
            node_sending = nodeReady[0];
            total_transmissions[node_sending]++;
            utilized++;
          }
        }
      } // end of t iterations
      double utilized_percent = ((double)utilized / (double)t) * 100.0 ;
      double idle_percent = ((double)idle / (double)t) * 100.0;

      /*utilized_data[n] += utilized_percent;
      idle_data[n] += idle_percent;
      collision_data[n] += total_collisions;


      double util_avg = utilized_data[n];
      double idle_avg = idle_data[n];
      double collision_avg = collision_data[n];

      utilizedFile << util_avg;
      utilizedFile << "\n";
      idleFile << idle_avg;
      idleFile << "\n";
      collisionFile << collision_avg;
      collisionFile << "\n";*/

      /*if (l == 20) {
        L_20 << util_avg;
        L_20 << "\n";
      }

      if (l == 40) {
        L_40 << util_avg;
        L_40 << "\n";
      }

      if (l == 60) {
        L_60 << util_avg;
        L_60 << "\n";
      }

      if (l == 80) {
        L_80 << util_avg;
        L_80 << "\n";
      }

      if (l == 100) {
        L_100 << util_avg;
        L_100 << "\n";
      }*/

      /*if (init_r == 1) {
        R_1 << util_avg;
        R_1 << "\n";
      }

      if (init_r == 2) {
        R_2 << util_avg;
        R_2 << "\n";
      }

      if (init_r == 4) {
        R_4 << util_avg;
        R_4 << "\n";
      }

      if (init_r == 8) {
        R_8 << util_avg;
        R_8 << "\n";
      }

      if (init_r == 16) {
        R_16 << util_avg;
        R_16 << "\n";
      }*/

  //} // end of traversing all the n values

//} // end of travering all the l values



  cout << "utilized = " << utilized << endl;
  cout << "idle = " << idle << endl;
  cout << "number of collisions = " << total_collisions << endl;


  double successful_variance = calcVariance(good_transmissions);
  double collision_variance = calcVariance(collisions_per_node);

  // Create and open a text file
  ofstream MyFile("output.txt");

  // Write to the file
  MyFile << utilized_percent;
  MyFile << "\n";

  MyFile << idle_percent;
  MyFile << "\n";

  MyFile << total_collisions;
  MyFile << "\n";

  MyFile << successful_variance;
  MyFile << "\n";

  MyFile << collision_variance;
  MyFile << "\n";


  // Close the file
  MyFile.close();



}
