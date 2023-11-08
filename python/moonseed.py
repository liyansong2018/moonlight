# from liyansong2018

import sys
import json
import os

def main():
    if len(sys.argv) != 4:
        print('Usage: %s <src> <dst> json-file' % sys.argv[0])
        return
    else:
        if not os.path.exists(sys.argv[2]):
            os.system('mkdir %s' % sys.argv[2])
        try:
            with open(sys.argv[3]) as fjson:
                data = json.load(fjson)
                for seed in data['solution']:
                    os.system('cp %s %s' % (os.path.join(sys.argv[1], seed), sys.argv[2]))
        except Exception as e:
            print(e)

if __name__ == '__main__':
    main()