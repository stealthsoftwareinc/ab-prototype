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
#include <NTL/ZZ.h>
#include <NTL/ZZ_p.h>
#include <NTL/vector.h>
#include <NTL/vec_ZZ_p.h>
#include <cmath>
#include <assert.h>   

using namespace NTL;
using namespace std;

// Generate the additive 2-basis (exponents)
void add2basis_gen(Vec<ZZ>& basis, const size_t L)
{
   ZZ a;
   a = 0;
   for(size_t i = 0 ; i < L ; i++){
      a += 1;
      basis.append(a);       
   } // the arithmetic progression S_1
   a = L;
   for(size_t i = 0 ; i < 3*L ; i++){
      a += L;
      basis.append(a);       
   } // the arithmetic progression S_2
   a = 3*pow(L,2)+L-1;
   for(size_t i = 0 ; i < L ; i++){
      a += L+1;
      basis.append(a);       
   } // the arithmetic progression S_3
   a = 6*pow(L,2)+4*L-1;
   for(size_t i = 0 ; i < L+1 ; i++){
      a += 1;
      basis.append(a);       
   } // the arithmetic progression S_4
   a = 10*pow(L,2)+7*L-1;
   for(size_t i = 0 ; i < L+1 ; i++){
      a += 1;
      basis.append(a);       
   } // the arithmetic progression S_5
   basis.append((ZZ) L+1);
   basis.append((ZZ) 6*pow(L,2)+4*L-1);
   basis.append((ZZ) 10*pow(L,2)+7*L-1); // the set S_6
}

// Given a message msg and basis, generate an encoding of msg
void encode_input(Vec<ZZ_p>& code, const Vec<ZZ>& basis, const ZZ_p msg)
{
   for(size_t i = 0 ; i < basis.length() ; i++){
      code.append(power(msg,basis[i]));
   }
}

// Given a message msg and L, generate an encoding of msg
void add2basis_encode(Vec<ZZ_p>& code, const ZZ_p msg, const size_t L)
{
   ZZ a;
   a = 0;
   for(size_t i = 0 ; i < L ; i++){
      a += 1;
      code.append(power(msg,a));       
   } // the arithmetic progression S_1
   a = (ZZ) L;
   for(size_t i = 0 ; i < 3*L ; i++){
      a += (ZZ) L;
      code.append(power(msg,a));
   } // the arithmetic progression S_2
   a = (ZZ) 3*pow(L,2)+L-1;
   for(size_t i = 0 ; i < L ; i++){
      a += (ZZ) L+1;
      code.append(power(msg,a));
   } // the arithmetic progression S_3
   a = (ZZ) 6*pow(L,2)+4*L-1;
   for(size_t i = 0 ; i < L+1 ; i++){
      a += 1;
      code.append(power(msg,a));
   } // the arithmetic progression S_4
   a = (ZZ) 10*pow(L,2)+7*L-1;
   for(size_t i = 0 ; i < L+1 ; i++){
      a += 1;
      code.append(power(msg,a));
   } // the arithmetic progression S_5
   code.append(power(msg, (ZZ) L+1));
   code.append(power(msg, (ZZ) 6*pow(L,2)+4*L-1));
   code.append(power(msg, (ZZ) 10*pow(L,2)+7*L-1)); // the set S_6
}

