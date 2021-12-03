import yaml
import sqlite3
import pandas as pd
import os
import json
from IPython import embed
from plot import plot_output

def config_to_spec(config, output_path):
    f = open(output_path, 'w')
    f.write(f"tsv_file: {config['tsv_file']}" + "\n")
    f.write(f"feature_hier_number: {len(config['feature_hier'])}" + "\n")
    for hier in config['feature_hier']:
        f.write(f"feature_number: {len(hier['features'])}" + "\n")
        for d in hier['features']:
            f.write(d + "\n")
    f.write(f"time_col_number: {len(config['time_cols'])}" + "\n")
    for d in config['time_cols']:
        f.write(d + "\n")
    f.write(f"try_seg_len_ratio: {config['try_seg_len_ratio']}" + "\n")
    f.write(f"try_total_len_ratio: {config['try_total_len_ratio']}" + "\n")
    f.write(f"val_col: {config['val_col']}" + "\n")
    f.write(f"supp_ratio: {config['supp_ratio']}" + "\n")
    f.write(f"time_min: {config['time_min']}" + "\n")
    f.write(f"time_max: {config['time_max']}" + "\n")
    f.write(f"post_process: {config['post_process']}" + "\n")
    f.write(f"explain_time_min: {config['explain_time_min']}" + "\n")
    f.write(f"explain_time_max: {config['explain_time_max']}" + "\n")
    f.write(f"metric: {config['metric']}" + "\n")
    f.write(f"topK: {config['topK']}" + "\n")
    f.write(f"pred_len: {config['pred_len']}" + "\n")
    f.write(f"sim: {config['sim']}" + "\n")
    f.write(f"seg_number: {config['seg_number']}" + "\n")
    f.write(f"min_len: {config['min_len']}" + "\n")
    f.write(f"min_dist: {config['min_dist']}" + "\n")
    f.write(f"cascading_opt: {config['cascading_opt']}" + "\n")

def editdistance(truth, b):
    truth = set(map(int, truth))
    b = set(map(int, b))
    truth |= {0,99}
    b |= {0, 99}
    truth = sorted(list(truth))
    b = sorted(list(b))
    s = 0
    if len(truth) != len(b):
        print(truth, b)
    for i in range(len(truth)):
        s += abs(truth[i] - b[i])
    return s/float(len(truth))

def run_normal(config):
    config_to_spec(config, "tmp.spec")
    print("Running", config)
    os.system("../build/tsexplainC tmp.spec > /dev/null 2>&1")
    output = json.load(open("output.json"))
    time_measure = output["measurement"]

    # compute segmentation
    S = []
    timeline = output['result'][-1]['timeline']
    segs = output['result'][-1]['segments'] 
    for s in segs:
        S.append(timeline[s['begin']])
    S.append(timeline[segs[-1]['end']])
    sol = str(S)
        
    db = sqlite3.connect("exp.db")
    try:
        count = pd.read_sql_query("select count(*) from experiment_res", db)
        count = count['count(*)'][0]
    except pd.io.sql.DatabaseError as e:
        count = 0

    res = {
        "dataset": config['dataset'],
        "method": config.get('method', ""),
        "srn": config.get('srn', 0),
        "data_num": config.get('data_num', 0),
        'time_granularity': config['granularity'],
        "row_num": output['measurement']["row_number"],
        'target': config['val_col'],
        'cascading_opt': config['cascading_opt'],
        "x": str(config['time_cols']),
        'post_process': config['post_process'],
        'try_seg_len_ratio': config['try_seg_len_ratio'],
        'try_total_len_ratio': config['try_total_len_ratio'],
        'explain_by': str(list([h["features"] for h in config["feature_hier"]])),
        'explain_time_min': config['explain_time_min'],
        'explain_time_max': config['explain_time_max'],
        'N': output['measurement']['timeline_len'],
        'm': config["topK"],
        'seg_num_K': config['seg_number'],
        'supp_ratio': config['supp_ratio'],
        'pred_len': config['pred_len'],
        '|predicate| after filter': output['measurement']['predicate_num'],
        'sim': config['sim'],
        'read_file_time': output['measurement']['read_file'],
        'compute_cube': output['measurement']['compute_cube'],
        'compute_trendline': output['measurement']['compute_trendline'],
        'compute_metric': output['measurement']['compute_metric'],
        'cascading': output['measurement']['cascading'],
        'compute_sim': output['measurement']['compute_sim'],
        'search': output['measurement']['search'],
        'overall': output['measurement']['overall'],
        'segmentation': sol,
        'sim_score': output['measurement']['score'],
        'vis': ["%04d"%(count)],
        'ground_truth': str(config.get('ground_truth', "")),
        'distance': editdistance(config['ground_truth'], S) if 'ground_truth' in config else 0
    }

    print(res['vis'][0])
    #plot_output([output["result"][-1]], res['vis'][0])
    os.system(f"mv output.json expr_output/{res['vis'][0]}.json")

    data = pd.DataFrame(data=res)
    data.to_sql("experiment_res", db, if_exists="append", index=False)

    db.close()


if __name__ == "__main__":
    input_config = os.sys.argv[1]
    config = yaml.load(open(input_config).read())
    run_normal(config)
