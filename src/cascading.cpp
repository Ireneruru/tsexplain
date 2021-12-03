#include "tsexplain.h"

/*
 * Build the cascading tree structure
 * (1) extract predicate refine relation, e.g. (A=? -> A=1, A=2) (A=1 -> A'=x, A'=y)
 *      - children[(A=1)] = [(A'=x),(A'=y)]
 * (2) construct cascading tree, including
 *     - node_list[idx] = node
 *     - node_idx[node] = idx
 *     - root: index of root
 *     - node_children[(node_idx, dim)]:  node's children indices on dimension `dim`
 *     - node_parent[(node_idx, dim)] = par_node_idx
 *     - to_pred[node_idx] = pred_idx
 *     - to_node[pred_idx] = node_idx
 * Auxiliary structure:
 *   - children_set: maintain saved children
 *   - node_set: maintain constructed nodes
 */

typedef int node_idx_t;
typedef int dimension_t;
typedef int pred_idx_t;

unordered_map<pred_t, vector<pred_t>, pair_ii_hash> children;

int DIM; // dimension(node.length())
int topK;
inline int _fidx(int inode, int j) { return inode * (topK + 1) + j; }
inline int _sidx(int inode, int i) { return inode * DIM + i; }

vector<predicate_t> node_list;  // idx = node_idx
unordered_map<predicate_t, int, predicate_hash> node_idx;  // idx = node_idx
node_idx_t root;
vector<vector<node_idx_t>> node_children; // idx = node_idx * DIM + dim
vector<node_idx_t> node_parent; // idx = node_idx * DIM + dim
vector<pred_idx_t> to_pred;  // idx = node_idx
vector<node_idx_t> to_node;  // idx = pred_idx

// Auxiliary
unordered_map<pair<predicate_t, dimension_t>, vector<predicate_t>, predicate_int_hash> __node_children;
unordered_map<pair<predicate_t, dimension_t>, predicate_t, predicate_int_hash> __node_parent;
predset_t node_set;

bool build_full_nodes(predicate_t node, int len, int pred_len) {
    if (node_set.count(node) > 0) return true;
    node_set.insert(node);

    // stop if node represents an invalid predicate and all its children must also be invalid
    auto pred = predicate_t();
    for (const auto& p : node) {
        if (p.second != 0) {
            pred.push_back(p);
        }
    }
    if (!pred.empty() && pred_idx.count(pred) == 0) return false;

    // node = (A=?, B=x)
    // for each dim i (e.g. 0)
    // substitute node[i] with its children
    // e.g. i = 0, children[(A=?)] = [(A=1), (A=2)]
    //         node_children[(node, 0)] = [(A=1, B=x), (A=2, B=x)]
    for (int i = 0; i < node.size(); ++i) {
        auto node_i = make_pair(node, i);
        __node_children[node_i] = vector<predicate_t>();
        const auto &p = node[i];
        if ((p.second != 0 || len < pred_len) && children.count(p) > 0 && !children[p].empty()) {
            for (const auto &ch : children[p]) {
                predicate_t node0 = node;
                node0[i] = ch;
                bool valid_node;
                if (p.second == 0) {
                    valid_node = build_full_nodes(node0, len + 1, pred_len);
                } else {
                    valid_node = build_full_nodes(node0, len, pred_len);
                }
                if (valid_node) {
                    __node_children[node_i].push_back(node0);
                    __node_parent[make_pair(node0, i)] = node;
                }
            }
        }
    }
    node_idx[node] = node_list.size();
    node_list.push_back(node);
    return true;
}

