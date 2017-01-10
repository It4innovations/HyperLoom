
#
# This program waits for X workers in server
#

import argparse
import sys
import time
from ..client import Client

def parse_args():
    parser = argparse.ArgumentParser(
        description="Wait "
                    "-- waits for workers in server")

    parser.add_argument("server",
                        metavar="SERVER",
                        type=str,
                        help="Server hostname")

    parser.add_argument("port",
                        metavar="PORT",
                        type=int,
                        help="Server TCP port")

    parser.add_argument("n_workers",
                        metavar="N_WORKERS",
                        type=int,
                        help="Number of workers")

    parser.add_argument("timeout",
                        metavar="TIMEOUT",
                        type=int,
                        help="timeout (in seconds)")

    args = parser.parse_args()
    return args



def main():
    args = parse_args()
    client = Client(args.server, args.port)

    t = 0
    workers = -1

    while True:
        if t > args.timeout:
            print("Timeouted")
            sys.exit(1)
        n_workers = client.get_stats()["n_workers"]
        if workers != n_workers:
            workers = n_workers
            print("Workers in server: {}/{}".format(workers, args.n_workers))
            if workers == args.n_workers:
                return
        time.sleep(1)
        t += 1


if __name__ == "__main__":
    main()
