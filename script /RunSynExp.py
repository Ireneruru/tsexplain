import sqlite3 
import os
import pandas as pd
import json
from sqlalchemy import *


tsexplain_path = "/Users/yiru/Project/TSExplain/C/cmake-build-debug/tsexplainC"
dataset_path = "/Users/yiru/Project/TSExplain/dataset"

def gen_spec(input_tsv, max_t, sim, seg_number, output_path, kurt):
    f = open(output_path, 'w')
    f.write(f"tsv_file: {input_tsv}" + "\n")
    f.write(f"feature_hier_number: 1" + "\n")
    f.write(f"feature_number: 1" + "\n")
    f.write("animal\n")
    f.write(f"time_col_number: 1" + "\n")
    f.write("x\n")
    f.write(f"rollup_type: none" + "\n")
    f.write(f"val_col: y\n")
    f.write(f"supp_ratio: 0" + "\n")
    f.write(f"time_min: 0" + "\n")
    f.write(f"time_max: {max_t}" + "\n")
    f.write(f"post_process: none" + "\n")
    f.write(f"explain_time_min: 0" + "\n")
    f.write(f"explain_time_max: {max_t}" + "\n")
    f.write(f"metric: MChangeAbs" + "\n")
    f.write(f"topK: 3" + "\n")
    f.write(f"pred_len: -1" + "\n")
    f.write(f"sim: {sim}" + "\n")
    f.write(f"seg_number: {seg_number}" + "\n")
    f.write(f"min_len: 2" + "\n")
    f.write(f"min_dist: 0" + "\n")
    f.write(f"kurt_threshold: {kurt}" + "\n")

    f.close()

def run_tsexplain(method, snr, data_id, sim, kurt, fixK=True, K=None):
    if method in ['linear', 'poly']:
        tsv = dataset_path + f"/syn_10_1/{method}-{snr}-{data_id}.tsv"
        meta = dataset_path + f"/syn_10_1/{method}-{data_id}-meta.txt"
    elif method in ['trend', 'seasonality']:
        tsv = dataset_path + f"/syn-seasonality_10_1/{method}_decomposition-{snr}-{data_id}.tsv"
        meta = dataset_path + f"/syn-seasonality_10_1/{method}-{data_id}-meta.txt"
    else:
        return None
    truth = eval(open(meta).readline())
    min_t = truth[0]
    max_t = truth[-1]
    seg_number = K or len(truth) - 1
    
    gen_spec(tsv, max_t, sim, seg_number if fixK else -seg_number*2, "./tmp.spec", kurt)
    os.system(tsexplain_path + " tmp.spec" )
    output = json.load(open("output.json"))
    segments = open("segments.txt").readlines()

    res = {}
    res['vis_output'] = output['measurement']['vis_segment'] + [max_t]
    res['seg_number'] = seg_number
    res['asap'] = output['measurement']['mavg_window']
    res['score'] = output['measurement']['score']
    res['vis_score'] = output['measurement']['vis_score']
    res['groundtruth'] = truth
    res['output'] = set()
    for s in output['result'][-1]['segments']:
        res['output'].add(int(s['begin']))
        res['output'].add(int(s['end']))
    res['output'] = sorted(list(res['output']))
    res['output_seg_number'] = len(res['output']) - 1

    segs = {}
    for i in range(0, len(segments), 5):
        l = segments[i].split("~")
        s, e = int(l[0].strip()[1:-1]), int(l[1].strip()[1:-1])
        sc = float(segments[i+1].split(" ")[1])
        segs[(s, e)] = sc

    res['groundtruth_score'] = 0
    if K is None:
        for i in range(seg_number):
            res['groundtruth_score'] += segs[(truth[i], truth[i+1])]

    os.system("rm output.json segments.txt")

    return res, segs

def expr_calc_distribution(db):
    method = ['linear', 'poly']
    data_ids = list(range(0, 10))
    snrs = [20, 25, 30, 35, 40, 45, 50, 100]
    sim = ['mutual', 'sim1', 'sim2', 'allpair', 'Smutual', 'Ssim1', 'Ssim2', 'Sallpair' ]
    kurts = [1]

    result = {'dataset': [], 'method': [], 'snr': [], 'id': [], 
              'seg_number': [], 'sim': [], 'kurt': [], 'run': [], 'groundtruth': [], 
              'asap': [], 'output': [], 'score': []}

    def sample(res, segs, run, met, snr, data_id, sim, kurt, points=None):
        nonlocal result

        import random

        min_t = res['groundtruth'][0]
        max_t = res['groundtruth'][-1]
        seg_number = res['seg_number']

        if points is None:
            points = set([min_t, max_t])
            while len(points) < seg_number + 1:
                points.add(random.randint(min_t, max_t))
            points = sorted(list(points))
        score = 0
        for i in range(seg_number):
            score += segs[(points[i], points[i+1])]
        
        result['dataset'].append('syn')
        result['method'].append(met)
        result['snr'].append(snr)
        result['id'].append(data_id)
        result['seg_number'].append(seg_number)
        result['sim'].append(sim)
        result['kurt'].append(kurt)
        result['run'].append(run)
        result['groundtruth'].append(" ".join(map(str, res['groundtruth'])))
        result['asap'].append(res['asap'])
        result['output'].append(" ".join(map(str, points)))
        result['score'].append(score)

    for met in method:
        for snr in snrs:
            for did in data_ids:
                for s in sim:
                    for k in kurts:
                        if met == "linear" and snr < 0.09 and k > 0.98: continue
                        print(met, snr, did, s, k)
                        res, segs = run_tsexplain(met, snr, did, s, k)

                        sample(res, segs, 0, met, snr, did, s, k, res['groundtruth'])

                        for i in range(1, 10001):
                            sample(res, segs, i,met, snr, did, s, k)

            pd.DataFrame(result).to_sql("distribution", db, if_exists='append')
            result = {'dataset': [], 'method': [], 'snr': [], 'id': [], 
              'seg_number': [], 'sim': [], 'kurt': [], 'run': [], 'groundtruth': [], 
              'asap': [], 'output': [], 'score': []}


