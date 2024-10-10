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
#include <NTL/ZZ_pX.h>
#include <NTL/ZZ_pXFactoring.h>
#include <NTL/vec_ZZ_p.h>
#include <bits/stdc++.h>

using namespace std;
using namespace NTL;
using std::pair;

pair<ZZ_pX, ZZ_pX> initial_linear_expansion(ZZ_pX const & f,
                                            ZZ_p const & neg_tau) {

  ZZ_pX h{};
  SetCoeff(h, 0L, eval(f, neg_tau));

  ZZ_pX hbar{};

  ZZ_p factorial{1L};
  auto poly_derived{diff(f)};
  for (long i = 1L; i <= deg(f); ++i) {
    // factorial = (i-1)!
    auto const taylor_eval{eval(poly_derived, neg_tau)};

    // hbar[i - 1] = f^{(i - 1)}(-\tau) / (i - 1)!
    SetCoeff(hbar, i - 1L, taylor_eval / factorial);

    factorial *= ZZ_p(i); // now factorial = i!

    // h[i] = f^{(i)}(-\tau) / i!
    SetCoeff(h, i, taylor_eval / factorial);

    // take next derivative
    poly_derived = diff(poly_derived);
  }
  return {h, hbar};
}

void update_linear_expansion(ZZ_pX & h, ZZ_pX & hbar) {

  //  hbarneg := hbar(-x)
  ZZ_pX hbarneg{};
  for (long i = 0L; i <= deg(hbar); ++i) {
    SetCoeff(hbarneg,
             i,
             ((i % 2L) == 0) ? coeff(hbar, i) : -coeff(hbar, i));
  }

  // // Uncomment for timings when running graeffe.test.cpp
  // auto start = std::chrono::high_resolution_clock::now();

  ZZ_pX a_even;
  ZZ_pX a_odd;
  for (long i = 0L; i <= deg(h); ++i) {
    SetCoeff((i % 2L == 0L) ? a_even : a_odd, i, coeff(h, i));
  }

  FFTSqr(a_even, a_even);
  FFTSqr(a_odd, a_odd);
  auto a = a_even - a_odd;

  // // Uncomment for timings when running graeffe.test.cpp
  // auto stop = std::chrono::high_resolution_clock::now();
  // auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
  // std::cout << "a: " << duration.count() << " microseconds" << std::endl;
  // start = stop;

  ZZ_pX b;
  FFTMul(b, h, hbarneg);

  // b(x) := b(x) + b(-x). So double the even coefficients. The
  // odd coefficients should be set to zero, but since only the even
  // coefficients are used below to update hbar we don't bother to
  // reset the odd coefficients here.
  mul(b, b, 2L);

  // // Uncomment for timings when running graeffe.test.cpp
  // stop = std::chrono::high_resolution_clock::now();
  // duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
  // std::cout << "b: " << duration.count() << " microseconds" << std::endl;

  // Update h and hbar with the even coefficients from a and b,
  // respectively.
  for (long i = 0L; i <= deg(h); ++i) {
    SetCoeff(h, i, coeff(a, 2L * i));
  }
  for (long i = 0L; i <= deg(hbar); ++i) {
    SetCoeff(hbar, i, coeff(b, 2L * i));
  }
}

pair<ZZ_pX, ZZ_pX>
tangent_graeffe_transform(ZZ_pX const & f, ZZ & rho, ZZ_p const & tau) {

  auto hs{initial_linear_expansion(f, -tau)};
  auto & h{hs.first};
  auto & hbar{hs.second};
  while ((rho > 1L) == 1L) {
    update_linear_expansion(h, hbar);
    rho >>= 1L;
  }
  return {h, hbar};
}

ZZ_pX batch_eval(ZZ_pX const & f,
                 ZZ_pX const & powers_of_w,
                 ZZ_pX const & powers_of_w_inv) {
  long const chi =   deg(powers_of_w) + 1;
  ZZ_pX tmp{};
  for (long i = 0; i <= deg(powers_of_w); i++) {
    SetCoeff(tmp, i, coeff(f, i) * coeff(powers_of_w, i));
  }

  mul(tmp, tmp, powers_of_w_inv); //degree of tmp <= 2*chi

  ZZ_pX ret{};
  ret.SetLength(chi);
  for (long i = 0; i < chi; i++) {
    SetCoeff(ret,
             i,
             (coeff(tmp, i) + coeff(tmp, i + chi))
                 * coeff(powers_of_w, i));
  }

  return ret;
}

