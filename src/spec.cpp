#include "tsexplain.h"

times_t to_vector(const string& s)
{
    istringstream ss(s);
    times_t v;
    int n;
    while (ss >> n) v.push_back(n);
    return v;
}

Spec parse_spec_from_file(string spec_path)
{
    ifstream f(spec_path);
    string line;
    Spec spec;

    //tsv_file: [absolute file path to .tsv data]
    getline(f, line);
    spec.tsv_file = trim(line.substr(line.find_first_of(':') + 1));

    //feature_hier_number: [number of feature hierarchies]
    //feature_number: [the number of features of the 1st hierarchy]
    //[feature_1]
    //...
    getline(f, line);
    int n_feat_heir = stoi(trim(line.substr(line.find_first_of(':') + 1)));
    while (n_feat_heir--) {
        getline(f, line);
        int n_feat = stoi(trim(line.substr(line.find_first_of(':') + 1)));
        spec.feature_hier.emplace_back();
        while (n_feat--) {
            getline(f, line);
            string feat = trim(line);
            spec.feature_hier.rbegin()->push_back(feat);
            spec.features.push_back(feat);
        }
    }
    sort(spec.features.begin(), spec.features.end());

    //time_col_number: [the number of columns to determine the timeline]
    //[time_col_1]
    //...
    getline(f, line);
    int n_timec = stoi(trim(line.substr(line.find_first_of(':') + 1)));
    while (n_timec--) {
        getline(f, line);
        spec.time_cols.push_back(trim(line));
    }

    getline(f, line);
    spec.try_seg_len_ratio = stod(trim(line.substr(line.find_first_of(':') + 1)));

    getline(f, line);
    spec.try_total_len_ratio = stod(trim(line.substr(line.find_first_of(':') + 1)));

    //val_col: [the column of the target value]
    getline(f, line);
    spec.val_col = trim(line.substr(line.find_first_of(':') + 1));

    //supp_ratio: [a double number]
    getline(f, line);
    spec.supp_ratio = stod(trim(line.substr(line.find_first_of(':') + 1)));

    //time_min: [first int] [second int]
    getline(f, line);
    spec.time_min = to_vector(line.substr(line.find_first_of(':') + 1));

    //time_max: [first int] [second int]
    getline(f, line);
    spec.time_max = to_vector(line.substr(line.find_first_of(':') + 1));

    //post_process: [function name of post process, none means no post_process]
    getline(f, line);
    spec.mavg_window = stoi(trim(line.substr(line.find_first_of(':') + 1)));
    if (spec.mavg_window == 0) {
        spec.post_process = nullptr;
    }
    else {
        spec.post_process = moving_avg_left_k_right_k;
    }

    //explain_time_min: [first int] [second int]
    getline(f, line);
    spec.explain_time_start = to_vector(line.substr(line.find_first_of(':') + 1));

    //explain_time_max: [first int] [second int]
    getline(f, line);
    spec.explain_time_end = to_vector(line.substr(line.find_first_of(':') + 1));

    //metric: [name of metric]
    getline(f, line);
    string met = trim(line.substr(line.find_first_of(':') + 1));
    if (met == "MChangeAbs") {
        spec.metric = MChangeAbs;
    }

    //topK: [int of topK]
    getline(f, line);
    spec.topK = stoi(trim(line.substr(line.find_first_of(':') + 1)));

    //pred_len: [int of max predicate length, -1 means no restriction]
    getline(f, line);
    spec.pred_len = stoi(trim(line.substr(line.find_first_of(':') + 1)));
    if (spec.pred_len == -1) spec.pred_len = spec.feature_hier.size();

    //sim: [name of similarity score]
    getline(f, line);
    spec.sim = trim(line.substr(line.find_first_of(':') + 1));

    //seg_number: [number of segmentation in the result]
    getline(f, line);
    spec.seg_number = stoi(trim(line.substr(line.find_first_of(':') + 1)));

    //min_len: [min length of one segment]
    getline(f, line);
    spec.min_len = stoi(trim(line.substr(line.find_first_of(':') + 1)));

    //min_dist: [min length between two consecutive segments]
    getline(f, line);
    spec.min_dist = stoi(trim(line.substr(line.find_first_of(':') + 1)));

    //cascading_opt: bool
    getline(f, line);
    spec.cascading_opt = stoi(trim(line.substr(line.find_first_of(':') + 1)));

    //starts_number: [the number of fixed starts, 0 or seg_number]
    //[starts_1]
    //...
    getline(f, line);
    int n_starts = stoi(trim(line.substr(line.find_first_of(':') + 1)));
    while (n_starts--) {
        getline(f, line);
        spec.starts.push_back(stoi(trim(line)));
    }

    return spec;
}
