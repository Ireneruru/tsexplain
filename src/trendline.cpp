#include "tsexplain.h"

int mavg_window = 0;

trendline_t moving_avg_left_k_right_k(trendline_t& init_trendline, vector<times_t>& timeline)
{
    trendline_t trend;
    for (int i = 0; i < timeline.size(); ++i)
    {
        double sum = 0;
        int n = 0;
        for (int j = i - mavg_window; j <= i + mavg_window; ++j) {
            if (j >= 0 && j < timeline.size()) {
                sum += init_trendline[timeline[j]];
                n++;
            }
        }
        trend[timeline[i]] = sum / n;
    }
    return trend;
}

void __enumerate_features(features_t& cur_dim, const Spec& spec, int next, pred_trend_t& trends, DataSource* source)
{
    if (cur_dim.size() > spec.pred_len) return;
    if (next == spec.feature_hier.size()) {
        const auto& t = source->fetch_data(cur_dim);
        trends.insert(t.begin(), t.end());
        for (const auto& p : t) {
            predicates.push_back(p.first);
        }
    }
    else {
        __enumerate_features(cur_dim, spec, next + 1, trends, source);
        for (int i = 0; i < spec.feature_hier[next].size(); ++i) {
            cur_dim.push_back(spec.feature_hier[next][i]);
            __enumerate_features(cur_dim, spec, next + 1, trends, source);
            cur_dim.pop_back();
        }
    }
}

pred_trend_t raw_trends;

void compute_trendline(Spec& spec, DataSource* source)
{
    auto start = CurrentTimeMillis();

    mavg_window = spec.mavg_window;
    raw_trends.clear();

    auto cur_feat = features_t();
    __enumerate_features(cur_feat, spec, 0, raw_trends, source);
    if (spec.post_process) {
        for (auto& trend : raw_trends) {
            trend.second = (*spec.post_process)(trend.second, source->timeline);
        }
    }
    trend_full = raw_trends[predicate_t()];

    timeline = source->timeline;
    spec.try_seg_len = (int)(spec.try_seg_len_ratio * timeline.size());
    spec.try_seg_number = (int)(spec.try_total_len_ratio * timeline.size() / spec.try_seg_len);

    while (!timeline.empty() && *timeline.begin() < spec.explain_time_start)
        timeline.erase(timeline.begin());
    while (!timeline.empty() && *timeline.rbegin() > spec.explain_time_end)
        timeline.erase(timeline.begin() + (timeline.size() - 1));

    vector<predicate_t> deleted_pred{predicate_t()};
    for (const auto& pred : predicates) {
        bool deleted = true;
        for (const auto& t : timeline) {
            if (abs(raw_trends[pred][t]) > abs(trend_full[t]) * spec.supp_ratio) {
                deleted = false;
                break;
            }
        }
        if (deleted) deleted_pred.push_back(pred);
    }
    for (const auto& pred : deleted_pred) {
        predicates.erase(find(predicates.begin(), predicates.end(), pred));
    }

    int i = 0;
    for (const auto& pred: predicates) {
        pred_idx[pred] = i;
        trends.push_back(raw_trends[pred]);
        trends_comp.push_back(trendline_t());
        for (const auto& time : source->all_time) {
            trends_comp[i][time] = trend_full[time] - trends[i][time];
        }
        i++;
    }
    T = timeline.size();

    raw_trends.clear();

    // set children[(A=?)] = [(A=x), (A=y)]
    for (const auto& hier : spec.feature_hier) {
        children_set[make_pair(strings[hier[0]], 0)] = unordered_set<pred_t, pair_ii_hash>();
    }
    for (const auto &pred : predicates) {
        for (const auto &p : pred) {
            if (children_set.count(make_pair(p.first, 0)) > 0) {
                children_set[make_pair(p.first, 0)].insert(p);
            }
            if (children_set.count(p) == 0) {
                children_set[p] = unordered_set<pred_t, pair_ii_hash>();
            }
        }
    }

    // compute children relation from co-occurrence of predicates
    for (const auto &hier : spec.feature_hier) {
        for (int i = 0; i < hier.size() - 1; ++i) {
            auto& p = hier[i], &sp = hier[i+1];
            features_t feats{p, sp};
            const auto& d = source->fetch_data(feats);
            for (const auto &pred : d) {
                int ip, isp;
                if (pred.first.at(0).first == strings[p]) {
                    ip = 0; isp = 1;
                }
                else {
                    ip = 1; isp = 0;
                }
                children_set[pred.first.at(ip)].insert(pred.first.at(isp));
            }
        }
    }

    auto end = CurrentTimeMillis();
    compute_trendline_time += double(end - start) / 1000;
}


void compute_metric_scores(const Spec& spec)
{
    auto start = CurrentTimeMillis();

    vector<double> trendfull;
    vector<double> trend;

    if (pred_scores.empty()) {
        pred_scores.resize(T * T, vector<score_t>(predicates.size()));
        trendfull.reserve(timeline.size());
        trend.reserve(timeline.size());
    }
    trendfull.clear();
    for (const auto& time : timeline) {
        trendfull.push_back(trend_full[time]);
    }

    for (int k = 0; k < predicates.size(); ++k) {
        trend.clear();
        for (const auto& time : timeline) {
            trend.push_back(trends_comp[k][time]);
        }
        if (phase1) {
            for (int L = 1; L < spec.try_seg_len; ++L) {
                for (int i = 0; i < T - L; ++i) {
                    int j = i + L;
                    auto time_range = make_pair(i, j);

                    auto sc = MChangeAbs(trendfull, trend, i, j);
                    sc.first += k * 1E-6; // remove randomness
                    pred_scores[time_range_idx(time_range)][k] = sc;
                }
            }
        }
        else {
            for (int i = 0; i < T - 1; ++i) {
                int j = i + 1;
                auto time_range = make_pair(i, j);
                auto sc = MChangeAbs(trendfull, trend, i, j);
                sc.first += k * 1E-6; // remove randomness
                pred_scores[time_range_idx(time_range)][k] = sc;
            }
            for (int ii = 0; ii < valid_time_idx.size(); ++ii) {
                for (int jj = ii + 1; jj < valid_time_idx.size(); ++jj) {
                    int i = valid_time_idx[ii];
                    int j = valid_time_idx[jj];
                    auto time_range = make_pair(i, j);
                    auto sc = MChangeAbs(trendfull, trend, i, j);
                    sc.first += k * 1E-6; // remove randomness
                    pred_scores[time_range_idx(time_range)][k] = sc;
                }
            }
        }
    }

    auto end = CurrentTimeMillis();
    compute_metric_time += double(end - start) / 1000;
}
