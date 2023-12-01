from terminal import *
import argparse


def parse_args():
    parser = argparse.ArgumentParser(description='Ftrace Parser')
    parser.add_argument('-p', nargs='?', type=str, help='Process pid to trace')
    parser.add_argument('-t', nargs='?', type=str, help='Record Time', default='10')
    return parser.parse_args()

if __name__ == '__main__':
    args = parse_args()
    pid = args.p
    record_time = args.t
    tc = TraceCmd(pid, record_time)
    parser = Parser(pid, tc.report_file)
    # parser.execute()
    tree_parser = TreeParser(pid, tc.report_file)
    # print("Executing tree parser")
    # tree_parser.execute()
    terminal = Terminal(tc, parser, tree_parser)
    terminal.run()
