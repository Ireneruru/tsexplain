dataset_base = "/Users/yiru/Project/TSExplain/dataset"
dataset = {
    # "covid": [{
    #     "name": "total",
    #     "gran": "day",
    #     "dataset": f"{dataset_base}/Covid/covid.tsv",
    #     "feature_hier": [["state"]],
    #     "time_cols": ["month", "day"],
    #     "val_col": "total-confirmed-cases",
    #     "post_process": [0],
    #     "try_strategy": [(0.05, 3), (1, 1)],
    #     "time_min": "1 22",
    #     "time_max": "12 31",
    #     "explain_time_min": "1 22",
    #     "explain_time_max": "12 31",
    #     "supp_ratio": [0, 0.001],
    #     "order" : [-1, 2, 3], #the length of the predicate
    #     "seg_number": [20],
    # },
    # {
    #     "name": "daily",
    #     "gran": "day",
    #     "dataset": f"{dataset_base}/Covid/covid.tsv",
    #     "feature_hier": [["state"]],
    #     "time_cols": ["month", "day"],
    #     "val_col": "daily-confirmed-cases",
    #     "post_process": [7],
    #     "try_strategy": [(0.05, 3), (1, 1)],
    #     "time_min": "1 22",
    #     "time_max": "12 31",
    #     "explain_time_min": "1 22",
    #     "explain_time_max": "12 31",
    #     "supp_ratio": [0, 0.001],
    #     "order" : [-1, 2, 3], #the length of the predicate
    #     "seg_number": [20],
    # }],
    # "sp500": [{
    #     "name": "freeCap853",
    #     "gran": "day",
    #     "dataset": f"{dataset_base}/sp500/sp500.tsv",
    #     "feature_hier": [["category", "subcategory", "symbol"]],
    #     "time_cols": ["month", "day"],
    #     "val_col": "freeCap853",
    #     "post_process": [7],
    #     "try_strategy": [(0.05, 3), (1, 1)],
    #     "time_min": "1 1",
    #     "time_max": "12 31",
    #     "explain_time_min": "1 1",
    #     "explain_time_max": "12 31",
    #     "supp_ratio": [0, 0.001],
    #     "order" : [-1, 2, 3], #the length of the predicate
    #     "seg_number": [20],
    # }],
    # "liquor": [{
    #     "name": "sale",
    #     "gran": "day",
    #     "dataset": f"{dataset_base}/liquor/liquor2020a.tsv",
    #     "feature_hier": [["Bottle Volume (ml)"], ["Category Name"], ["Pack"], ["Vendor Name"]],
    #     "time_cols": ["year", "month", "day"],
    #     "val_col": "Sale (Dollars)",
    #     "post_process": [7],
    #     "try_strategy": [(0.05, 3), (1, 1)],
    #     "time_min": "2020 1 1",
    #     "time_max": "2020 6 31",
    #     "explain_time_min": "2020 1 1",
    #     "explain_time_max": "2020 6 31",
    #     "supp_ratio": [0, 0.001],
    #     "order" : [-1, 2, 3], #the length of the predicate
    #     "seg_number": [20],
    # }],
    "liquor": [{
        "name": "bottle",
        "gran": "day",
        "dataset": f"{dataset_base}/liquor/liquor2020a.tsv",
        "feature_hier": [["Bottle Volume (ml)"], ["Category Name"], ["Pack"], ["Vendor Name"]],
        "time_cols": ["year", "month", "day"],
        "val_col": "Bottles Sold",
        "post_process": [7],
        "try_strategy": [(0.05, 3), (1, 1)],
        "time_min": "2020 1 1",
        "time_max": "2020 6 31",
        "explain_time_min": "2020 1 1",
        "explain_time_max": "2020 6 31",
        "supp_ratio": [0, 0.001],
        "order" : [-1, 2, 3], #the length of the predicate
        "seg_number": [20],
    }],
}

for data in dataset:
    for case in dataset[data]:
        for post in case["post_process"]:
            for try_seg_len, try_total_len in case["try_strategy"]:
                for supp in case["supp_ratio"]:
                        for seg_number in case['seg_number']:
                            for ord in case['order']:
                                for cas_opt in [0, 1]:
                                    name = f"{data}-{case['name']}-mavg{post}-{seg_number}-{try_seg_len}-{try_total_len}-{supp}-{ord}-{cas_opt}"
                                    f = open(f"{name}.yaml", "w")
                                    f.write(f"dataset: {data}\n")
                                    f.write(f"granularity: {case['gran']}\n")
                                    f.write(f"tsv_file: {case['dataset']}\n")
                                    f.write(f"feature_hier:\n")
                                    for feats in case["feature_hier"]:
                                        f.write(f"  - features:\n")
                                        for ft in feats:
                                            f.write(f"    - {ft}\n")
                                    f.write(f"time_cols:\n")
                                    for tc in case["time_cols"]:
                                        f.write(f"  - {tc}\n")
                                    f.write(f"val_col: {case['val_col']}\n")
                                    f.write(f"try_seg_len_ratio: {try_seg_len}\n")
                                    f.write(f"try_total_len_ratio: {try_total_len}\n")
                                    f.write(f"supp_ratio: {supp}\n")
                                    f.write(f"time_min: {case['time_min']}\n")
                                    f.write(f"time_max: {case['time_max']}\n")
                                    f.write(f"post_process: {post}\n")
                                    f.write(f"explain_time_min: {case['explain_time_min']}\n")
                                    f.write(f"explain_time_max: {case['explain_time_max']}\n")
                                    f.write(f"metric: MChangeAbs\n")
                                    f.write(f"topK: 3\n")
                                    f.write(f"pred_len: {ord}\n")
                                    f.write(f"sim: mutual\n")
                                    f.write(f"seg_number: {seg_number}\n")
                                    f.write(f"min_len: 2\n")
                                    f.write(f"min_dist: 0\n")
                                    f.write(f"cascading_opt: {cas_opt}\n")
                                    f.close()
