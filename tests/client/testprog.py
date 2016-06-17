import argparse
import time
import sys
from datetime import datetime

def main():
    parser = argparse.ArgumentParser(
        description='Simple program called in test')
    parser.add_argument(
        'operation', metavar='OP', choices=["empty", "copy", "plus1"])
    parser.add_argument(
        'sleep', metavar='SLEEP', type=float)
    parser.add_argument(
        '--stamp', action='store_true', help='Print timestamp on stdout')
    parser.add_argument(
        '--out', help='Output file')
    parser.add_argument(
        '--input', help='Input file')

    args = parser.parse_args()
    run(args)


def run(args):
    if args.stamp:
        print datetime.now()

    op = args.operation
    if op != "empty":
        if args.input:
            inp = open(args.input, "r")
        else:
            inp = sys.stdin

        data = inp.read()
        if op == "plus1":
            data = "".join(chr(ord(c) + 1) for c in data)
        else:
            assert op == "copy"

        if args.out:
            out = open(args.out, "w")
        else:
            out = sys.stdout

        out.write(data)

    time.sleep(args.sleep)

    if args.stamp:
        print datetime.now()


if __name__ == "__main__":
    main()
