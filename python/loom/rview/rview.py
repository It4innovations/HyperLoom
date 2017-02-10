
from .report import Report
import argparse
import subprocess
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import sys
from itertools import cycle
import json


def parse_args():
    parser = argparse.ArgumentParser(
        description="rview -- Loom report inscpector")

    parser.add_argument("report",
                        metavar="REPORT",
                        type=str,
                        help="Path to report")

    parser.add_argument("--print-symbols",
                        action="store_true")

    parser.add_argument("--print-stats",
                        action="store_true")

    parser.add_argument("--show-graph",
                        action="store_true")

    parser.add_argument("--write-graph",
                        metavar="FILENAME")

    parser.add_argument("--show-trace",
                        action="store_true")

    parser.add_argument("--show-ctransfer",
                        action="store_true")

    parser.add_argument("--show-ctasks",
                        action="store_true")

    parser.add_argument("--show-ctime",
                        action="store_true")

    parser.add_argument("--show-btime",
                        action="store_true")

    parser.add_argument("--show-evrate",
                        action="store_true")

    parser.add_argument("--write-json",
                        metavar="FILENAME")

    parser.add_argument("--write-html",
                        metavar="FILENAME")

    return parser.parse_args()


def run_program(args, stdin=None):
    p = subprocess.Popen(args, stdin=subprocess.PIPE)
    p.communicate(input=stdin)


def print_symbols(report):
    for i, symbol in enumerate(report.symbols):
        print("{}: {}".format(i, symbol))


def show_graph(report):
    dot = report.create_graph().make_dot("Plan").encode()
    run_program(("xdot", "-"), dot)


def write_graph(report, filename):
    dot = report.create_graph().make_dot("Plan")
    with open(filename, "w") as f:
        f.write(dot)


def show_ctasks(report):
    data = report.get_ctasks_data()
    lines = ["o", "s", "D", "<", "p", "8"]
    handles = []
    for k, line in zip(data.keys(), cycle(lines)):
        handles.append(plt.plot(data[k][0], data[k][1],
                       line, ls="-", label=k, markevery=0.1)[0])
    plt.legend(loc='upper left', handles=handles)
    plt.show()


def show_ctime(report):
    data = report.get_ctime_data()
    lines = ["o", "s", "D", "<", "p", "8"]
    handles = []
    for k, line in zip(data.keys(), cycle(lines)):
        handles.append(plt.plot(data[k][0], data[k][1],
                       line, ls="-", label=k, markevery=0.1)[0])
    plt.legend(loc='upper left', handles=handles)
    plt.show()


def show_btime(report):
    data = report.get_btime_data()
    plt.boxplot(list(data.values()))
    plt.xticks(range(1, len(data.keys()) + 1), list(data.keys()),
               rotation="vertical")
    plt.show()


def show_trace(report):
    plt.ion()
    plt.gca().invert_yaxis()
    data, data_unfinished, labels, symbols = report.get_events_hline_data()
    y, xmin, xmax, colors = data
    y_uf, xmin_uf, xmax_uf, colors_uf = data_unfinished

    plt.hlines(y, xmin, xmax, colors, linewidth=2)
    plt.scatter(xmin, y, marker='|', s=100, c=colors)
    plt.scatter(xmax, y, marker='|', s=100, c=colors)

    plt.hlines(y_uf, xmin_uf, xmax_uf, colors_uf, linewidth=2)
    plt.scatter(xmin_uf, y_uf, marker='|', s=100, c=colors_uf)
    plt.scatter(xmax_uf, y_uf, marker='>', s=100, c=colors_uf)

    plt.yticks(range(len(labels)), labels)
    plt.legend(handles=[mpatches.Patch(color=c, label=s)
                        for s, c in symbols])
    plt.show(block=True)


def show_ctransfer(report):
    results1 = report.get_ctransfer_data(True)
    results2 = report.get_ctransfer_data(False)

    f, (c1, c2, c3) = plt.subplots(3, sharex=True)
    assert f  # silence nonused f

    c1.set_title("Total transfers")
    c1.plot(results1[-1][0], results1[-1][1], label="Intra")
    c1.plot(results2[-1][0], results2[-1][1], label="Results")

    c2.set_title("Intra transfers per label")
    for data0, data1, label in results1[:-1]:
        c2.plot(data0, data1, label=label)

    c3.set_title("Result transfers per label")
    for data0, data1, label in results2[:-1]:
        c3.plot(data0, data1, label=label)

    c1.legend(loc='upper left')
    c2.legend(loc='upper left')

    plt.show(block=True)


