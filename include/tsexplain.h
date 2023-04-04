#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <string>
#include <vector>
#include <ctime>
#include <utility>
#include <chrono>
#include <map>

using namespace std;

/*
 * types
 */

typedef vector<int> times_t;
typedef vector<string> features_t;
typedef pair<int, int> pred_t;
typedef vector<pred_t> predicate_t;
typedef pair<double, char> score_t;
typedef pair<int, int> time_range_t;

/*
 * Hash
 */

const size_t _P = 100000007;
const size_t _Q = 100000037;
const size_t _R = 100000039;

struct pair_ii_hash {
    size_t operator()(const pair<int, int> &p) const {
        return p.first * _P + p.second;
    }
};

struct predicate_hash {
    size_t operator()(const predicate_t &pred) const {
        size_t h = 0;
        for (const auto& p : pred) {
            h = (h * _Q) + p.first * _P + p.second;
        }
        return h;
    }
};

struct predicate_int_hash {
    size_t operator()(const pair<predicate_t, int> &pred) const {
        return predicate_hash()(pred.first) * _R + pred.second;
    }
};

struct features_hash {
    size_t operator()(const features_t &feats) const {
        size_t h = 0;
        auto _hash = hash<string>();
        for (const auto& f : feats) {
            h = (h * _P) + _hash(f);
        }
        return h;
    }
};

struct times_hash {
    size_t operator()(const times_t &feats) const {
        size_t h = 0;
        for (const auto& f : feats) {
            h = (h * _P) + f;
        }
        return h;
    }
};

typedef unordered_map<times_t, double, times_hash> trendline_t;
typedef unordered_set<times_t, times_hash> timeset_t;
typedef unordered_set<predicate_t, predicate_hash> predset_t;
typedef unordered_map<predicate_t, trendline_t, predicate_hash> pred_trend_t;
typedef vector<trendline_t> trendlines_t;
typedef vector<vector<score_t> > range_scores_t;
typedef vector<vector<int> > explanations_t;
typedef vector<double> range_val_t;

typedef trendline_t (*trendline_processor)(trendline_t& init_trendline, vector<times_t>& timeline);
typedef pair<double, char> (*metric_t)(
        vector<double>& trendfull, vector<double>& trend, int first, int last);


/*
 * specification
 */

struct Spec {
    string tsv_file;
    vector<features_t> feature_hier;
    features_t features;
    features_t time_cols;

    double try_seg_len_ratio;
    double try_total_len_ratio;
    int try_seg_number;
    int try_seg_len;

    string val_col;
    double supp_ratio;

    times_t time_min;
    times_t time_max;

    trendline_processor post_process;
    int mavg_window;
    times_t explain_time_start;
    times_t explain_time_end;

    metric_t metric;
    int topK;
    int pred_len;
    string sim;

    int seg_number;
    int min_len; //min_len: 2: the min length has two points, 1: the min length can only have 1 point
    int min_dist; // min_dist: 0: two segments can have overlapping ending points; 1: two segments can not have overlapping ending points

    int cascading_opt;

    vector<int> starts;
};


/*
 * global variables
 */

// map string to integer to speed up
extern unordered_map<string, int> strings;
extern vector<string> string_list;

extern vector<predicate_t> predicates;
extern unordered_map<predicate_t, int, predicate_hash> pred_idx;
extern unordered_map<pred_t, unordered_set<pred_t, pair_ii_hash>, pair_ii_hash> children_set;

// phase indicator
extern bool phase1;

// trendlines
extern vector<times_t> timeline;
extern vector<bool> valid_time;
extern vector<int> valid_time_idx;
extern trendlines_t trends;
extern trendlines_t trends_comp;
extern trendline_t trend_full;
extern int T;
static inline int time_range_idx(const pair<int, int>& range) { return range.first * T + range.second; }

// compute metrics
extern range_scores_t pred_scores;

// cascading result
extern explanations_t explanation;
extern vector<bool> explanation_computed;

// sim score
extern range_val_t seg_score;
extern vector<bool> seg_score_computed;

// dp results
extern double dp_score;
extern vector<vector<int>> all_starts;

// measurements
extern double readfile_time;
extern double compute_cube_time;
extern double compute_trendline_time;
extern double compute_metric_time;
extern double cascading_time;
extern double compute_sim_time;
extern double compute_segment_time;

/*
 * Data Source Class
 */

class DataSource
{
private:
    // map string to integer to accelarate
    // dim_index -> dim values of all rows
    unordered_map<int, vector<int>> content_dim;
    // time_index -> time values of all rows
    unordered_map<int, vector<int>> content_time;
    // value_index -> time values of all rows
    vector<double> content_value;

    unordered_map<features_t, pred_trend_t, features_hash> trendlines;

    void enumerate_combination(features_t& cur_dim, int next, const Spec& spec);
    pred_trend_t __fetch_data( const vector<string>& features, const Spec& spec);
public:
    int n_rows;
    timeset_t all_time;
    vector<times_t> timeline;

    unordered_map<times_t, times_t, times_hash> rollup_map;
    pred_trend_t fetch_data(const vector<string>& features);
    DataSource(const Spec& spec);
    void clear() {
        content_dim.clear();
        content_time.clear();
        content_value.clear();
        trendlines.clear();
    }
    ~DataSource();
};

/*
 * Metrics
 */

score_t MChangeAbs(vector<double>& trend_full, vector<double>& trend, int first, int last);

/*
 * procedures
 */

Spec parse_spec_from_file(string spec_path);
extern int mavg_window;
trendline_t moving_avg_left_k_right_k(trendline_t& init_trendline, vector<times_t>& timeline);
void compute_trendline(Spec &spec, DataSource* source);
void compute_metric_scores(const Spec& spec);
vector<time_range_t> visual_segmentation(trendline_t &trend, vector<times_t>& timeline, int seg_number);
void cascading(int first, int last, const Spec& spec);
double R2_error(const vector<double>& x, const vector<double>& y);
double L1_error(const vector<double>& x, const vector<double>& y);
double monotonic(const vector<double>& x, const vector<double>& y);
void dp_optimal_fixedk(const Spec& spec);

/*
 * Utils
 */
static int64_t CurrentTimeMillis()
{
    int64_t timems = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    return timems;
}

template <std::ctype_base::mask mask>
class IsNot
{
    std::locale myLocale;       // To ensure lifetime of facet...
    std::ctype<char> const* myCType;
public:
    IsNot( std::locale const& l = std::locale() )
            : myLocale( l )
            , myCType( &std::use_facet<std::ctype<char> >( l ) )
    {
    }
    bool operator()( char ch ) const
    {
        return ! myCType->is( mask, ch );
    }
};

typedef IsNot<std::ctype_base::space> IsNotSpace;

static std::string trim( std::string const& original )
{
    std::string::const_iterator right = std::find_if( original.rbegin(), original.rend(), IsNotSpace() ).base();
    std::string::const_iterator left = std::find_if(original.begin(), right, IsNotSpace() );
    return std::string( left, right );
}

static string time_to_string(const times_t& t)
{
    ostringstream s;
    for (int i = 0; i + 1 < t.size(); ++i) s << t[i] << "-";
    s << t[t.size() - 1];
    return s.str();
}
