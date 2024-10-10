/*
#
# Copyright (C) 2024 Stealth Software Technologies, Inc.
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use,
# copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following
# conditions:
#
# The above copyright notice and this permission notice (including
# the next paragraph) shall be included in all copies or
# substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
#
# SPDX-License-Identifier: MIT
#
*/
/* RM Tool Libraries */
#include "rm_common.hpp"
#include "secretsharing.h"
#include "additive2basis.h"
#include "root_finding.h"

/* MPC Networking Libraries */
#include "network_common.hpp"
#include "network_message.hpp"
#include "network_ts_queue.hpp"
#include "network_connection.hpp"
#include "network_server.hpp"

/* RM Client/Server Libraries */
#include "rm_client.hpp"
#include "rm_server_stm.hpp"
#include "rm_server.hpp"

/* Standard Libraries */
#include <assert.h>
#include <fstream>
#include <ratio>
#include <utility>
#include <functional>

/* NTL Libraries */
#include <NTL/ZZ.h>
#include <NTL/ZZ_p.h>
#include <NTL/vector.h>
#include <NTL/vec_ZZ_p.h>

int main(int argc, char *argv[]){
  if(argc != 4){
    std::cout << "Configuration Files Required as Follows:" << std::endl;
    std::cout << "(1) mpc configuration" << std::endl;
    std::cout << "(2) mix configuration" << std::endl;
    std::cout << "(3) network configuration" << std::endl;
    std::cout << "Usage: ./rm_server configs/mix_config configs/mpc_config configs/net_config" << std::endl;
    return 1;
  }
  std::string filename1(argv[1]);
  std::string filename2(argv[2]);
  std::string filename3(argv[3]);
  std::ifstream fin1;
  std::ifstream fin2;
  std::ifstream fin3;
  fin1.open(filename1);
  fin2.open(filename2);
  fin3.open(filename3);
  if(!fin1.is_open() || !fin2.is_open() || !fin3.is_open() ){
    std::cout << "Reading configuration file failed" << std::endl;
    return 1;
  }

  std::string read_param;
  rm_info info;

  /* Prime Modulus Setup */
  fin1 >> read_param;
  if(!fft_prime_from_bit_length(info.fft_prime_info, std::stoi(read_param))){
    std::cerr << "Prime Length is Invalid" << std::endl;
    return 1;
  }
  NTL::ZZ_p::init(info.fft_prime_info.prime);

  /* MPC servers */
  fin1 >> read_param;
  info.n = (size_t) std::stoi(read_param); // the number of servers
  if(info.n%4 != 0){
    info.t = info.n / 4;
  }
  else{
    info.t = (info.n - 1) / 4;
  }
  fin1 >> read_param;
  info.server_id = (size_t) std::stoi(read_param);
  info.l = 1;

  /*
  std::cout << "Prime: "  << info.fft_prime_info.prime << std::endl;
  std::cout << "Number of Servers: "  << info.n << std::endl;
  std::cout << "Max Number of Corrupted Servers: "  << info.t << std::endl;
  std::cout << "My Server ID: "  << info.server_id << std::endl;
  std::cout << "Share Packing Size: "  << info.l << std::endl;
  */

  /* Mix Parameter Configuration */
  fin2 >> read_param;
  info.L = (size_t) std::stoi(read_param); // Mixing parameter L
  info.N = 14 * pow(info.L, 2) + 10 * info.L - 1; // the number of messages to be mixed 
  std::cout << "The number of messages in an epoch: "  << info.N << std::endl;

  /* Network Parameters */
  std::vector<std::string> IPs;
  std::vector<std::string> ports;
  std::string temp_IP;
  std::string temp_port;
  for(size_t i = 0 ; i != info.n ; i++){
    if (fin3.eof()){
      cerr << "Incorrect Network Configuration" << endl;
      return 1;
    }
    fin3 >> temp_IP;
    if (fin3.eof()){
      cerr << "Incorrect Network Configuration" << endl;
      return 1;
    }
    fin3 >> temp_port;
    IPs.push_back(temp_IP);
    ports.push_back(temp_port);
  }//

  for(size_t i = 0 ; i != info.n ; i++){
    std::cout << "Server[" << i+1 << "]'s IP/Port: " << IPs[i] + "/" + ports[i] << std::endl;
  }

  // Preprocess of g0 required for root-finding
  NTL::vec_ZZ_p xvals;
  NTL::ZZ_pX g0;
  xvals = gen_xvals(info.n);
  g0 = NTL::BuildFromRoots(xvals);

  // Concurrency parameter and variables
  unsigned int num_threads = 3;

  fin1.close();
  fin2.close();
  fin3.close();
  /* End of RM Server Setup */

  // Begin of RM Server Body 
  rm_server server(std::stoi(ports[info.server_id-1]));
  server.start();
  std::cout << "Listening in Port " << ports[info.server_id-1] << std::endl;

  // Each server will be a client to other servers
  rm_client clients[info.n]; 

  // connect to other server
  for(size_t i = 0 ; i != info.n ;){
    clients[i].local_partyID = info.server_id;
    clients[i].remote_partyID = i+1;
    if(i == info.server_id - 1)
    {
      i++;
      continue; // skip the client connection if I am the server
    }
    if (clients[i].connect(IPs[i], ports[i]))
    {
      std::cout << "Server[" << info.server_id << "]: " 
                << "Connection to Server[" << IPs[i] << " : " << ports[i] << "] established" 
                << std::endl;
      i++;
    }
    else{
      std::cout << "Server[" << info.server_id << "]: " 
                << "Connection to Server[" << IPs[i] << " : " << ports[i] << "] failed" 
                << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(2));
    }
  }
  std::cout << "All server connections established\n";

  // move these to a separate class for despute resolution
  std::shared_ptr<std::map<uint32_t,bool>> corrupted_clients(new std::map<uint32_t,bool>);
  std::shared_ptr<std::map<uint32_t,bool>> corrupted_servers(new std::map<uint32_t,bool>); 

  // initialize
  for(size_t i = 0 ; i != info.N ; i++){
    corrupted_clients->insert(pair<uint32_t,bool>(i,false));
  }
  for(size_t i = 0 ; i != info.n ; i++){
    corrupted_servers->insert(pair<uint32_t,bool>(i,false));
  }

  // Server STM map
  std::map<uint32_t,std::shared_ptr<rm_mixing_stm>> stms;
  rm_net::async_queue<rm_net::deserialized_message> deserialzed_msgs;

  // initialize
  for(size_t i = 0 ; i != info.N ; i++){
    corrupted_clients->insert(pair<uint32_t,bool>(i,false));
  }
  for(size_t i = 0 ; i != info.n ; i++){
    corrupted_servers->insert(pair<uint32_t,bool>(i,false));
  }

  /***********************************************/
  /********* Test Case Generation/Setup **********/
  /***********************************************/
  size_t prime_idx = 0;
  size_t L_idx = 0;
  size_t prime_max_idx = prime_length.size();
  size_t L_max_idx = L_value.size();
  std::vector<rm_info> test_cases;

  for(L_idx = 0 ; L_idx != L_max_idx ; L_idx++)
  {
    for(prime_idx = 0 ; prime_idx != prime_max_idx ; prime_idx++)
    {
      if(!fft_prime_from_bit_length(info.fft_prime_info, prime_length[prime_idx]))
      {
        std::cerr << "Prime Length is Invalid" << std::endl;
        return 1;
      }
      // Mix Parameter Configuration 
      info.L = L_value[L_idx]; // Mixing parameter L
      info.N = 14 * pow(info.L, 2) + 10 * info.L - 1; // the number of messages to be mixed
      test_cases.emplace_back(info);
      std::cout << "[Test Case]: Prime Info -> " << test_cases.back().fft_prime_info.two_exponent << ", "
                                   << test_cases.back().fft_prime_info.odd_factor << ", "
                                   << test_cases.back().fft_prime_info.zeta << ", "
                << "# of Msgs -> " << test_cases.back().N << std::endl;
    }
  }

  if(!fft_prime_from_bit_length(info.fft_prime_info, test_plen))
  {
    std::cerr << "Prime Length is Invalid" << std::endl;
    return 1;
  }

  for(size_t i = 0 ; i != L_value2.size() ; i++)
  {
    info.L = L_value2[i]; // Mixing parameter L
    info.N = 14 * pow(info.L, 2) + 10 * info.L - 1; // the number of messages to be mixed
    test_cases.emplace_back(info);
    std::cout << "[Test Case]: Prime Info -> " << test_cases.back().fft_prime_info.two_exponent << ", "
                                   << test_cases.back().fft_prime_info.odd_factor << ", "
                                   << test_cases.back().fft_prime_info.zeta << ", "
                << "# of Msgs -> " << test_cases.back().N << std::endl;
  }

  /*******************************/
  /********* Test Start **********/
  /*******************************/
  for(size_t case_idx = 0 ; case_idx != test_cases.size() ; case_idx++)
  {

  /*******************************/
  /********* Test Setup **********/
  /*******************************/
  info = test_cases[case_idx];

  NTL::ZZ_p::init(info.fft_prime_info.prime);

  // initialize
  for(size_t i = 0 ; i != info.N ; i++){
    corrupted_clients->insert(pair<uint32_t,bool>(i,false));
  }
  for(size_t i = 0 ; i != info.n ; i++){
    corrupted_servers->insert(pair<uint32_t,bool>(i,false));
  } 
  /*******************************/
  /*******************************/
  /*******************************/
  bool is_cont = true;

  while(is_cont)
  {
    server.update(info,deserialzed_msgs);
    while(deserialzed_msgs.count() != 0)
    {
      // get the first deserialized msg package
      auto dm = deserialzed_msgs.front();
      deserialzed_msgs.pop_front();

      // if stm with sid does not exists;
      if(stms.find(dm.sid) == stms.end()) // If a stm exists for sid
      {
        //create a new stm with sid
        std::shared_ptr<rm_mixing_stm> stm(new rm_mixing_stm(info,g0));
        stm->sid = dm.sid;
        stms.insert(std::pair<uint32_t,std::shared_ptr<rm_mixing_stm>> (dm.sid,stm));
      }
      // execute the stm with dm, clients
      stms.at(dm.sid)->message_handler(dm, info);
      stms.at(dm.sid)->execute_rm_stm(clients,
                                      info,
                                      corrupted_clients,
                                      corrupted_servers);
      if(stms.at(dm.sid)->get_state() == 15)
      {
        stms.erase(dm.sid); // remove the completed stm.
        //the followings are only for testing
        //std::cout << "Server terminated\n";
        is_cont = false; 
        break;
      }
    }
  }

  } // for-loop for test ends

  return 0;
}
