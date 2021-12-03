import json
import matplotlib.pyplot as plt

def plot(a, hier, n_layer):

    ax_line = plt.subplot(2 * len(a), 1, 2 * n_layer - 1)
    ax_bar = plt.subplot(2 * len(a), 1, 2 * n_layer)

    ax_line.set_xticks(list(range(len(hier["timeline"]))))
    xlabels = []
    for i, t in enumerate(hier["timeline"]):
        if len(hier["timeline"]) < 60 or i % 4 == 0:
            xlabels.append(t)
        else:
            xlabels.append("")
    ax_line.set_xticklabels(xlabels, ha="left", rotation="-45")
    ax_line.plot(list(range(len(hier["timeline"]))), hier["trend_full"], color="b")
    ax_line.scatter(list(range(len(hier["timeline"]))), hier["trend_full"], color="b")
    trend_full = hier["trend_full"]
    timeline = hier["timeline"]
    #from IPython import embed; embed()

    bar_x = [timeline[0]]
    bar_bottom = [0]
    bar_top = [trend_full[0]]
    color = ["blue"]

    for seg in hier["segments"]:
        x = [seg["begin"], seg["end"]]
        y = [trend_full[x[0]], trend_full[x[1]]]
        ax_line.plot(x, y, linewidth=4)

        comp = seg["trend_comp"]
        ax_line.plot(list(range(x[0], x[1] + 1)), comp, color = 'r')
        ax_line.scatter(list(range(x[0], x[1] + 1)), comp, color = 'r')
        x_begin_end = [x[0], x[1]]
        y_begin_end = [comp[0], comp[-1]]
        ax_line.plot(x_begin_end, y_begin_end, linewidth=4)

        preds = []
        begin = trend_full[seg["begin"]]
        interm = begin
        end = trend_full[seg["end"]]

        for expl in seg["explanation"]:
            pp = " & ".join([f"{k} = {v}" for k, v in expl["predicate"]])
            met = expl["metric"]
            bar_x += [f"{pp} {timeline[x[0]]} ~ {timeline[x[1]]} ({met})"]
            delta = met * (1 if expl["direction"] == "+" else -1)
            next_interm = interm + delta
            bar_bottom += [min(interm, next_interm)]
            bar_top += [max(interm, next_interm)]
            if interm < next_interm:
                color += ['green']
            else:
                color += ['red']
            interm = next_interm

        bar_x += [ f"other {timeline[x[0]]} ~ {timeline[x[1]]}", str(timeline[x[1]])]
        bar_bottom += [ min(interm, end), 0]
        bar_top += [max(interm, end), end]
        color += ["y", "b"]

    ax_bar.bar(bar_x, [(bar_top[i] - bar_bottom[i]) for i in range(len(bar_x))], bottom=bar_bottom, color=color)
    ax_bar.set_xticklabels(bar_x, ha="left", rotation="-45")

def plot_output(output, fig_path):
    plt.clf()

    fig = plt.gcf()
    fig.set_size_inches(25, 30 * len(output))

    for i, hier in enumerate(output):
        plot(output, hier, i + 1)

    plt.subplots_adjust(hspace=0.4)
    plt.savefig("./plot/" + fig_path)