void build_full_tree(const Spec& spec)
{
    predicate_t root_node;
    node_list.clear();
    node_set.clear();

    // build root_node = (A=?, B=?, C=?)
    for (const auto& hier : spec.feature_hier) {
        root_node.push_back(make_pair(strings[hier[0]], 0));
    }

    for (const auto& pc : children_set) {
        children[pc.first] = vector<pred_t>(pc.second.begin(), pc.second.end());
    }
    build_full_nodes(root_node, 0, spec.pred_len);
    root = node_idx[root_node];

    // node/pred conversion
    to_pred.resize(node_list.size(), 0);
    to_node.clear();
    to_node.resize(predicates.size(), 0);
    for (const auto &node : node_list) {
        auto pred = predicate_t();
        for (const auto &p : node) {
            if (p.second != 0) {
                pred.push_back(p);
            }
        }
        if (pred_idx.count(pred) > 0) {
            to_pred[node_idx[node]] = pred_idx[pred];
            to_node[pred_idx[pred]] = node_idx[node];
        }
        else {
            to_pred[node_idx[node]] = -1;
        }
    }

    // build index version of node_children/parent
    for (int n = 0; n < node_list.size(); ++n) {
        const auto &node = node_list[n];
        for (int i = 0; i < DIM; ++i) {
            auto node_i = make_pair(node, i);
            node_children.emplace_back();
            for (const auto &c : __node_children[node_i]) {
                node_children[_sidx(n, i)].push_back(node_idx[c]);
            }
            if (__node_parent.count(node_i)) {
                node_parent.push_back(node_idx[__node_parent[node_i]]);
            }
            else {
                node_parent.push_back(-1);
            }
        }
    }
}


/*
 * Construct subtree from full tree, given a list of predicates
 *     - sub_node_list: index list
 *     - is_leaf[node_idx]: a node is leaf or not
 *     - all_leaf[(node_idx, dim)]: node's children on dimension `dim` are all leaf node or not
 *     - sub_node_children[(node_idx, dim)]:  node's children indices on dimension `dim`
 *     - valid[node_idx]: whether a node can be selected as predicate
 *     - active[node_idx]: whether a node is in the subtree
 */
vector<node_idx_t> sub_node_list;
vector<bool> is_leaf; // idx = node_idx
vector<bool> all_leaf;  // idx = node_idx * DIM + dim
vector<vector<node_idx_t>> sub_node_children; // idx = node_idx * DIM + dim
vector<bool> active, valid; // idx = node_idx
vector<bool> inqueue;

// Auxiliary
vector<node_idx_t> queue;

void build_sub_tree(const vector<pred_idx_t> &pred_ids, const Spec &spec)
{
    queue.resize(node_list.size());
    active.clear();
    valid.clear();
    inqueue.clear();
    active.resize(node_list.size(), false);
    valid.resize(node_list.size(), false);
    inqueue.resize(node_list.size(), false);
    is_leaf.resize(node_list.size());
    all_leaf.resize(node_list.size() * DIM);
    sub_node_children.resize(node_list.size() * DIM);

    int head = 0, tail = -1;

    // insert selected predicates into queue
    for (auto pid : pred_ids) {
        int nid = to_node[pid];
        queue[++tail] = nid;
        active[nid] = true;
        valid[nid] = true;
        inqueue[nid] = true;
    }

    // insert all active nodes' parents into queue
    for ( ; head <= tail; ++head) {
        int nid = queue[head];
        for (int i = 0; i < DIM; ++i) {
            int par = node_parent[_sidx(nid, i)];
            if (par != -1 && !inqueue[par]) {
                queue[++tail] = par;
                active[par] = true;
                inqueue[par] = true;
            }
        }
    }

    sub_node_list.clear();
    for (int n = 0; n < node_list.size(); ++n) {
        if (active[n]) {
            sub_node_list.push_back(n);
            is_leaf[n] = true;
            for (int i = 0; i < DIM; ++i) {
                sub_node_children[_sidx(n, i)].clear();
                all_leaf[_sidx(n, i)] = true;
                for (auto c : node_children[_sidx(n, i)]) {
                    if (active[c]) {
                        sub_node_children[_sidx(n, i)].push_back(c);
                        is_leaf[n] = false;
                        if (!is_leaf[c]) all_leaf[_sidx(n, i)] = false;
                    }
                }
            }
        }
    }
}

struct PredLess
{
    vector<score_t>& pred_sc;

