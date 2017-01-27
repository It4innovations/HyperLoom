
from .report import Report
import argparse
import subprocess
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import sys
from itertools import cycle


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

    parser.add_argument("--show-ctasks",
                        action="store_true")

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


def print_stats(report):
    print("Labels:")
    result = report.label_counts()
    for label in sorted(result):
        print("{}: {}".format(label, result[label]))


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

    if args.show_ctasks:
        empty = False
        show_ctasks(report)

    if empty:
        sys.stderr.write("No operation specified (use --help)\n")

if __name__ == "__main__":
    main()
