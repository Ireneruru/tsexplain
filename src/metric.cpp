#include "tsexplain.h"

score_t MChangeAbs(vector<double>& trendfull, vector<double>& trend, int first, int last)
{
    double start_val = trendfull[first] - trend[first];
    double end_val = trendfull[last] - trend[last];

    if (start_val > end_val) {
        return make_pair(start_val - end_val, '-');
    }
    else {
        return make_pair(end_val - start_val, '+');
    }
}
