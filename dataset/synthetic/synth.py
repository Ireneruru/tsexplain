import random 
import numpy as np
import pandas as pd
from plotnine import * 
import sys

data_ids = list(range(10, 20))
snrs = [20, 25, 30, 35, 40, 45, 50, 100]
attrs = ['A', 'B', 'C']#, 'D']#, 'F', 'G', 'H', 'I', 'J']
min_t = 0
max_t = 99
max_seg = 3 # maximum predicate segment number
min_points_dist = 5

min_y_init = 0
max_y_init = 0
# y + slope * dx
min_slope = 0.3
max_slope = 10
# y + p * dx^2 + q * dx
min_p = 0.5
max_p = 10
min_q = 0
max_q = 10
# a(x - b)^2 + c
min_a = 2
max_a = 15


def gen_linear_trendline(cur_seg_points):
    n = random.randint(1, max_seg)

    points = set([min_t, max_t])
    
    while(len(points) < n + 1):
        new_point = random.randint(min_t, max_t)
        if any(map(lambda p: new_point in range(p - min_points_dist, p + min_points_dist + 1), points | cur_seg_points)):
           continue
        points.add(new_point)

    points = sorted(list(points))
    
    y1 = random.uniform(min_y_init, max_y_init)
    y, sl = [], []

    sgn = 1
    for i in range(n):
        slope = sgn * random.uniform(min_slope, max_slope)
        y2 = y1 + slope * (points[i+1] - points[i])
        sgn *= -1
        for ii in  range(points[i], points[i+1]):
            vy = y1 + slope * (ii - points[i])
            y.append(vy)
            sl.append(abs(y2-y1))
        y1 = y2
    y.append(y2)
    sl.append(slope)
    return y, sl, points

def gen_poly_trendline(cur_seg_points):
    n = random.randint(1, max_seg)

    points = set([min_t, max_t])
    
    while(len(points) < n + 1):
        new_point = random.randint(min_t, max_t)
        if any(map(lambda p: new_point in range(p - min_points_dist, p + min_points_dist + 1), points | cur_seg_points)):
           continue
        points.add(new_point)

    points = sorted(list(points))
    
    y1 = random.uniform(min_y_init, max_y_init)
    y, sl = [], []

    sgn = 1
    for i in range(n):
        p = sgn * random.uniform(min_p, max_p)
        q = sgn * random.uniform(min_q, max_q)
        y2 = y1 + p * (points[i+1] - points[i]) ** 2 + q * (points[i+1] - points[i])
        sgn *= -1
        for ii in  range(points[i], points[i+1]):
            vy = y1 + p * (ii - points[i]) ** 2 + q * (ii - points[i])
            y.append(vy)
            sl.append(abs(y2-y1))#2 * p * (ii - points[i]) + q)
        y1 = y2
    y.append(y2)
    sl.append(2 * (points[n] - points[n - 1]) + 1)
    return y, sl, points
    
 
def gen_poly_trendline2(cur_seg_points):
# a(x - b)^2 + c
    n = random.randint(1, max_seg)

    points = set([min_t, max_t])
    
    while(len(points) < n + 1):
        new_point = random.randint(min_t, max_t)
        if any(map(lambda p: new_point in range(p - min_points_dist, p + min_points_dist + 1), points | cur_seg_points)):
           continue
        points.add(new_point)

    points = sorted(list(points))
    
    y1 = random.uniform(min_y_init, max_y_init)
    y, sl = [], []

    sgn = 1
    for i in range(n):
        a =  sgn * random.uniform(min_a, max_a)
        b = random.uniform(points[i]-20, points[i])
        c = y1 - a*(points[i]-b)**2 
        y2 = a* (points[i+1] - b ) **2 + c 
        sgn *= -1
        for ii in  range(points[i], points[i+1]):
            vy = a* ( ii - b ) **2 + c   
            y.append(vy)
        y1 = y2
    y.append(y2)
    sl = []
    return y,sl,  points
   

#for method, get_line in [("linear", gen_linear_trendline), ("poly", gen_poly_trendline2)]:
for method, get_line in [("linear", gen_linear_trendline)]:
#for method, get_line in [("poly", gen_poly_trendline2)]:
    for d in data_ids:
        data = {}
        cur_seg_points = set()
        for a in attrs:
            y, sl, points = get_line(cur_seg_points)
            cur_seg_points |= set(points)
            data[a] = {'y': y, 'sl': sl}
        truth = sorted(list(cur_seg_points))
        f = open(f"/Users/yiru/Project/TSExplain/dataset/syn_10_1/{method}-{d}-meta.txt", "w")
        f.write(repr(truth) + "\n")
        f.write(repr(data))
        f.close()
        for snr in snrs:
            df = {'x': [], 'y': [], 'animal': []}
            for a in data:
                y, sl = data[a]['y'], data[a]['sl']
                noise = np.random.normal(0, 1, len(y))
                Es = np.square(np.array(y)).mean()
                En = np.square(noise).mean()
                alpha = (Es/((10**(snr/10.))*En))**0.5
                for x in range(min_t, max_t + 1):
                    df['x'].append(x)
                    df['animal'].append(a)
                df['y'] += list(np.array(y) + alpha * noise)
            df = pd.DataFrame(data=df)
            df["dummy"] = "dummy"
            df.to_csv(f"/Users/yiru/Project/TSExplain/dataset/syn_10_1/{method}-{snr}-{d}.tsv", index=False, header=True, sep="\t")

            agg = df.groupby('x').sum().reset_index()
            agg['animal'] = 'all'

            df = df.append(agg)
            plot = ggplot(df, aes(x='x', y='y', color = 'animal')) + geom_line()

            print(df)
            plot.save(f"/Users/yiru/Project/TSExplain/dataset/syn_10_1/{method}-{snr}-{d}-plot.pdf")