// Input Decompression Circuit over shares
vec_ZZ_p decompress_encoding(const Vec<ZZ_p>& input, const size_t L)
{
   vec_ZZ_p decompressed;
   size_t N;
   N = 14*pow(L,2)+10*L-1; // the number of clients
   for (size_t i = 0 ; i < L ; i++){  // 1 ~ L (t-share)
      decompressed.append(input[i]);
   }
   for (size_t i = 0 ; i < L ; i++){  // L+1 ~ 2L (2t-share)
      decompressed.append(input[L-1]*input[i]);
   }
   for (size_t j = L ; j < 4*L ; j++){  // 2L+1 ~ 3L^2+2L 
      for (size_t i = 0 ; i < L ; i++){
         decompressed.append(input[i]*input[j]);
      }
   }
   for (size_t j = 4*L ; j < 5*L ; j++){ // 3L^2+2L+1 ~ 4L^2+3L-1
      if (j > 4*L){
         decompressed.append(input[j]);
      }
      for (size_t i = 0 ; i < L ; i++){
         decompressed.append(input[i]*input[j]);
      }
   }
   for (size_t i = 1 ; i < 2*L+2 ; i++ ){  // 4L^2+3L ~ 6L^2+4L-1
      for (size_t j = 1 ; j < L+1 ; j++){
         decompressed.append(input[(2*L-1)+i-j]*input[4*L+j-1]);
      }
   }
   for (size_t i = 5*L ; i < 6*L+1 ; i++){  // 6L^2+4L ~ 6L^2+5L
      decompressed.append(input[i]);
   }
   for (size_t i = 0 ; i < L ; i++){  // 6L^2+5L+1 ~ 6L^2+6L
      decompressed.append(input[i]*input[6*L]);
   }
   for (size_t i = L ; i < 4*L ; i++){  // 6L^2+6L+1 ~ 9L^2+6L
      for (size_t j = 5*L+1 ; j < 6*L+1 ; j++){
         decompressed.append(input[i]*input[j]);
      }
   }
   for (size_t j = 5*L+1 ; j < 6*L+1 ; j++){  // 9L^2+6L+1 ~ 9L^2+7L
      decompressed.append(input[4*L]*input[j]);
   }
   for (size_t i = 4*L+1 ; i < 5*L ; i++){  // 9L^2+7L+1 ~ 10L^2+7L-1
      for (size_t j = 5*L ; j < 6*L+1 ; j++){
         decompressed.append(input[i]*input[j]);
      }
   }
   for (size_t i = 6*L+1 ; i < 7*L+2 ; i++){  // 10L^2+7L ~ 10L^2+8L
      decompressed.append(input[i]);
   }
   for (size_t i = 0 ; i < L ; i++){  // 10L^2+8L+1 ~ 10L^2+9L
      decompressed.append(input[i]*input[7*L+1]);
   }
   for (size_t i = L ; i < 4*L ; i++){  // 10L^2+9L+1 ~ 13L^2+9L
      for (size_t j = 6*L+2 ; j < 7*L+2 ; j++){
         decompressed.append(input[i]*input[j]);
      }
   }
   for (size_t i = 6*L+2 ; i < 7*L+2 ; i++){  // 13L^2+9L+1 ~ 13L^2+10L
      decompressed.append(input[4*L]*input[i]);
   }
   for (size_t i = 4*L+1 ; i < 5*L ; i++){  // 13L^2+10L+1 ~ 14L^2+10L-1
      for (size_t j = 6*L+1 ; j < 7*L+2 ; j++){
         decompressed.append(input[i]*input[j]);
      }
   }
   return decompressed;
}

