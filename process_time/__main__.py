from terminal import *
import argparse


def parse_args():
    parser = argparse.ArgumentParser(description='Ftrace Parser')
    parser.add_argument('-p', nargs='?', type=str, help='Process pid to trace')
    return parser.parse_args()

if __name__ == '__main__':
    args = parse_args()
    pid = args.p
    tc = TraceCmd(pid)
    parser = Parser(pid)
    terminal = Terminal(tc, parser)
    terminal.run()
