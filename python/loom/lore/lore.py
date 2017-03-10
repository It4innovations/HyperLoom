
from loom.lore.report import Report
from loom.lore import html

import sys
import os
import argparse


def parse_args():
    parser = argparse.ArgumentParser(
        description="rview -- Loom report inscpector")

    parser.add_argument("trace_path",
                        metavar="TRACE_PATH",
                        type=str,
                        help="Path to report")
    parser.add_argument("--full", action="store_true",
                        help="full report")
    return parser.parse_args()


def main():
    args = parse_args()

    if not os.path.isdir(args.trace_path):
        sys.stderr.write("Directory '{}' not found\n".format(args.trace_path))

    report = Report(args.trace_path)
    html.create_html(report, "output.html", args.full)


if __name__ == "__main__":
    main()
