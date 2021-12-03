#include "tsexplain.h"

vector<time_range_t> visual_segmentation(trendline_t &trend, vector<times_t>& timeline, int seg_number)
{
    map<time_range_t, double> error;
    map<time_range_t, double> merge_error;
    vector<time_range_t> segments;

    int N = timeline.size();
    for (int i = 0; i < N - 1; ++i) {
        segments.emplace_back(make_pair(i, i + 1));
    }
    for (int i = 0; i < N - 1; ++i) {
        for (int j = i + 1; j < N; ++j) {
            vector<double> x;
            vector<double> y;
            for (int k = i; k <= j; ++k) {
                x.push_back(k);
                y.push_back(trend.at(timeline[k]));
            }
            error[make_pair(i, j)] = L1_error(x, y);
        }
    }
    for (int i = 0; i < segments.size() - 1; ++i) {
        merge_error[segments[i]] = error[make_pair(i, i + 2)];
    }
    double ERROR = 0;
    while (segments.size() > seg_number) {
        int min_i = -1;
        for (int i = 0; i < segments.size() - 1; ++i) {
            int ll = segments[i].first, rr = segments[i + 1].second;
            //if (error[make_pair(ll, rr)] > sub_bound) continue;
            if (min_i == -1 || merge_error[segments[i]] < merge_error[segments[min_i]]) min_i = i;
        }
        if (min_i == -1) break;
        ERROR += merge_error[segments[min_i]];
        int l = segments[min_i].first, r = segments[min_i].second;
        int ll = segments[min_i + 1].first, rr = segments[min_i + 1].second;
        segments.erase(segments.begin() + min_i + 1);
        segments[min_i] = make_pair(l, rr);
        if (min_i < segments.size() - 1) {
            rr = segments[min_i + 1].second;
            merge_error[segments[min_i]] = error[make_pair(l, rr)] - error[segments[min_i]] - error[segments[min_i + 1]];
        }
        if (min_i > 0) {
            ll = segments[min_i - 1].first;
            merge_error[segments[min_i - 1]] = error[make_pair(ll, rr)] - error[segments[min_i - 1]] - error[segments[min_i]];
        }
    }
    return segments;
}
