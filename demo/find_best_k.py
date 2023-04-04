import pandas as pd
import matplotlib.pyplot as plt

def collect_segments(output):
    segments = {"seg_number": [], "sim": [], "segments": []}

    for seg in output["result"]:
        seg_number = len(seg["segments"])
        sim = 0
        segs = []
        for s in seg['segments']:
            sim += s["sim"]
            segs.append(f"{seg['timeline'][s['begin']]}-{seg['timeline'][s['end']]}")
        segments['seg_number'].append(seg_number)
        segments['sim'].append(sim)
        segments['segments'].append(" ".join(segs))
    segments = pd.DataFrame(segments)
    return segments

def choose_k(output):
    segments = collect_segments(output)

    k = None
    x = segments.seg_number.array
    y = segments.sim.array
    x = (x - x.min()) / (x.max() - x.min() + 1e-20)
    y = (y - y.min()) / (y.max() - y.min() + 1e-20)
    y = y - x
    k = y.argmax()
    #print("K = ", k + 1)
    #plt.plot(list(x), list(y))
    #plt.show()
    return k + 1