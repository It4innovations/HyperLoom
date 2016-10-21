
from report import Report
import argparse
import subprocess
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import sys


def parse_args():
    parser = argparse.ArgumentParser(
        description="rview -- Loom report inscpector")

    parser.add_argument("report",
                        metavar="REPORT",
                        type=str,
                        help="Path to report")

    parser.add_argument("--show-symbols",
                        action="store_true")

    parser.add_argument("--show-graph",
                        action="store_true")

    parser.add_argument("--write-graph",
                        metavar="FILENAME")

    parser.add_argument("--show-trace",
                        action="store_true")

    return parser.parse_args()


def run_program(args, stdin=None):
    p = subprocess.Popen(args, stdin=subprocess.PIPE)
    p.communicate(input=stdin)


def show_symbols(report):
    for i, symbol in enumerate(report.symbols):
        print "{}: {}".format(i, symbol)


def show_graph(report):
    dot = report.create_graph().make_dot("Plan")
    run_program(("xdot", "-"), dot)


def write_graph(report, filename):
    dot = report.create_graph().make_dot("Plan")
    with open(filename, "w") as f:
        f.write(dot)


def show_trace(report):
    plt.ion()
    plt.gca().invert_yaxis()
    y, xmin, xmax, colors, labels, symbols = report.get_events_hline_data()
    plt.hlines(y, xmin, xmax, colors, linewidth=2)
    plt.scatter(xmin, y, marker='|', s=100, c=colors)
    plt.scatter(xmax, y, marker='|', s=100, c=colors)
    plt.yticks(range(len(labels)), labels)
    plt.legend(handles=[mpatches.Patch(color=c, label=s)
                        for s, c in symbols])
    plt.show(block=True)


def main():
    args = parse_args()
    report = Report(args.report)

    empty = True
    if args.show_symbols:
        empty = False
        show_symbols(report)

    if args.show_graph:
        empty = False
        show_graph(report)

    if args.write_graph:
        empty = False
        write_graph(report, args.write_graph)

    if args.show_trace:
        empty = False
        show_trace(report)

    if empty:
        sys.stderr.write("No operation specified (use --help)\n")

if __name__ == "__main__":
    main()
