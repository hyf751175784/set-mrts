// Copyright [2016] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <random_gen.h>
#include <iostream>

// default_random_engine
//     Random_Gen::generator(std::chrono::system_clock::now().time_since_epoch()
//      .count());
using std::max;
using std::min;

std::random_device rd;
std::mt19937 Random_Gen::generator(rd());


double Random_Gen::exponential_gen(double lambda) {
  exponential_distribution<double> distribution(lambda);
  return distribution(generator);
}

int Random_Gen::uniform_integral_gen(int min, int max) {
  if (min > max)
    std::swap(min, max);
  uniform_int_distribution<int> distribution(min, max);
  return distribution(generator);
}

ulong Random_Gen::uniform_ulong_gen(ulong min, ulong max) {
  if (min > max)
    std::swap(min, max);
  uniform_int_distribution<ulong> distribution(min, max);
  return distribution(generator);
}

double Random_Gen::uniform_real_gen(double min, double max) {
  if (min > max)
    std::swap(min, max);
  uniform_real_distribution<double> distribution(min, max);
  return distribution(generator);
}

bool Random_Gen::probability(double prob) {
  int i = 1, j = 0;
  if (1 < prob) prob = 1;
  while (prob != floor(prob) && j++ < 3) {
    prob *= 10;
    i *= 10;
  }
  prob = floor(prob);
  if (uniform_integral_gen(1, i) <= prob)
    return true;
  else
    return false;
}

vector<double> Random_Gen::RandFixedSum(uint size, double sum, double l_bound,
                                        double u_bound) {
  // Check the arguments
  if (size < 1) {
    cout << "RandomNumber.RandFixedSum was passed arguments that violate the "
            "rule: size < 1"
         << endl;
  }
  if (sum < size * l_bound) {
    cout << "RandomNumber.RandFixedSum was passed arguments that violate the "
            "rule: sum < size * min"
         << endl;
  }
  if (sum > size * u_bound) {
    cout << "RandomNumber.RandFixedSum was passed arguments that violate the "
            "rule: sum > size * max"
         << endl;
  }
  if (l_bound >= u_bound) {
    cout << "RandomNumber.RandFixedSum was passed arguments that violate the "
            "rule: min >= max"
         << endl;
  }
  // Rescale to a unit cube: 0 <= x(i) <= 1
  sum = (sum - static_cast<double>(size * l_bound)) / (u_bound - l_bound);

  // Must have 0 <= k <= size - 1
  double k = max(min(floor(sum), static_cast<double>(size - 1)), 0.0);
  // Must have k <= sum <= k + 1
  sum = max(min(sum, k + 1), k);

  // s1 & s2 will never be negative
  vector<double> s1, s2;

  for (int element = k; element >= (k - size + 1); element--) {
    s1.push_back(sum - element);
  }
  for (int element = (k + size); element >= (k + 1); element--) {
    s2.push_back(element - sum);
  }

  vector<vector<double>> w;

  for (int i = 0; i < size; i++) {
    vector<double> temp;
    for (int j = 0; j <= size; j++) temp.push_back(0);
    w.push_back(temp);
  }
  w[0][1] = DBL_MAX;

  vector<vector<double>> t;
  for (int i = 0; i < (size - 1); i++) {
    vector<double> temp;
    for (int j = 0; j <= (size - 1); j++) temp.push_back(0);
    t.push_back(temp);
  }

  double tiny = DBL_MIN;

  for (int i = 1; i < size; i++) {
    vector<double> tmp1, tmp2, tmp3, tmp4;

    int sizeIndex = size - (i + 1);
    for (int j = 0; j <= i; j++) {
      tmp1.push_back(w[i - 1][j + 1] * (s1[j] / (static_cast<double>(i) + 1)));
      tmp2.push_back(w[i - 1][j] *
                     (s2[sizeIndex] / (static_cast<double>(i) + 1)));
      sizeIndex++;
    }

    for (int j = 0; j <= i; j++) {
      w[i][j + 1] = tmp1[j] + tmp2[j];
    }

    sizeIndex = size - (i + 1);
    for (int j = 0; j <= i; j++) {
      // In case tmp1 & tmp2 are both 0,
      tmp3.push_back(w[i][j + 1] + tiny);
      // then t is 0 on left & 1 on right
      tmp4.push_back(s2[sizeIndex] > s1[j] ? 1 : 0);
      t[i - 1][j] = (tmp2[j] / tmp3[j]) * tmp4[j] +
                    (1 - tmp1[j] / tmp3[j]) * (tmp4[j] == 0 ? 1 : 0);
      sizeIndex++;
    }
  }

  // Derive the polytope volume v from the appropriate
  // element in the bottom row of w.
  double v = pow(size, 3.0 / 2.0) *
             (w[size - 1][static_cast<int>(k) + 1] / DBL_MAX) *
             pow(u_bound - l_bound, size - 1);

  // Now compute the matrix x.
  vector<double> x;
  for (int j = 0; j < size; j++) x.push_back(0);
  vector<double> rt;
  vector<double> rs;

  for (int i = 0; i < size - 1; i++) {
    // For random selection of simplex type
    rt.push_back(Random_Gen::uniform_real_gen(0, 1));
    // For random location within a simplex
    rs.push_back(Random_Gen::uniform_real_gen(0, 1));
  }

  // Start with sum zero & product 1
  double sm = 0, pr = 1;
  int jj = 0;

  // Work backwards in the t table
  for (int i = size - 1; i > 0; i--) {
    double e, sx;
    // Use rt to choose a transition
    e = rt[jj] <= t[i - 1][static_cast<int>(k)] ? 1 : 0;
    // Use rs to compute next simplex coord.
    sx = pow(rs[jj], 1 / static_cast<double>(i));
    // Update sum
    sm += (1 - sx) * pr * sum / static_cast<double>(i + 1);
    // Update product
    pr *= sx;
    // Calculate x using simplex coords.
    x[jj] = sm + pr * e;
    // Transition adjustment
    sum -= e;
    k -= e;

    jj++;
  }

  // Compute the last x
  x[size - 1] = sm + pr * sum;

  // Randomly permute the order in the columns of x and rescale.
  vector<int> p;
  for (int i = 0; i < size; i++) {
    p.push_back(i);
  }

  // // Use p to carry out a matrix 'randperm'
  // try {
  //   p.Sort(delegate(int p1, int p2) {
  //     return ((p1 == p2) ? 0 : RandomNumber(-1, 1));
  //   });
  // } catch (ArgumentException e) {
  //   // Sort sometimes throws an IComparable exception.
  //   // This doesn't seem to affect the result of the algorithm,
  //   // so catch and ignore the error.
  // }

  vector<double> ret;
  for (int i = 0; i < size; i++) {
    // Permute & rescale x
    ret.push_back(static_cast<double>(u_bound - l_bound) *
                      static_cast<double>(x[p[i]]) +
                  static_cast<double>(l_bound));
  }

  return ret;
}