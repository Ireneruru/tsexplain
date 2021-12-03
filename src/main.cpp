#include "tsexplain.h"

using namespace std;

int main(int argc, char* argv[])
{
    string spec_path = string(argv[1]);
    Spec spec = parse_spec_from_file(spec_path);
    // initialize data source
    DataSource *source;

    source = new DataSource(spec);

    // load data from database and compute trendline
    compute_trendline(spec, source);
    source->clear();

    if (spec.try_seg_len < timeline.size()) {
        // use two phase strategy
        phase1 = true;
        valid_time.clear();
        valid_time.resize(timeline.size(), true);
        compute_metric_scores(spec);
        cascading(0, timeline.size() - 1, spec);
        dp_optimal_fixedk(spec);

        valid_time.clear();
        valid_time.resize(timeline.size(), false);
        valid_time_idx.clear();
        auto &starts = *all_starts.rbegin();
        for (auto t : starts) {
            valid_time[t] = true;
            valid_time_idx.push_back(t);
        }
        valid_time[T - 1] = true;
        valid_time_idx.push_back(T - 1);
    }
    else {
        valid_time.clear();
        valid_time.resize(timeline.size(), true);
        valid_time_idx.clear();
        for (int i = 0; i < T; ++i) valid_time_idx.push_back(i);
    }

    phase1 = false;
    compute_metric_scores(spec);
    cascading(0, timeline.size() - 1, spec);
    if (spec.seg_number < 0) {
        spec.seg_number = -spec.seg_number;
        dp_optimal_fixedk(spec);

        map<int, double> best_scores;
        int seg = 1;
        for (int sn = 1; sn <= spec.seg_number; ++sn) {
            best_scores[sn] = 0;
            const auto& starts = all_starts[sn - 1];
            for (int i = 0; i < sn; ++i) {
                best_scores[sn] += seg_score[time_range_idx(make_pair(starts[i], starts[i+1]))];
            }
            if (best_scores[sn] > best_scores[seg] && best_scores[sn] - best_scores[sn-1] >  best_scores[sn-1] * 0.025) {
                seg = sn;
            }
            else if (sn > 2 && best_scores[sn-1] - best_scores[sn-2] <  best_scores[sn-2] * 0.025) break;
        }
        spec.seg_number = seg;
        dp_optimal_fixedk(spec);
    }
    else {
        dp_optimal_fixedk(spec);
    }

    vector<int> starts;

    for (int i = 0; i < all_starts.rbegin()->size(); ++i)
        cout << time_to_string(timeline[all_starts.rbegin()->at(i)]) << " ";
    cout << endl;

    // vis segmentation
    auto vis_seg = visual_segmentation(trend_full, timeline, spec.seg_number);
    double vis_score = 0;
    vector<int> vis_segpoint;
    for (const auto& t : vis_seg) {
        vis_score += seg_score[time_range_idx(t)];
        vis_segpoint.push_back(t.first);
    }

    ofstream fout("output.json");

    fout << "{" << endl;
    fout << "\"measurement\":" << endl;
    fout << "{" << endl;
    fout << "\"row_number\": " << source->n_rows << "," << endl;
    fout << "\"timeline_len\": " << timeline.size() << "," << endl;
    fout << "\"predicate_num\": " << predicates.size() << "," << endl;
    fout << "\"score\": " << dp_score << "," << endl;
    fout << "\"mavg_window\": " << mavg_window << "," << endl;
    fout << "\"read_file\": " << readfile_time << "," << endl;
    fout << "\"compute_cube\": " << compute_cube_time << "," << endl;
    fout << "\"compute_trendline\": " << compute_trendline_time << "," << endl;
    fout << "\"compute_metric\": " << compute_metric_time << "," << endl;
    fout << "\"cascading\": " << cascading_time << "," << endl;
    fout << "\"compute_sim\": " << compute_sim_time << "," << endl;
    fout << "\"search\": " << compute_segment_time << "," << endl;
    fout << "\"overall\": " << compute_trendline_time + compute_metric_time + cascading_time + compute_sim_time + compute_segment_time << endl;
    fout << "}," << endl;
    fout << "\"result\":" << endl;

    fout << "[" << endl;
    
    for (int firstN = 0; firstN < spec.seg_number; ++firstN)
    {
        starts = all_starts[firstN];

        if (false) { // get vis_segmentation_only
            if (firstN == spec.seg_number - 1) {
                cout << "seg" << endl;
                for (auto i : starts) cout << i << endl;
                starts = vis_segpoint;
            }
        }

        fout << "{" << endl;
        fout << "\"timeline\":" << endl;
        fout << "[" << endl;
        for (int i = 0; i < timeline.size(); ++i) {
            fout << "\"" << time_to_string(timeline[i]) << "\"";
            if (i + 1 < timeline.size()) fout << "," << endl; else fout << endl;
        }
        fout << "]," << endl;

        fout << "\"" << "trend_full" << "\":" << endl;
        fout << "[" << endl;
        for (int i = 0; i < timeline.size(); ++i) {
            fout << trend_full[timeline[i]];
            if (i + 1 < timeline.size()) fout << "," << endl; else fout << endl;
        }
        fout << "]," << endl;

        vector<int> tmp;
        for (int i = 0; i <= firstN; ++i) tmp.push_back(starts[i]);
        tmp.push_back(timeline.size() - 1);
        sort(tmp.begin(), tmp.end());

        fout << "\"" << "segments" << "\":" << endl;
        fout << "[" << endl;
        for (int i = 0; i < tmp.size() - 1; ++i) {
            fout << "{" << endl;

            fout << "\"begin\": " << tmp[i] << "," << endl;
            fout << "\"end\": " << tmp[i + 1] - spec.min_dist << "," << endl;
            fout << "\"sim\": " <<  seg_score[time_range_idx(make_pair(tmp[i], tmp[i+1]))] << "," << endl;

            fout << "\"explanation\": " << endl;
            fout << "[" << endl;
            for (int j = 0; j < explanation[time_range_idx(make_pair(tmp[i], tmp[i+1]-spec.min_dist))].size(); ++j) {
                auto& pred = predicates[explanation[time_range_idx(make_pair(tmp[i], tmp[i+1]-spec.min_dist))][j]];
                fout << "{" << endl;
                fout << "\"predicate\": " << endl;
                fout << "[" << endl;
                for (int k = 0; k < pred.size(); ++k) {
                    fout << "[\"" << string_list[pred[k].first] << "\", " << "\"" << string_list[pred[k].second] << "\"]" << endl;
                    if (k + 1 < pred.size()) fout << "," << endl; else fout << endl;
                }
                fout << "]," << endl;
                fout << "\"metric\": " << pred_scores[time_range_idx(make_pair(tmp[i], tmp[i+1]-spec.min_dist))][pred_idx[pred]].first << "," << endl;
                fout << "\"direction\": \"" << pred_scores[time_range_idx(make_pair(tmp[i], tmp[i+1]-spec.min_dist))][pred_idx[pred]].second << "\"," << endl;
                fout << "\"trend_sub\":" << endl;
                fout << "[" << endl;
                for (int t = tmp[i]; t <= tmp[i+1]-spec.min_dist; ++t) {
                    fout << trends[pred_idx[pred]][timeline[t]];
                    if (t + 1 <= tmp[i+1]-spec.min_dist) fout << "," << endl; else fout << endl;
                }
                fout << "]" << endl;
                fout << "}";
                if (j + 1 < explanation[time_range_idx(make_pair(tmp[i], tmp[i+1]-spec.min_dist))].size()) fout << "," << endl; else fout << endl;
            }
            fout << "]," << endl;

            fout << "\"" << "trend_comp" << "\":" << endl;
            fout << "[" << endl;
            for (int j = tmp[i]; j <= tmp[i+1] - spec.min_dist; ++j) {
                fout << trends_comp[explanation[time_range_idx(make_pair(tmp[i], tmp[i+1]-spec.min_dist))][0]][timeline[j]];
                if (j + 1 <= tmp[i+1] - spec.min_dist) fout << "," << endl; else fout << endl;
            }
            fout << "]" << endl;
            fout << "}";
            if (i + 1 < tmp.size() - 1) fout << "," << endl; else fout << endl;
        }
        fout << "]" << endl;
        fout << "}";
        if (firstN + 1 < spec.seg_number) fout << "," << endl; else fout << endl;
    }
    fout << "]" << endl;
    fout << "}" << endl;

    fout.close();

    ofstream flog("segments.txt");
    for (int Ln = 1; Ln < timeline.size(); ++Ln) {
        for (int i = 0; i < timeline.size() - Ln; ++i) {
            int j = i + Ln;
            flog << "(" << time_to_string(timeline[i]) << ")";
            flog << " ~ ";
            flog << "(" << time_to_string(timeline[j]) << ")";
            flog << endl;
            flog << "sim " << seg_score[time_range_idx(make_pair(i, j))] << " (" <<  seg_score[time_range_idx(make_pair(i, j))] / (j - i) << ")" << endl;
            for (int k = 0; k < explanation[time_range_idx(make_pair(i, j))].size(); ++k) {
                auto& pred = predicates[explanation[time_range_idx(make_pair(i, j))][k]];
                flog << "[";
                for (int t = 0; t < pred.size(); ++t) {
                    flog << string_list[pred[t].first] << "=" << string_list[pred[t].second];
                    if (t + 1 < pred.size()) flog << " & ";
                }
                flog << "], ";
                flog << pred_scores[time_range_idx(make_pair(i, j))][pred_idx[pred]].first << ", ";
                flog << pred_scores[time_range_idx(make_pair(i, j))][pred_idx[pred]].second << endl;
            }
        }
    }
    flog.close();

    return 0;
}

/*
 * Global data
 */

// map string to integer to speed up
unordered_map<string, int> strings;
vector<string> string_list;

vector<predicate_t> predicates;
unordered_map<predicate_t, int, predicate_hash> pred_idx;
unordered_map<pred_t, unordered_set<pred_t, pair_ii_hash>, pair_ii_hash> children_set;

int T;
vector<times_t> timeline;
vector<bool> valid_time;
vector<int> valid_time_idx;
trendlines_t trends;
trendlines_t trends_comp;
trendline_t trend_full;
range_scores_t pred_scores;
explanations_t explanation;
range_val_t seg_score;
double dp_score;
vector<vector<int>> all_starts;

vector<bool> explanation_computed;
vector<bool> seg_score_computed;

double readfile_time;
double compute_cube_time;
double compute_trendline_time;
double compute_metric_time;
double cascading_time;
double compute_sim_time;
double compute_segment_time;

bool phase1;
