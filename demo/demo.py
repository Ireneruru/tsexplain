import streamlit as st
import time
import numpy as np
import pandas as pd
import datetime as dt
import os
import sys
import inspect
import re
import pickle
import json
import matplotlib.pyplot as plt
import matplotlib as mpl
from find_best_k import choose_k

mpl.rcParams['lines.linewidth'] = 5


currentdir = os.path.dirname(os.path.abspath(
    inspect.getfile(inspect.currentframe())))
parentdir = os.path.dirname(currentdir)
sys.path.insert(0, parentdir)

#st.set_option('deprecation.showPyplotGlobalUse', False)

@st.cache
def get_COVID_data():
    data = pd.read_csv("../dataset/covid.tsv", sep = '\t')
    return data

@st.cache
def get_sp500_data():
    data = pd.read_csv("../dataset/sp500.tsv", sep = '\t')
    return data

@st.cache
def get_liquor_data():
    data = pickle.load(open("../dataset/liquor.tsv", "rb"))
    return data


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
    f.write(f"start_number: {config['start_number']}" + "\n")


datasets = ['COVID-19',  'sp500']
selected_dataset = st.sidebar.selectbox("Select a Dataset", datasets)
if selected_dataset == 'COVID-19':
    data = get_COVID_data()
elif selected_dataset == 'liquor':
    data = get_liquor_data()
else:
    data = get_sp500_data()
attributes = tuple(data.keys())
print(attributes)
x_candidates = ('week_number', 'month', 'date')  # 'date',
y_candidates = ( 'daily-confirmed-cases', 'total-confirmed-cases',  'sale (dollars)', "freefloatcap")  # 'deaths',
legend_candidates = ('state', 'county', 'Bottle Volume (ml)', 'County', 'Category Name',
                     'Vendor Name', 'Bottle Volume (ml)', 'Bottles Sold', "category", "subcategory", "symbol")  # 'county',

# user select x, y, aggregate, legend, explain features
x_axis = st.sidebar.selectbox(
    'X-axis', tuple(x for x in attributes if x.lower() in x_candidates))
if x_axis == 'date': x_axis = 'index'

# data[y].dtype == np.dtype('int64') or data[y].dtype == np.dtype('float64')
if selected_dataset == 'COVID-19':
    y_axis = st.sidebar.selectbox('Y-axis',( 'daily-confirmed-cases', 'total-confirmed-cases'))   
else:
    y_axis = st.sidebar.selectbox('Y-axis',tuple(y for y in attributes if y.lower() in y_candidates))



smooth = st.sidebar.slider('Moving Average Window Size', 0, 10, 7)
agg_func = st.sidebar.selectbox(
    'Aggregate Function on Y-axis', ('sum', 'mean'))
legend = st.sidebar.selectbox(
    'Select Legend', (' ',) + tuple(l for l in attributes if l in legend_candidates), index=0)
groupby_dimensions = [x_axis]
if legend != ' ':
    groupby_dimensions.append(legend)

# explain_features

if selected_dataset == "Covid" and y_axis == "total_confirmed":
    groupby_dimensions_with_date = groupby_dimensions + ["date"]
    overall_trend_data = data.groupby(groupby_dimensions_with_date).aggregate(
        {y_axis: agg_func}).reset_index()  # {x_axis: data[x_axis], y_axis: data[y_axis]}
    overall_trend_data = overall_trend_data.groupby(groupby_dimensions).aggregate(
        {y_axis: "max"}).reset_index()  # {x_axis: data[x_axis], y_axis: data[y_axis]}
elif selected_dataset == "sp500" and y_axis == "FreeFloatCap":
    groupby_dimensions_with_date = groupby_dimensions + ["date"]
    overall_trend_data = data.groupby(groupby_dimensions_with_date).aggregate(
        {y_axis: agg_func}).reset_index()  # {x_axis: data[x_axis], y_axis: data[y_axis]}
    overall_trend_data = overall_trend_data.groupby(groupby_dimensions).aggregate(
        {y_axis: "mean"}).reset_index()  # {x_axis: data[x_axis], y_axis: data[y_axis]}
else:
    overall_trend_data = data.groupby(groupby_dimensions).aggregate(
        {y_axis: agg_func}).reset_index()  # {x_axis: data[x_axis], y_axis: data[y_axis]}
axis_type = {'index': 'temporal', 'week': 'temporal', 'week_number': 'nominal', 'month': 'nominal', 'date': 'temporal'}
if_sidebar_explore = st.sidebar.button('Explore TSExplain?')