def expr_calc_distance(db):
    #method = ['linear', 'poly', 'seasonality', 'trend']
    method = ['linear']
    #method = ['poly']
    data_ids = list(range(10, 20))
    snrs = [20, 25, 30, 35, 40, 45, 50, 100]
    #sim = ['mutual', 'sim1', 'sim2', 'allpair', 'visual', 'smutual', 'ssim1', 'ssim2', 'sallpair' ]
    sim = ['mutual', 'visual']
    kurts = [1] #, 0.998, 0.996, 0.994, 0.992, 0.99, 0.988, 0.98, 0.97, 0.96]

    result = {'dataset': [], 'method': [], 'snr': [], 'id': [], 
              'seg_number': [], 'sim': [], 'kurt': [], 'groundtruth': [], 
              'asap': [], 'output': [], 'distance': []}

    def editdistance(truth, b):
        truth = sorted(list(truth))
        b = sorted(list(b))
        s = 0
        if len(truth) != len(b):
            print(truth, b)
        for i in range(len(truth)):
            s += abs(truth[i] - b[i])
        return s/float(len(truth))

    for snr in snrs:
        for did in data_ids:
            for s in sim:
                for met in method:
                    for k in kurts:
                        print(met, snr, did, s, k)
                        res, segs = run_tsexplain(met, snr, did, s if s != "visual" else "sim1", k)
                        result['dataset'].append('syn')
                        result['method'].append(met)
                        result['snr'].append(snr)
                        result['id'].append(did)
                        result['seg_number'].append(res['seg_number'])
                        result['sim'].append(s)
                        result['kurt'].append(k)
                        result['groundtruth'].append(" ".join(map(str, res['groundtruth'])))
                        result['asap'].append(res['asap'])
                        result['output'].append(" ".join(map(str, res['output' if s != "visual" else 'vis_output'])))
                        result['distance'].append(editdistance(res['groundtruth'], res['output' if s != "visual" else 'vis_output']))

        pd.DataFrame(result).to_sql("distance", db, if_exists='append')
        result = {'dataset': [], 'method': [], 'snr': [], 'id': [], 
                  'seg_number': [], 'sim': [], 'kurt': [], 'groundtruth': [], 
                  'asap': [], 'output': [], 'distance': []}

def expr_calc_results(db):
    method = ['seasonality', 'trend']
    data_ids = list(range(0, 10))
    snrs = [20, 25, 30, 35, 40, 45, 50, 100]
    sim = ['mutual', 'visual']
    kurts = [1]

    result = {'dataset': [], 'method': [], 'snr': [], 'id': [], 
              'seg_number': [], 'sim': [], 'kurt': [], 'groundtruth': [], 
              'asap': [], 'output': [], 'score': []}

    for snr in snrs:
        for did in data_ids:
            for s in sim:
                for met in method:
                    for k in range(0, 10):
                        print(met, snr, did, s, k)
                        res, segs = run_tsexplain(met, snr, did, s, 1, K=k)

                        result['dataset'].append('syn_10_1')
                        result['method'].append(met+'_10_1')
                        result['snr'].append(snr)
                        result['id'].append(did)
                        result['seg_number'].append(res['seg_number'])
                        result['sim'].append(s)
                        result['kurt'].append(1)
                        result['groundtruth'].append(" ".join(map(str, res['groundtruth'])))
                        result['asap'].append(res['asap'])
                        result['output'].append(" ".join(map(str, res['output'])))
                        result['score'].append(res['score'])

        pd.DataFrame(result).to_sql("results", db, if_exists='append')
        result = {'dataset': [], 'method': [], 'snr': [], 'id': [], 
                  'seg_number': [], 'sim': [], 'kurt': [], 'groundtruth': [],
                  'asap': [], 'output': [], 'score': []}

if __name__ == "__main__":

    db = sqlite3.connect("../expressiveness/res1001.db")

    #expr_calc_distribution(db)
    #expr_calc_results(db)
    expr_calc_distance(db)
