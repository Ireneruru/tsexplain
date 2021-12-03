#include "tsexplain.h"

string str_join(const vector<string>& strs, const string& delim)
{
    string ret;

    bool first = true;
    for (auto &s : strs)
    {
        if (!first)
            ret += delim;
        ret += s;
        first = false;
    }
    return ret;
}

static int n_symbols;

static int _idx(const string& str)
{
    if (strings.count(str) == 0) {
        strings[str] = n_symbols++;
        string_list.push_back(str);
    }
    return strings[str];
}

DataSource::DataSource(const Spec& spec)
{
    auto start = CurrentTimeMillis();

    // clear the string-to-index mapping, create index for empty string ""
    strings.clear();
    n_symbols = 0;
    _idx("");

    // build sets of feature names, and time column names (including rollup time columns)
    unordered_set<string> dim_set(spec.features.begin(), spec.features.end());
    unordered_set<string> time_set(spec.time_cols.begin(), spec.time_cols.end());

    // clear contents
    content_dim.clear();
    content_time.clear();
    content_value.clear();
    all_time.clear();
    timeline.clear();

    // initialize content mapping (column name -> empty column)
    for (const auto& s : dim_set) {
        content_dim[_idx(s)] = vector<int>();
    }
    for (const auto& s : time_set) {
        content_time[_idx(s)] = vector<int>();
    }
    content_value = vector<double>();

    ifstream fin(spec.tsv_file);
    cout << spec.tsv_file << endl;

    string line, token;

    // get header
    getline(fin, line);
    vector<string> header;
    stringstream ss(line);
    while (getline(ss, token, '\t')) {
        header.push_back(token);
    }

    // read file
    n_rows = 0;
    while (getline(fin, line)) {
        int i = 0;
        stringstream ss(line);
        while (getline(ss, token, '\t')) {
            if (dim_set.count(header[i]) > 0) content_dim[_idx(header[i])].push_back(_idx(token));
            if (time_set.count(header[i]) > 0) content_time[_idx(header[i])].push_back(stoi(token));
            if (spec.val_col == header[i]) content_value.push_back(stod(token));
            i += 1;
        }
        n_rows++;
    }
    fin.close();

    auto read = CurrentTimeMillis();
    readfile_time = double(read - start) / 1000;

    start = CurrentTimeMillis();

    features_t cur_dim{};
    enumerate_combination(cur_dim, 0, spec);

    for (const auto& time : all_time) {
        if (time >= spec.time_min && time <= spec.time_max) {
            timeline.push_back(time);
        }
    }
    sort(timeline.begin(), timeline.end());
    auto cube = CurrentTimeMillis();
    compute_cube_time += double(cube - start) / 1000;
}

void DataSource::enumerate_combination(features_t& cur_dim, int next, const Spec& spec)
{
    trendlines[cur_dim] = __fetch_data(cur_dim, spec);
    for (const auto& d : cur_dim) cout << "[" << d << "] ";
    cout << "<" << spec.val_col << "> " << trendlines[cur_dim].size() <<endl;

    if (next < spec.features.size()) {
        for (int i = next; i < spec.features.size(); ++i) {
            cur_dim.push_back(spec.features[i]);
            enumerate_combination(cur_dim, i + 1, spec);
            cur_dim.pop_back();
        }
    }
}

pred_trend_t DataSource::__fetch_data(const vector<string>& features, const Spec& spec)
{
    pred_trend_t result;
    vector<int> _features, _time_cols;

    for (const string& s : features) {
        _features.push_back(_idx(s));
    }
    for (const string& s : spec.time_cols) _time_cols.push_back(_idx(s));

    for (int r = 0; r < n_rows; ++r)
    {
        predicate_t predicate;
        for (int i = 0; i < _features.size(); ++i) {
            predicate.push_back(make_pair(_features[i], content_dim[_features[i]][r]));
        }
        times_t time, rollup_time;
        for (int i = 0; i < _time_cols.size(); ++i) {
            time.push_back(content_time[_time_cols[i]][r]);
        }

        all_time.insert(time);
        rollup_map[time] = rollup_time;

        double val = content_value[r];

        if (result.count(predicate) == 0) {
            result[predicate] = trendline_t();
        }
        if (result[predicate].count(time) == 0) {
            result[predicate][time] = 0;
        }
        result[predicate][time] += val;
    }
    return result;
}

pred_trend_t DataSource::fetch_data(const vector<string>& features)
{
    vector<string> f(features);
    sort(f.begin(), f.end());
    return trendlines[f];
}

DataSource::~DataSource() {}
