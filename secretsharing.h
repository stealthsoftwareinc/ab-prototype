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
#include <NTL/tools.h>
#include <NTL/mat_ZZ_p.h>
#include <NTL/ZZ_pX.h>
#include <assert.h>

using namespace std;
using namespace NTL;

/* Generate an N X (d+1) Vandermonde Matrix */
mat_ZZ_p vandermonde_gen(size_t N, size_t d){
  Mat<ZZ_p> vdm;
  vdm.SetDims(N,d+1); // set the matrix dimension N X d+1 
  for (size_t i = 0 ; i != N ; i++){
    for (size_t j = 0 ; j != d+1 ; j++){
      vdm.put(i,j,power(conv<ZZ_p>(i+1),conv<ZZ>(j)));
    }
  }
  return vdm;
}

// generate a vector of ZZ_p values from 1 to n
vec_ZZ_p gen_xvals(const size_t n){
  vec_ZZ_p xvals;
  for(size_t i = 0 ; i != n ; i++){
    xvals.append(conv<ZZ_p>(i+1));
  }
  return xvals;
}

/* Secret Sharing using Vandermond matrix */
vec_ZZ_p vdm_share_secret(const mat_ZZ_p& vdm, const ZZ_p m){
  mat_ZZ_p coef; 
  coef.SetDims(vdm.NumCols(),1);
  coef.put(0,0,m);
  for(size_t i = 1 ; i != vdm.NumCols() ; i++){
    coef.put(i,0,random_ZZ_p());
  }
  mat_ZZ_p temp;
  temp = transpose(vdm * coef);
  return temp[0]; // output f(1), ..., f(N) in the form of vector of length N
}

/* Sharmir Secret Sharing using ZZ_pX.h routines */
vec_ZZ_p packed_share_secret(const vec_ZZ_p xvals, const vec_ZZ_p msgs, const size_t t){
  assert(t+msgs.length()-1 > 0); // check the block size is correct
  size_t d = t+msgs.length()-1; // compute the degree
  assert(2*d < xvals.length()); // check the degree is correct w.r.t. n
  ZZ_pX p;
  p.SetMaxLength(d+1); // polynomical p's degree is "d+1"
  for(size_t i = 0 ; i != msgs.length() ; i++){
    SetCoeff(p,i,msgs[i]);
  }
  for(size_t i = msgs.length() ; i != d+1 ; i++){
    SetCoeff(p,i,random_ZZ_p());
  }
  vec_ZZ_p yval;
  eval(yval,p,xvals); // presumably eval is done via FFT
  return yval; // output f(1), ..., f(N) in the form of an N*1 matrix
}

// Gao's version of Berlekemp-Welsh RS decoding 
// The output 'error' will store the x values where errors occur on
bool rs_decode(
  vec_ZZ_p& secrets, 
  vec_ZZ_p& errors, 
  const vec_ZZ_p xvals, 
  const vec_ZZ_p shares, 
  const ZZ_pX g0, 
  const size_t d, 
  const size_t ell)
  {
  assert(secrets.length() == 0);
  assert(errors.length() == 0);
  ZZ_pX g1, a, b, t0, t1, t2, s0, s1, s2, q, r, v, g;
  size_t n;
  double target_deg;
  n = xvals.length();
  target_deg = (n + d + 1)*0.5;

  // Step 0: 0 polynomial of degree d case
  size_t max_errors = (n-d)*0.5;
  size_t nz_cnt = 0;
  for (size_t i = 0 ; i != n ; i++){
    if(shares[i] != 0){
      nz_cnt++;
      errors.append(xvals[i]);
    }
  }
  if (nz_cnt <= max_errors){
    for(size_t i = 0 ; i != ell ; i++){
      secrets.append(conv<ZZ_p>(0));
    }
    return true;
  }
  else{
    errors.kill();
  }

  // Step 1: Interpolate
  NTL::interpolate(g1,xvals,shares); // degree(g1) <= 2*deg+t. This must be over a prime field
  //cout << "Interpolated: = " << g1 << endl;
  
  // Step 2: Partial GCD
  t1 = t1 + 1; // initialized to t_1 = 1
  a = g0;
  b = g1;
  if (NTL::deg(b) < target_deg){
    g = g1;
    v = t1;
  }
  else{
    while(true){
      NTL::DivRem(q, r, a, b);
      t2 = t0 - q * t1;
      if (deg(r) < target_deg){
        g = r;
        v = t2;
        break;
      }
      a = b;
      b = r;
      t0 = t1;
      t1 = t2;
    }
  } 

  // Step 3: Long Division
  NTL::DivRem(q,r,g,v);
  if(NTL::IsZero(r) && NTL::deg(q) < d+1){
    //cout << "Reconstructed Poly: = " << q << endl;
    for(size_t i = 0 ; i < ell ; i++){
      secrets.append(coeff(q,i));
    }
    vec_ZZ_p yvals;
    yvals = eval(q,xvals);
    for(size_t i = 0 ; i < yvals.length() ; i++){
      if(yvals[i] != shares[i]){
        errors.append(xvals[i]);
      }
    }
    return true;
  }
  return false;
}