    PredLess(int st, int ed) : pred_sc(pred_scores[time_range_idx(make_pair(st, ed))]) {}

    bool operator()(const int& p1, const int& p2)
    {
        return pred_sc[p1].first > pred_sc[p2].first;
    }
};


struct NodeLess
{
    vector<score_t>& pred_sc;

    NodeLess(int st, int ed) : pred_sc(pred_scores[time_range_idx(make_pair(st, ed))]) {}

    bool operator()(const node_idx_t & n1, const node_idx_t & n2)
    {
        return pred_sc[to_pred[n1]].first > pred_sc[to_pred[n2]].first;
    }
};

static vector<double> F;
static vector<int> G;
static vector<vector<double>> S;
static vector<vector<int>> R;

void get_dp_result(node_idx_t node, int j, vector<int>& result)
{
    if (j == 0) return;
    int i = G[_fidx(node, j)];
    if (i == -1) {
        result.push_back(to_pred[node]);
    }
    else {
        const auto& chs = sub_node_children[_sidx(node, i)];
        int n = chs.size() - 1;
        while (n >= 0) {
            const auto& node0 = chs[n];
            int m = R[_sidx(node, i)][_fidx(n, j)];
            if (m == -1) {
                for (int k = 0; k < j; ++k) {
                    result.push_back(to_pred[sub_node_children[_sidx(node, i)][k]]);
                    n = -1;
                }
            }
            else {
                get_dp_result(node0, j - m, result);
                j = m;
                n--;
            }
        }
    }
}