if x_axis == 'index':
    x_range = []
    x_range.append( st.sidebar.date_input("Start date", dt.datetime(2020, 1, 1) + dt.timedelta(days=min(data[x_axis]))))
    x_range.append( st.sidebar.date_input("End date", dt.datetime(2020, 1, 1) + dt.timedelta(days=min(max(data[x_axis]), 365))))
    #x_range = st.sidebar.slider("Select a Range for TSExplain", dt.datetime(2020, 1, 1) + dt.timedelta(days=min(data[x_axis])),
    #                            dt.datetime(2020, 1, 1) + dt.timedelta(days=min(max(data[x_axis]), 365)),
    #                            value=(dt.datetime(2020, 1, 1) + dt.timedelta(days=min(data[x_axis])),
    #                                   dt.datetime(2020, 1, 1) + dt.timedelta(days=min(max(data[x_axis]), 365))), format="MM/DD")
    x_range = ((x_range[0] - dt.date(2020, 1, 1)).days, (x_range[1] - dt.date(2020, 1, 1)).days)

else:
    x_range = st.sidebar.slider("Select a Range for TSExplain", min(data[x_axis]), min(12, max(data[x_axis])) if x_axis == 'month' else max(data[x_axis]),
        value=(min(data[x_axis]), min(12, max(data[x_axis])) if x_axis == 'month' else max(data[x_axis])))

explain_features = st.sidebar.multiselect('Select Explain-by Features', tuple(
    l for l in attributes if l in legend_candidates))

selected_trend_data = overall_trend_data[(overall_trend_data[x_axis] >= x_range[0]) & (
    overall_trend_data[x_axis] <= x_range[1])]
if x_axis == "index":
    selected_trend_data["date"] = selected_trend_data["index"].apply(lambda d: dt.datetime(2020, 1, 1) + dt.timedelta(days=d))
if smooth > 1:
    selected_trend_data = selected_trend_data.sort_values(x_axis)
    if legend == ' ':
        selected_trend_data[y_axis] = selected_trend_data[y_axis].transform(lambda x: x.rolling(smooth*2-1, 1, True).mean())
    else:
        selected_trend_data[y_axis] = selected_trend_data.groupby(legend)[y_axis].transform(lambda x: x.rolling(smooth*2-1, 1, True).mean())


metric_types = ('Absolute-Change', 'Slope Change')
vis_type = st.sidebar.selectbox("Select a Difference Metric", metric_types)
# 0 means tool selected by user
SEG_NUM = st.sidebar.slider('Select # of Segments (Optional)', 0, 10, 0)
# 0 means tool selected by user
TOPK = st.sidebar.slider('TOP-m Explanations?', 0, 5, 3)


selected_trend = st.vega_lite_chart(selected_trend_data, {
    "$schema": "https://vega.github.io/schema/vega-lite/v4.json",
    "vconcat": [
        {
            "width": 500,
            "height": 300,
            "mark": {'type': 'line', "point": True, "tooltip": True},
            "selection": {
                "data": {
                    "type": "multi", "fields": [] if legend == ' ' else [legend], "bind": "legend"
                }
            },
            "encoding": {
                "x": {
                    "field": "date" if x_axis == "index" else x_axis,
                    "type": axis_type[x_axis],
                    "scale": {"domain": {"selection": "brush"}},
                    "axis": {"title": "", "labelFontSize": 16}
                },
                "y": {"field": y_axis, "type": "quantitative", "axis": {"labelFontSize": 16, "titleFontSize": 16}},
                "color": {"field": None if legend == ' ' else legend, "type": "nominal", 'legend': {'columns': 1, 'symbolLimit': 15, 'labelFontSize': 16}},
                "opacity": {
                    "condition": {"selection": "data", "value": 1},
                    "value": 0.2
                }
            }
        },
        {
            "width": 500,
            "height": 100,
            "mark": {'type': 'line', "point": True, "tooltip": True},
            "selection": {"brush": {"type": "interval", "encodings": ["x"]}},
            "encoding": {
                "x": {
                    "field": "date" if x_axis == "index" else x_axis,
                    "type": axis_type[x_axis],
                    "axis": {"labelFontSize": 16, "titleFontSize": 16}
                },
                "y": {
                    "field": y_axis,
                    "type": "quantitative",
                    "axis": {"tickCount": 3, "grid": False, "labelFontSize": 16, "titleFontSize": 16}
                },
                "color": {"field": None if legend == ' ' else legend, "type": "nominal"},
            }
        }
    ]
})

############################################
# Explanation 
############################################