// Input Decompression Circuit over shares without APPEND
vec_ZZ_p opt_decompress_encoding(const Vec<ZZ_p>& input, const size_t L)
{
   vec_ZZ_p decompressed;
   size_t N, pos;
   N = 14*pow(L,2)+10*L-1; // the number of clients
   decompressed.SetLength(N);
   pos = 0;
   for (size_t i = 0 ; i < L ; i++){  // 1 ~ L (t-share)
      decompressed[pos] = input[i];
      pos++;
   }
   for (size_t i = 0 ; i < L ; i++){  // L+1 ~ 2L (2t-share)
      decompressed[pos] = input[L-1]*input[i];
      pos++;
   }
   for (size_t j = L ; j < 4*L ; j++){  // 2L+1 ~ 3L^2+2L 
      for (size_t i = 0 ; i < L ; i++){
         decompressed[pos] = input[i]*input[j];
         pos++;
      }
   }
   for (size_t j = 4*L ; j < 5*L ; j++){ // 3L^2+2L+1 ~ 4L^2+3L-1
      if (j > 4*L){
         decompressed[pos] = input[j];
         pos++;
      }
      for (size_t i = 0 ; i < L ; i++){
         decompressed[pos] = input[i]*input[j];
         pos++;
      }
   }
   for (size_t i = 1 ; i < 2*L+2 ; i++ ){  // 4L^2+3L ~ 6L^2+4L-1
      for (size_t j = 1 ; j < L+1 ; j++){
         decompressed[pos] = input[(2*L-1)+i-j]*input[4*L+j-1];
         pos++;
      }
   }
   for (size_t i = 5*L ; i < 6*L+1 ; i++){  // 6L^2+4L ~ 6L^2+5L
      decompressed[pos] = input[i];
      pos++;
   }
   for (size_t i = 0 ; i < L ; i++){  // 6L^2+5L+1 ~ 6L^2+6L
      decompressed[pos] = input[i]*input[6*L];
      pos++;
   }
   for (size_t i = L ; i < 4*L ; i++){  // 6L^2+6L+1 ~ 9L^2+6L
      for (size_t j = 5*L+1 ; j < 6*L+1 ; j++){
         decompressed[pos] = input[i]*input[j];
         pos++;
      }
   }
   for (size_t j = 5*L+1 ; j < 6*L+1 ; j++){  // 9L^2+6L+1 ~ 9L^2+7L
      decompressed[pos] = input[4*L]*input[j];
      pos++;
   }
   for (size_t i = 4*L+1 ; i < 5*L ; i++){  // 9L^2+7L+1 ~ 10L^2+7L-1
      for (size_t j = 5*L ; j < 6*L+1 ; j++){
         decompressed[pos] = input[i]*input[j];
         pos++;
      }
   }
   for (size_t i = 6*L+1 ; i < 7*L+2 ; i++){  // 10L^2+7L ~ 10L^2+8L
      decompressed[pos] = input[i];
      pos++;
   }
   for (size_t i = 0 ; i < L ; i++){  // 10L^2+8L+1 ~ 10L^2+9L
      decompressed[pos] = input[i]*input[7*L+1];
      pos++;
   }
   for (size_t i = L ; i < 4*L ; i++){  // 10L^2+9L+1 ~ 13L^2+9L
      for (size_t j = 6*L+2 ; j < 7*L+2 ; j++){
         decompressed[pos] = input[i]*input[j];
         pos++;
      }
   }
   for (size_t i = 6*L+2 ; i < 7*L+2 ; i++){  // 13L^2+9L+1 ~ 13L^2+10L
      decompressed[pos] = input[4*L]*input[i];
      pos++;
   }
   for (size_t i = 4*L+1 ; i < 5*L ; i++){  // 13L^2+10L+1 ~ 14L^2+10L-1
      for (size_t j = 6*L+1 ; j < 7*L+2 ; j++){
         decompressed[pos] = input[i]*input[j];
         pos++;
      }
   }
   return decompressed;
}

// Input Format Verification Circuit
ZZ_p verify_format(const Vec<ZZ_p>& coins, const Vec<ZZ_p>& input, const size_t L)
{
   assert(input.length() == 7*L+5);
   assert(coins.length() == 7*L+4);
   ZZ_p pred;
   pred = 0;
   size_t idx = 0;
   for (size_t i = 0 ; i < L-1 ; i++){
      pred += coins[idx]*(input[i+1] - input[i]*input[0]);
      idx++;
   }
   for (size_t i = L ; i < 4*L-1 ; i++){
      pred += coins[idx]*(input[i+1] - input[i]*input[L-1]);
      idx++;
   }
   for (size_t i = 4*L ; i < 5*L-1 ; i++){
      pred += coins[idx]*(input[i+1] - input[i]*input[7*L+2]);
      idx++;
   }
   pred += coins[idx]*(input[7*L+2] - input[0]*input[L-1]);
   idx++;
   for (size_t i = 5*L ; i < 6*L ; i++){
      pred += coins[idx]*(input[i+1] - input[0]*input[i]);
      idx++;
   }
   for (size_t i = 6*L+1 ; i < 7*L+1 ; i++){
      pred += coins[idx]*(input[i+1] - input[0]*input[i]);
      idx++;
   }
   pred += coins[idx]*(input[L] - input[L-1]*input[L-1]);
   idx++;
   pred += coins[idx]*(input[4*L] - input[4*L-1]*input[L-1]);
   idx++;
   pred += coins[idx]*(input[5*L] - input[7*L+3]*input[0]);
   idx++;
   pred += coins[idx]*(input[7*L+3] - input[3*L]*input[5*L-1]);
   idx++;
   pred += coins[idx]*(input[6*L+1] - input[7*L+4]*input[0]);
   idx++;
   pred += coins[idx]*(input[7*L+4] - input[5*L-1]*input[6*L]);
   return pred;
}