vector<int> cascading_dp(int st, int ed, const vector<pred_idx_t>& pred_ids, const Spec& spec)
{
    build_sub_tree(pred_ids, spec);

    auto time_r = make_pair(st, ed);
    int time_r_idx = time_range_idx(time_r);

    F.resize(node_list.size() * (topK + 1));
    G.resize(node_list.size() * (topK + 1));
    R.resize(node_list.size() * DIM);
    S.resize(node_list.size() * DIM);

    for (int n = 0; n < sub_node_list.size(); ++n) {
        int nd = sub_node_list[n];
        /*
         * update S, R
         */
        if (!is_leaf[nd]) {
            for (int i = 0; i < DIM; ++i) {
                auto node_i = _sidx(nd, i);
                auto& chs = sub_node_children[node_i];
                int Nc = chs.size();
                if (Nc > 0) {
                    S[node_i].resize(Nc * (topK + 1));
                    R[node_i].resize(Nc * (topK + 1));
                    auto& SS = S[node_i];
                    auto& RR = R[node_i];
                    if (all_leaf[node_i]) {
                        sort(chs.begin(), chs.end(), NodeLess(st, ed));
                        SS[_fidx(Nc - 1, 0)] = 0;
                        for (int m = 1; m <= topK; ++m) {
                            if (m <= Nc) {
                                SS[_fidx(Nc - 1, m)] = SS[_fidx(Nc - 1, m - 1)] + pred_scores[time_r_idx][to_pred[chs[m - 1]]].first;
                            }
                            else {
                                SS[_fidx(Nc - 1, m)] = -1E30;
                            }
                            RR[_fidx(Nc - 1, m)] = -1;
                        }
                    }
                    else {
                        for (int t = 0; t < Nc; ++t) {
                            const auto& node0 = chs[t];
                            for (int m = 0; m <= topK; ++m) {
                                if (t == 0) {
                                    SS[_fidx(t, m)] = F[_fidx(node0, m)];
                                    RR[_fidx(t, m)] = 0;
                                }
                                else {
                                    SS[_fidx(t, m)] = -1E30;
                                    RR[_fidx(t, m)] = -1;
                                    for (int mm = 0; mm <= m; ++mm) {
                                        double new_val = F[_fidx(node0, m - mm)] + SS[_fidx(t - 1, mm)];
                                        if (new_val > SS[_fidx(t, m)]) {
                                            SS[_fidx(t, m)] = new_val;
                                            RR[_fidx(t, m)] = mm;
                                        };
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        /*
         * update F, G
         */
        F[_fidx(nd, 0)] = 0;
        auto pred = to_pred[nd];
        for (int j = 1; j <= topK; ++j) {
            if (j == 1 && valid[nd]) {
                F[_fidx(nd, j)] = pred_scores[time_r_idx][pred].first;
                G[_fidx(nd, j)] = -1;
            }
            else {
                F[_fidx(nd, j)] = -1E30;
                G[_fidx(nd, j)] = -1;
            }
            if (!is_leaf[nd]) {
                for (int i = 0; i < DIM; ++i) {
                    const auto& chs = sub_node_children[_sidx(nd, i)];
                    int Nc = chs.size();
                    if (Nc > 0) {
                        double new_val = S[_sidx(nd, i)][_fidx(Nc - 1, j)];
                        if (new_val > F[_fidx(nd, j)]) {
                            F[_fidx(nd, j)] = new_val;
                            G[_fidx(nd, j)] = i;
                        }
                    }
                }
            }
        }
    }
    int bestK = 1;
    for (int i = 1; i <= topK; ++i) {
        if (F[_fidx(root, i)] > F[_fidx(root, bestK)]) {
            bestK = i;
        }
    }
    bestK = topK;

    vector<int> explain;
    get_dp_result(root, bestK, explain);
    sort(explain.begin(), explain.end(), PredLess(st, ed));
    return explain;
}

vector<int> pred_ids;

void __cascading(int st, int ed, const Spec& spec)
{
    vector<int> explain;
    if (spec.cascading_opt >= 1) {
        // guess optimization
        sort(pred_ids.begin(), pred_ids.end(), PredLess(st, ed));
        int NtopK = 10 * topK;
        bool is_best = false;
        while (NtopK < pred_ids.size()) {
            auto sub_preds = vector<int>(pred_ids.begin(), pred_ids.begin() + NtopK);
            explain = cascading_dp(st, ed, sub_preds, spec);
            double next_pred_score = pred_scores[time_range_idx(make_pair(st, ed))][pred_ids[NtopK]].first;
            is_best = true;
            for (int p = 1; p <= topK; ++p) {
                for (int q = 1; q < p; ++q) {
                    if (F[_fidx(root, p)] < F[_fidx(root, q)] + (p - q) * next_pred_score) {
                        is_best = false;
                        goto verify_end;
                    }
                }
            }
            verify_end:
            if (is_best) break;
            NtopK *= 2;
        }
        if (!is_best) {
            explain = cascading_dp(st, ed, pred_ids, spec);
        }
    } else {
        explain = cascading_dp(st, ed, pred_ids, spec);
    }
    explanation[time_range_idx(make_pair(st, ed))] = explain;
}

void cascading(int first, int last, const Spec& spec)
{
    auto start = CurrentTimeMillis();

    topK = spec.topK;
    DIM = spec.feature_hier.size();

    if (explanation.empty()) {
        build_full_tree(spec);
        for (int i = 0; i < predicates.size(); ++i) {
            if (to_pred[to_node[i]] == i) pred_ids.push_back(i);
        }
        explanation.resize(T * T);
        explanation_computed.resize(T * T, false);
    }
    if (phase1) {
        for (int Len = 1; Len < spec.try_seg_len; ++Len) {
            for (int st = first; st <= last-Len; ++st) {
                int ed = st + Len;
                __cascading(st, ed, spec);
                explanation_computed[time_range_idx(make_pair(st, ed))] = true;
            }
        }
    }
    else {
        for (int ii = 0; ii < valid_time_idx.size(); ++ii) {
            for (int jj = ii + 1; jj < valid_time_idx.size(); ++jj) {
                int st = valid_time_idx[ii];
                int ed = valid_time_idx[jj];
                if (explanation_computed[time_range_idx(make_pair(st, ed))]) continue;
                __cascading(st, ed, spec);
                explanation_computed[time_range_idx(make_pair(st, ed))] = true;
            }
        }
    }

    auto end = CurrentTimeMillis();
    cascading_time += double(end - start) / 1000;
}