vec_ZZ_p find_roots(ZZ_pX const & f,
                    int const zeta,
                    int const two_exponent,
                    int const odd_factor) {

  vec_ZZ_p Z;
  auto const degree = deg(f);

  long ell = 1L;
  ZZ chi_zz{odd_factor};
  chi_zz <<= two_exponent - 2L - ell;
  if (two_exponent <= 3L || (degree >= chi_zz) == 1L) {
    FindRoots(Z, f);
  } else {
    // The variable chi appears as both a long and as a ZZ (chi_zz),
    // so we don't overflow in the for loop below. Once this for loop
    // is completed we won't use chi_zz anymore.
    long chi{};
    ZZ rho{};
    ZZ_p rho_zz_p{};

    // Find the largest 1 <= ell <= two_exponent - 2 such that
    // degree < odd_factor*2^{two_exponent - 2 - ell} (= chi_zz). If
    // degree < odd_factor then set ell := two_exponent - 2.
    while ((degree < chi_zz) == 1L && ell < two_exponent - 2L) {
      chi_zz >>= 1L;
      ell++;
    }

    // The variable rho appears as both a ZZ and a ZZ_p (as
    // rho_zz_p) so we can use the first one to define the
    // primitive root of unity z and the second one when
    // evaluating potential roots in the polynomial f.
    rho = power2_ZZ(ell);
    rho_zz_p = power(ZZ_p(2L), ell);

    // Right now chi_zz = odd_factor * 2^{two_exponent - 2 - ell},
    // and thus
    // p = odd_factor * 2^{two_exponent} + 1
    //   = 2^2*odd_factor * 2^{two_exponent - 2 - ell} * 2^ell + 1
    //   = (4 chi_zz) * rho + 1.
    // So set chi := 4*odd_factor*2^{two_exponent - 2 - ell}, and
    // then p = chi*rho + 1.
    // We're assuming that (two_exponent - 2 - ell) is small
    // enough that we won't have wrap around errors with
    // chi. Pretty sure this is satisfied by the property that the
    // degree of the polynomial is a long, and hence not overly
    // large.
    chi = 4 * odd_factor;
    chi <<= two_exponent - 2L - ell;

    // z:= zeta_zz_p^{rho} is a primitive chi^th root of unity.
    ZZ_p const zeta_zz_p{zeta};
    auto const z{power(zeta_zz_p, rho)};
    // w = sqrt(z) and is used in batch_eval
    auto const w{power(zeta_zz_p, rho >> 1L)};

    // // Uncomment for timings when running graeffe.test.cpp
    // auto start = std::chrono::high_resolution_clock::now();
    auto const tau{random_ZZ_p()};
    auto const hs{tangent_graeffe_transform(f, rho, tau)};
    // // Uncomment for timings when running graeffe.test.cpp
    // auto stop = std::chrono::high_resolution_clock::now();
    // auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    // std::cout << "Time for graeffe_transform " << duration.count() << " microseconds" << std::endl;
    // start = stop;

    auto const & h{hs.first};
    auto const & hbar{hs.second};
    auto const hprime{diff(h)};

    ZZ_pX powers_of_w{};
    powers_of_w.SetLength(chi);
    ZZ_pX powers_of_w_inv{};
    powers_of_w_inv.SetLength(chi);
    SetCoeff(powers_of_w, 0L, 1L);
    SetCoeff(powers_of_w_inv, 0L, 1L);
    {
      long i = 1L;
      auto const w_squared = power(w, 2L);
      auto w_diff_square{w};
      auto w_power_i_squared{w};

      while (i < chi) {
        SetCoeff(powers_of_w, i, w_power_i_squared);
        SetCoeff(powers_of_w_inv, i, inv(w_power_i_squared));

        // update diff from 2i+1 to 2(i+1) + 1
        mul(w_diff_square, w_diff_square, w_squared);

        // update w_power_i_squared from w^{i^2} to w^{(i+1)^2}
        mul(w_power_i_squared, w_power_i_squared, w_diff_square);
        i++;
      }
    }

    auto const h_eval{batch_eval(h, powers_of_w, powers_of_w_inv)};
    auto const hbar_eval{
        batch_eval(hbar, powers_of_w, powers_of_w_inv)};
    auto const hprime_eval{
        batch_eval(hprime, powers_of_w, powers_of_w_inv)};

    ZZ_p y{1L};
    for (long i = 0L; i < chi; ++i) {
      // y := z^i ( = zeta^{i * 2^l})
      // if h(y) == 0 and hbar(y) != 0, then
      // f((rho*y h'(y)) / hbar(y) - tau) = 0.
      if ((IsZero(h_eval[i]) == 1L) && (IsZero(hbar_eval[i]) == 0L)) {
        Z.append(((rho_zz_p * y * hprime_eval[i]) / hbar_eval[i])
                 - tau);
      }
      y *= z;
    }
    if (IsZero(eval(f, tau)) == 1L) {
      Z.append(tau);
    }

    // // Uncomment for timings when running graeffe.test.cpp
    //   stop = std::chrono::high_resolution_clock::now();
    //   duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    //   std::cout << "Time for poly evals etc " << duration.count() << " microseconds" << std::endl;
    //   start = stop;

    if (Z.length() < deg(f)) {
      auto f2{BuildFromRoots(Z)};
      ZZ_pX f3{};
      if (divide(f3, f, f2) == 1L) {
        Z.append(find_roots(f3,
                            zeta,
                            two_exponent,
                            odd_factor));
      }
    }
  }
  return Z;
}

void newton_to_polynomial(ZZ_pX & output,
                          vec_ZZ_p const & newton_sums,
                          long const degree) {
  output.SetLength(degree+1);
  output[degree] = 1;
  output[degree - 1] = newton_sums[0] * -1;

  for (long i = degree - 2; i >= 0; i--) {
    output[i] = newton_sums[degree - 1 - i];
    for (long j = 0L; j < (degree - 1L) - i; j++) {
      output[i] += newton_sums[j] * output[i + j + 1];
    }
    output[i] *= (-1);
    output[i] *= inv(ZZ_p(degree - i));
  }
}

/*
void values_to_newton(vec_ZZ_p & output,
                      vec_ZZ_p const & values,
                      long const degree) {
  auto powers_of_roots = VectorCopy(values, degree);
  output[0] = 0L;
  for (long i = 0L; i < degree; i++) {
    output[0] += values[i];
  }
  for (long i = 1L; i < degree; i++) {
    output[i] = 0;

    for (long j = 0L; j < degree; j++) {
      mul(powers_of_roots[j], powers_of_roots[j], values[j]);
      output[i] += powers_of_roots[j];
    }
  }
}
*/
