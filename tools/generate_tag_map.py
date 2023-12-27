import sys

def main(input_file: str):
    with open(input_file, "r") as fp:
        print(
"""#include <map>

std::map<int, int> tags{""")
        
        for line in fp.readlines():
            progressive, uuid, impulse = line.split()

            uuid = uuid.replace(":", "")

            print("    {", end='')
            print(f"0x{uuid}, {impulse}", end='')
            print("},")

        print("};")

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} filename", file=sys.stderr)
    else:
        main(sys.argv[1])