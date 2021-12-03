#include "tsexplain.h"
#include <cmath>

vector<int> best_starts;

void compute_score(int st, int ed, const Spec& spec, const vector<double> &log2n)
{
    auto time = make_pair(st, ed);
    int time_idx = time_range_idx(time);
    if (seg_score_computed[time_idx]) return;

    double optimal_expl_score = 0;
    for (int i = 0; i < explanation[time_idx].size(); ++i) {
        if (explanation[time_idx].size() <= i) break;
        int pred = explanation[time_idx][i];
        optimal_expl_score += pred_scores[time_idx][pred].first / log2n[i + 2];
    }

    seg_score[time_idx] = 0;
    double eps = 1E-8;
    int n = 0;
    for (int k = st; k <= ed - (spec.min_len - 1); ++k) {
        for (int l = k + (spec.min_len - 1); l <= k + (spec.min_len - 1); ++l) {
            double summarized_score = 0;
            double optimal_small = 0;
            auto time_kl = make_pair(k, l);
            int time_kl_idx = time_range_idx(time_kl);
            //small seg exp's score on larger segment
            for (int i = 0; i < explanation[time_kl_idx].size(); ++i) {
                int pred = explanation[time_kl_idx][i];
                const auto &expl = pred_scores[time_kl_idx][pred];
                const auto &expl_broad = pred_scores[time_idx][pred];
                if (expl.second == expl_broad.second) {
                    summarized_score += expl_broad.first / log2n[i + 2];
                }
                optimal_small += expl.first / log2n[i + 2];
            }

            double summarized_broad = 0;
            // larger seg exp's score on small segment
            for (int i = 0; i < explanation[time_idx].size(); ++i) {
                int pred = explanation[time_idx][i];
                const auto &expl = pred_scores[time_kl_idx][pred];
                const auto &expl_broad = pred_scores[time_idx][pred];
                if (expl.second == expl_broad.second) {
                    summarized_broad += expl.first / log2n[i + 2];
                }
            }

            //sim1 + sim2 /2
            seg_score[time_idx] += 0.5*(min(1.0, (summarized_score + eps) / (optimal_expl_score + eps)) + min(1.0, (summarized_broad + eps) / (optimal_small + eps)));
            n += 1;
        }
    }
    seg_score[time_idx] = seg_score[time_idx] / n * (ed - st + spec.min_dist);
    seg_score_computed[time_idx] = true;
}


void analysis_prep(const Spec& spec)
{
    auto start = CurrentTimeMillis();

    // compute segment scores
    vector<double> log2n;
    log2n.push_back(0);
    for (int i = 1; i <= spec.topK + 1; ++i) {
        log2n.push_back(log(i) / log(2));
        //log2n.push_back(1);
    }

    if (seg_score.empty()) {
        seg_score.resize(T * T);
        seg_score_computed.resize(T * T);
    }

    if (phase1) {
        for (int Len = 1; Len < spec.try_seg_len; ++Len) {
            for (int st = 0; st < T-Len; ++st) {
                int ed = st + Len;
                compute_score(st, ed, spec, log2n);
            }
        }
    }
    else {
        for (int ii = 0; ii < valid_time_idx.size(); ++ii) {
            for (int jj = ii + 1; jj < valid_time_idx.size(); ++jj) {
                int st = valid_time_idx[ii];
                int ed = valid_time_idx[jj];
                compute_score(st, ed, spec, log2n);
            }
        }
    }

    auto end = CurrentTimeMillis();
    compute_sim_time += double(end - start) / 1000;
}

static vector<double> F;
static vector<pair<int, double>> G;

// compute the one score for (i, n)
static inline int _dpid(int i, int n) { return (n - 1) * T + i; }

//F[i, n] :  0 - i the best score
//G[i, n] :  j, s.t. F[i, n] is obtained from F[j, n-1]

void dp_optimal_fixedk(const Spec& spec)
{
    analysis_prep(spec);

    auto start = CurrentTimeMillis();

    int SegN;
    if (phase1) SegN = spec.try_seg_number;
    else SegN = spec.seg_number;

    F.clear();
    G.clear();
    F.resize(T * (SegN + 1));
    G.resize(T * (SegN + 1));

    for (int n = 1; n <= SegN; ++n) {
        for (int i = spec.min_len - 1; i < T; ++i) {
            F[_dpid(i, n)] = -1E30;
            G[_dpid(i, n)] = make_pair(0, 0);
            if (!valid_time[i]) continue;
            if (n == 1) {
                if (seg_score[time_range_idx(make_pair(0, i))] > F[_dpid(i, n)]) {
                    F[_dpid(i, n)] = seg_score[time_range_idx(make_pair(0, i))];
                    G[_dpid(i, n)] = make_pair(0, 0);
                }
            }
            else if (n > 1) {
                for (int j = i - spec.min_len + 1; j - spec.min_dist >= 0; --j) {
                    if (phase1 && i - j > spec.try_seg_len) break;
                    if (!valid_time[j]) continue;
                    double val_k = F[_dpid(j - spec.min_dist, n - 1)] + seg_score[time_range_idx(make_pair(j, i))];
                    if (val_k > F[_dpid(i, n)]) {
                        F[_dpid(i, n)] = val_k;
                        G[_dpid(i, n)] = make_pair(j, 0);
                    }
                }
            }
        }
    }

    all_starts.clear();
    for (int l = 1; l <= SegN; ++l) {
        best_starts.clear();
        best_starts.resize(l);

        int i = T - 1;

        for (int n = l; n >= 1; --n) {
            best_starts[n - 1] = G[_dpid(i, n)].first;
            i = G[_dpid(i, n)].first - spec.min_dist;
        }
        all_starts.push_back(best_starts);
    }

    dp_score = F[_dpid(T - 1, SegN)];

    auto end = CurrentTimeMillis();
    compute_segment_time += double(end - start) / 1000;
}
