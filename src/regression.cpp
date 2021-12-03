#include "tsexplain.h"

double R2_error(const vector<double>& x, const vector<double>& y)
{
    double avg_x = 0, avg_y = 0;
    for (const auto& xi : x) avg_x += xi;
    for (const auto& yi : y) avg_y += yi;
    avg_x /= x.size();
    avg_y /= y.size();
    double cov_xy = 0, cov_xx = 0;
    for (int i = 0; i < x.size(); ++i) {
        cov_xy += (x[i] - avg_x) * (y[i] - avg_y);
        cov_xx += (x[i] - avg_x) * (x[i] - avg_x);
    }
    double b = cov_xy / cov_xx;
    double a = avg_y - b * avg_x;
    double err = 0;
    for (int i = 0; i < x.size(); ++i) {
        err += (a + x[i] * b - y[i]) * (a + x[i] * b - y[i]);
    }
    double total = 0;
    for (int i = 0; i < x.size(); ++i) {
        total += (y[i] - avg_y) * (y[i] - avg_y);
    }
    return 1 - err / total;
}

double L1_error(const vector<double>& x, const vector<double>& y)
{
    double avg_x = 0, avg_y = 0;
    for (const auto& xi : x) avg_x += xi;
    for (const auto& yi : y) avg_y += yi;
    avg_x /= x.size();
    avg_y /= y.size();
    double cov_xy = 0, cov_xx = 0;
    for (int i = 0; i < x.size(); ++i) {
        cov_xy += (x[i] - avg_x) * (y[i] - avg_y);
        cov_xx += (x[i] - avg_x) * (x[i] - avg_x);
    }
    double b = cov_xy / cov_xx;
    double a = avg_y - b * avg_x;
    double err = 0;
    for (int i = 0; i < x.size(); ++i) {
        err += abs(a + x[i] * b - y[i]);
    }
    return err;
}

double sign(double x)
{
  if (x > 1E-8) return 1;
  else if (x < -1E-8)  return -1;
  else return 0;
}

double monotonic(const vector<double>& x, const vector<double>& y)
{
  int N = y.size();

  double S = 0;

  for (int i = 0; i < N - 1; ++i)
    for (int j = i+1; j < N; ++j)
      S += sign(y[j] - y[i]);

  return 1 - abs(S /( (N) * (N-1) / 2));
}
