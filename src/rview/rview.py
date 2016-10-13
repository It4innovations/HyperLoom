
from report import Report
import argparse
import subprocess


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


def main():
    args = parse_args()
    report = Report(args.report)

    if args.show_symbols:
        show_symbols(report)

    if args.show_graph:
        show_graph(report)


if __name__ == "__main__":
    main()