if x_axis == 'index':
    xaxis = "date"
    time_min = (dt.datetime(2020, 1, 1) + dt.timedelta(days=x_range[0])).strftime("%m/%d")
    time_max = (dt.datetime(2020, 1, 1) + dt.timedelta(days=x_range[1])).strftime("%m/%d")
else:
    xaxis = x_axis
    time_min = x_range[0]
    time_max = x_range[1]

if_explore = st.button("What are the key influencers from " +
                       xaxis + " " + str(time_min) + " to " + str(time_max) + "?")

if if_sidebar_explore or if_explore:

    groupby_dimensions = explain_features + [x_axis]
    if selected_dataset == "Covid" and y_axis == "total_confirmed":
        groupby_dimensions_with_date = groupby_dimensions + ["date"]
        overall_trend_data = data.groupby(groupby_dimensions_with_date).aggregate(
            {y_axis: agg_func}).reset_index()  # {x_axis: data[x_axis], y_axis: data[y_axis]}
        overall_trend_data = overall_trend_data.groupby(groupby_dimensions).aggregate(
            {y_axis: "max"}).reset_index()  # {x_axis: data[x_axis], y_axis: data[y_axis]}
    elif selected_dataset == "sp500" and y_axis == "FreeFloatCap":
        groupby_dimensions_with_date = groupby_dimensions + ["date"]
        overall_trend_data = data.groupby(groupby_dimensions_with_date).aggregate(
            {y_axis: agg_func}).reset_index()  # {x_axis: data[x_axis], y_axis: data[y_axis]}
        overall_trend_data = overall_trend_data.groupby(groupby_dimensions).aggregate(
            {y_axis: "mean"}).reset_index()  # {x_axis: data[x_axis], y_axis: data[y_axis]}
    else:
        overall_trend_data = data.groupby(groupby_dimensions).aggregate(
            {y_axis: agg_func}).reset_index()  # {x_axis: data[x_axis], y_axis: data[y_axis]}
    overall_trend_data["dummy"] = "dummy"
    overall_trend_data.to_csv("demo_data.tsv", sep="\t", index=False)

    config = dict(
        dataset = "demo",
        granularity = "day",
        tsv_file = "demo_data.tsv",
        feature_hier = [{'features': explain_features}] if selected_dataset != "liquor" else list([{"features": [e]} for e in explain_features]),
        time_cols = [x_axis],
        val_col = y_axis,
        try_seg_len_ratio=1,
        try_total_len_ratio=1,
        supp_ratio = 0,
        time_min = x_range[0],
        time_max = x_range[1],
        post_process = 0 if y_axis == 'total-confirmed-cases' else smooth,
        explain_time_min = x_range[0],
        explain_time_max = x_range[1],
        metric = "MChangeAbs",
        topK = TOPK,
        pred_len = -1,
        sim = "mutual",
        seg_number = SEG_NUM if SEG_NUM > 0 else 20,
        min_len = 2,
        min_dist = 0,
        cascading_opt=1,
        start_number=0
        )

    config_to_spec(config, "demo.spec")

    os.system("../build/tsexplainC demo.spec _")

    def to_month_day(s):
        from datetime import datetime as dt
        week = int(s)
        d = dt.fromisocalendar(2020, week, 1)
        return f"{d.year}/{d.month}/{d.day}"
    def index_to_month_day(s):
        import datetime as dt
        days = int(s)
        d = dt.datetime(2020, 1, 1) + dt.timedelta(days=days)
        return f"{d.year}/{d.month}/{d.day}"

    if SEG_NUM == 0:
        out = json.load(open("_output.json"))
        best_k = choose_k(out)
        print("choose k = ", best_k)
        res = out['result'][best_k - 1]
    else:
        res = json.load(open("_output.json"))['result'][-1]
    data_t = res["timeline"]
    if x_axis == "week_number":
        data_t = list(map(to_month_day, data_t))
    elif x_axis == "index":
        data_t = list(map(index_to_month_day, data_t))

    trend_full = res["trend_full"]
    
