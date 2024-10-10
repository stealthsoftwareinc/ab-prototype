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
#include <iostream>
#include <utility> 
#include <assert.h>   
#include <NTL/ZZ.h>
#include <vector>

using namespace std;

//std::vector<int> prime_length({256,384});
//std::vector<int> L_value({5,9});

std::vector<int> prime_length({256,384,512,640,768,896,1024,1152,1280,1408,1536,1664});
std::vector<int> L_value({5,9,12,14,16});

int test_plen = 256; // 32 bytes
//std::vector<int> L_value2({6});

std::vector<int> L_value2({6,10,13,16,18,20,22,23,25,26,27});

// L = 1->23, 2->75, 3->155, 4->263, 5->399, 6->563, 7->755, 8->975, 9->1223, 10->1499, 
//     11->1803, 12->2135, 13->2495, 14->2883, 15->3299, 16->3743, 17->4215, 18-> 4715, 
//     19-> 5243, 20->5799, 21-> 6383, 22->6995, 23-> 7635, 24->8303, 25->8999, 26->9723,
//     27->10475

struct prime_info {
  NTL::ZZ prime;  // first
  int zeta;       // second
  int odd_factor; // odd factor
  int two_exponent; // two exponent
};

struct rm_info {
  prime_info fft_prime_info; // a pair of prime and zeta
  size_t n; // the number of servers 
  size_t t; // the max number of corrupted servers
  size_t server_id; // server ID > 0. If =0, it is a client.
  size_t l; // share-packing block size
  size_t N; // the number of clients (or messages to be mixed at an epoch)
  size_t L; // User input L
};

// returns the number of true values 
size_t number_of_truths(const std::vector<bool>& vec)
{
  size_t cnt = 0;
  size_t len = vec.size();
  for(size_t i = 0 ; i != len ; i++)
  {
    if(vec[i])
    {
      cnt++;
    }
  }
  return cnt;
}

// returns true if a boolean vector contains only true values
bool is_all_true(std::vector<bool> rec_status_vec)
{
  size_t len = rec_status_vec.size();
  bool res = true;
  for(size_t i = 0 ; i != len ; i++)
  {
    res = res & rec_status_vec[i];
  }
  return res;
}

bool fft_prime_from_bit_length(prime_info & fft_prime_info, const int & prime_selection){
  int zeta;
  int odd_factor;
  int two_exponent;
  NTL::ZZ prime;
  if (prime_selection == 32){
    //32 bits 
    zeta = 3;
    odd_factor = 101;
    two_exponent = 27;
  }
  else if (prime_selection == 40){
    //40 bits 
    zeta = 3;
    odd_factor = 125;
    two_exponent = 35;
  }
  else if (prime_selection == 64){
    //64 bits 
    zeta = 7;
    odd_factor = 129;
    two_exponent = 59;
  }
  else if (prime_selection == 128){
    //128 bits
    zeta = 3;
    odd_factor = 101;
    two_exponent = 123;
  }
  else if (prime_selection == 256){
    //256 bits = 32 bytes
    zeta = 7;
    odd_factor = 507;
    two_exponent = 251;
  }
  else if (prime_selection == 384){
    //384 bits = 48 bytes
    zeta = 10;
    odd_factor = 159;
    two_exponent = 379;
  }
  else if (prime_selection == 512){
    // 512 bits = 64 bytes
    zeta = 10;
    odd_factor = 267;
    two_exponent = 508;  
  }
  else if (prime_selection == 640){
    //640 bits = 80 bytes
    zeta = 3;
    odd_factor = 275;
    two_exponent = 635;
  }
  else if (prime_selection == 768){
    //768 bits = 96 bytes
    zeta = 3;
    odd_factor = 635;
    two_exponent = 763;
  }
  else if (prime_selection == 896){
    //896 bits = 112 bytes
    zeta = 3;
    odd_factor = 223;
    two_exponent = 892;
  }
  else if (prime_selection == 1024){
    // 1024 bits = 128 bytes
    zeta = 3;  
    odd_factor = 755;
    two_exponent = 1019;
  }
  else if (prime_selection == 1152){
    //1152 bits = 144 bytes
    zeta = 3;
    odd_factor = 149;
    two_exponent = 1147;
  }
  else if (prime_selection == 1280){
    //1280 bits = 160 bytes
    zeta = 5;
    odd_factor = 339;
    two_exponent = 1275;
  }
  else if (prime_selection == 1408){
    //1408 bits = 176 bytes
    zeta = 3;
    odd_factor = 539;
    two_exponent = 1403;
  }
  else if (prime_selection == 1536){
    //1536 bits = 192 bytes
    zeta = 7;
    odd_factor = 471;
    two_exponent = 1531;
  }
  else if (prime_selection == 1664){
    //1664 bits = 208 bytes
    zeta = 3;
    odd_factor = 865;
    two_exponent = 1662;
  }
  else if (prime_selection == 1792){
    //1792 bits = 224 bytes
    zeta = 23;
    odd_factor = 321;
    two_exponent = 1787;
  }
  else if (prime_selection == 1920){
    //1920 bits = 240 bytes
    zeta = 5;
    odd_factor = 203;
    two_exponent = 1917;
  }
  else if (prime_selection == 2048){
    //2048 bits = 256 bytes
    zeta = 3;
    odd_factor = 203;
    two_exponent = 2045;
  }
  else if (prime_selection == 3072){
    //3072 bits = 384 bytes
    zeta = 7;
    odd_factor = 675;
    two_exponent = 3068;
  }
  else if (prime_selection == 4096){
    //4096 bits = 512 bytes
    zeta = 3;
    odd_factor = 251;
    two_exponent = 4097;  
  }
  else{
    cout << "Wrong prime selection" << endl;
    return false;
  }

  prime = odd_factor;
  prime <<= two_exponent;
  prime += 1;
  fft_prime_info.prime = prime;
  fft_prime_info.zeta = zeta;
  fft_prime_info.two_exponent = two_exponent;
  fft_prime_info.odd_factor = odd_factor;
  return true;
}