def show_evrate(report):
    r1, r2 = report.get_event_rate_data(1000)
    plt.plot(r1, r2, label="# of events per second")
    r1, r2 = report.get_event_rate_data(60000)
    plt.plot(r1, r2, label="# of events per minute")
    plt.legend(loc='upper right')
    plt.show()


def print_stats(report):
    print("Labels:")
    result = report.label_counts()
    for label in sorted(result):
        print("{}: {}".format(label, result[label]))


def write_json(report, filename):
    data = report.get_json_data()
    with open(filename, "w") as f:
        f.write(json.dumps(data))


def write_html(report, filename):
    from bokeh.charts import Histogram, BoxPlot, HeatMap, save, output_file
    from bokeh.layouts import column
    from bokeh.palettes import Dark2_5 as palette
    from bokeh.plotting import figure

    # FIXME:Do NOT hardcode duration heatmap configuration
    # The following values are specific to ExCAPE pipeline
    dhm_conf = {"params": ["cost", "gamma"], "task_type": "train"}

    # Get data
    event_rate_data, task_duration_data, ctdd, dhmd = report.get_html_data(dhm_conf)

    # Plot event rate histograms
    erh = Histogram(event_rate_data["time"], title="Event rate histogram",
                    xlabel="Time [ms]", ylabel="Event Rate [#]", width=1000)
    erh2 = Histogram(event_rate_data, title="Event rate histogram (per task type)",
                     values="time", color="task_type", width=1000,
                     xlabel="Time [ms]", ylabel="Event Rate [#]")

    # Plot task duration
    tdbp = BoxPlot(task_duration_data, values="duration", label="task_type",
                   width=1000, legend=False, title="Task duration box plot",
                   xlabel="Task Type", ylabel="Duration [ms]")

    # Plot cummulative task duration
    ctdl = figure(title="Cummulative task duration", x_axis_label='Time [ms]',
                  y_axis_label='Cummulated duration [ms]', width=1000)
    for k, c in zip(ctdd.keys(), cycle(palette)):
        ctdl.line(ctdd[k][0], ctdd[k][1], color=c, legend=k, line_width=2)

    # Plot duration heatmap data (!ExCAPE specific)
    dhm = HeatMap(dhmd, title="Duration heatmap", x=dhm_conf["params"][0],
                  y=dhm_conf["params"][1], values='values', stat='mean', width=1000)

    # Plot duration BoxPlot data (!ExCAPE specific)
    dbp = BoxPlot(dhmd, values="values", label="labels",
                  width=1000, legend=False, title="Duration box plot",
                  xlabel="Label", ylabel="Duration [ms]")

    # Plot execution trace
    trcd, trcd_unfinished = report.get_events_hline_data_bokeh()
    trcp = figure(title="Execution trace", x_axis_label='Time [ms]',
                  y_axis_label='Worker [#]', width=1000)
    for k in trcd.keys():
        y, xmin, xmax, c = trcd[k]
        color = (int(c[0]*255), int(c[1]*255), int(c[2]*255))
        trcp.segment(x0=xmin, y0=y, x1=xmax, y1=y, line_width=2, legend=k,
                     color=color)
    for k in trcd_unfinished.keys():
        y, xmin, xmax, c = trcd_unfinished[k]
        color = (int(c[0]*255), int(c[1]*255), int(c[2]*255))
        trcp.segment(x0=xmin, y0=y, x1=xmax, y1=y, line_width=2, legend=k,
                     color=color, line_dash="dotted")

    output_file(filename)
    save(column(erh, erh2, tdbp, ctdl, dhm, dbp, trcp))


def main():
    args = parse_args()
    report = Report(args.report)

    empty = True
    if args.print_symbols:
        empty = False
        print_symbols(report)

    if args.print_stats:
        empty = False
        print_stats(report)

    if args.show_graph:
        empty = False
        show_graph(report)

    if args.write_graph:
        empty = False
        write_graph(report, args.write_graph)

    if args.show_trace:
        empty = False
        show_trace(report)

    if args.show_ctransfer:
        empty = False
        show_ctransfer(report)

    if args.show_ctasks:
        empty = False
        show_ctasks(report)

    if args.show_ctime:
        empty = False
        show_ctime(report)

    if args.show_btime:
        empty = False
        show_btime(report)

    if args.show_evrate:
        empty = False
        show_evrate(report)

    if args.write_json:
        empty = False
        write_json(report, args.write_json)

    if args.write_html:
        empty = False
        write_html(report, args.write_html)

    if empty:
        sys.stderr.write("No operation specified (use --help)\n")

if __name__ == "__main__":
    main()