#    {
#        "timeline": [TIME],
#        "trend_full": [xxx],
#        "segments": [
#            { "begin": (id, TIME),
#              "end: (id, TIME),
#              "sim": xxx,
#              "explain": [
#                    { "predicate": [["A", "X"], ],
#                      "direction": "+/-",
#                      "trend_sub: [xxx]
#                    }
#              ]
#            }
#        }
#    }
    # print segmentation info
    data = {'Segment': [], 'Variance': []}
    for i in range(TOPK):
        data['Top-' + str(i+1) + ' Explain'] = []
    for seg in res["segments"]:
        data['Segment'].append(data_t[seg["begin"]] + "-" + data_t[seg["end"]])
        data['Pureness'].append(1 - seg["sim"] / (seg["end"] - seg["begin"]))
        for m in range(TOPK):
            expl = seg["explanation"][m]
            pred = ",".join(p[1] for p in expl["predicate"])
            effect = expl["direction"]
            data['Top-' + str(m+1) + ' Explain'].append(pred + effect)
    df = pd.DataFrame(data)
    st.dataframe(df)

    # visualize
    ncols = len(res["segments"])
    x_len_total = 0
    x_len_current = 0
    x_len_segments = []
    multiplier = 5
    for i in range(ncols):
        seg = res["segments"][i]
        start_i, end_i = seg["begin"], seg["end"]
        segment_len = (end_i - start_i) * multiplier
        x_len_segments.append(segment_len)
        x_len_total += segment_len

    fig = plt.figure(figsize=(5*10, 2*10))
    fig0 = plt.figure(figsize=(5*10, 2*10))
    gs = fig.add_gridspec(1, x_len_total)
    gs0 = fig0.add_gridspec(1, x_len_total)

    colors = ['b', 'g', 'r', 'c', 'm', 'y', 'brown',
              'orange', 'olive', 'lime', 'hotpink', 'yellow']
    pred2idx = {}
    y_min = 9999999999999
    y_max = 0
    y0_min = min(res["trend_full"])
    y0_max = max(res["trend_full"])
    for i in range(ncols):
        seg = res["segments"][i]
        start_i, end_i = seg["begin"], seg["end"]
        trend_sub_all = None
        for k in range(TOPK):
            y = np.array(seg["explanation"][k]["trend_sub"])
            y_min = min(y_min, min(y))
            y_max = max(y_max, max(y))
    handles, labels = [], []
    handles0, labels0 = [], []

    for i in range(ncols):
        seg = res["segments"][i]
        start_i, end_i = seg["begin"], seg["end"]

        # plot each segment
        axes = fig.add_subplot(
            gs[0, x_len_current: x_len_current + x_len_segments[i]])
        axes0 = fig0.add_subplot(
            gs[0, x_len_current: x_len_current + x_len_segments[i]])
        x_len_current = x_len_current + x_len_segments[i]
        trend_sub_all = None
        for k in range(TOPK):
            expl = seg["explanation"][k]
            p = " & ".join((p[0] + "=" + p[1]) for p in expl["predicate"])
            if p not in pred2idx:
                pred2idx[p] = len(pred2idx)
            trend_sub = np.array(expl["trend_sub"])
            if trend_sub_all is None:
                trend_sub_all = trend_sub
            else:
                trend_sub_all = trend_sub_all + trend_sub
            axes.plot(data_t[start_i: end_i + 1], trend_sub,
                    color=colors[pred2idx[p] % len(colors)], label=p)
        # bbox_to_anchor=(0.5, 1.1, 0.5, 0.)
        axes.set_ylim([y_min, y_max*1.1])
        hs, ls = axes.get_legend_handles_labels()
        handles += hs
        labels += ls

        axes.yaxis.offsetText.set_fontsize(50)
        axes.xaxis.offsetText.set_fontsize(50)
        #axes.legend(loc='upper center', prop={'size': 60})
        if i != 0:
            axes.get_yaxis().set_ticks([])
            axes0.get_yaxis().set_ticks([])
        axes.get_xaxis().set_ticks([0, end_i - start_i] if i == ncols - 1 else [0])
        axes0.get_xaxis().set_ticks([0, end_i - start_i] if i == ncols - 1 else [0])
        axes.tick_params(labelsize=50, labelrotation=45)
        axes0.tick_params(labelsize=50, labelrotation=45)
        axes0.plot(data_t[start_i: end_i + 1], trend_full[start_i: end_i+1], 
                    color='b', label='With All Data', linewidth = 5)
        axes0.plot(data_t[start_i: end_i + 1], trend_sub_all,
                    color='r', label='With Top Influencers Only', linewidth = 5)
        yy_min = min(trend_sub_all) 
        axes0.set_ylim([yy_min, y0_max*1.1])
        if i == 0:
            hs0, ls0 = axes0.get_legend_handles_labels()
            handles0 += hs0
            labels0 += ls0

        axes0.yaxis.offsetText.set_fontsize(50)
        axes0.xaxis.offsetText.set_fontsize(50)
    fig.legend(handles, labels, loc='upper center', fontsize=30, ncol=ncols)
    fig0.legend(handles0, labels0, loc='upper center', fontsize=30)
    st.pyplot(fig)
    st.pyplot(fig0)
