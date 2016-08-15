import yaml
import glob
import os.path
import argparse
from subprocess import Popen, PIPE

basepath = os.path.normpath(os.path.join(os.path.abspath(__file__), os.pardir))
regpath = os.path.join(basepath, "registry.yaml")
regpath = os.path.normpath(regpath)

parser = argparse.ArgumentParser()
parser.add_argument('--bmp', help='path to probe',
                    default="/dev/serial/by-id/"
                    "usb-Black_Sphere_Technologies_Black_Magic_Probe_*-if00")
parser.add_argument('--registry', help='path to registry.yaml file',
                    default=regpath)
parser.add_argument('--find',
                    help='just output the target for a specific board',
                    default=None)
args = parser.parse_args()


registry = yaml.load(open(args.registry, "rb"))


def resolve_bmp(bmp):
    unglob = glob.glob(bmp)
    if len(unglob) == 0:
        raise RuntimeError("No BMPs matching glob found")
    bmp = unglob[0]
    real = os.path.realpath(bmp)
    return real


def get_n_targets(bmp):
    p = Popen(["arm-none-eabi-gdb", "--batch",
               "-ex", "tar ext {}".format(bmp),
               "-ex", "mon jtag_scan"], stderr=PIPE)
    _, out = p.communicate()
    out = out.decode()
    lines = out.split("\n")[:-1]
    heading = lines.index("No. Att Driver")
    targets = lines[heading+1:]
    return len(targets)


def get_id(bmp, target):
    p = Popen(["arm-none-eabi-gdb", "--batch",
               "-ex", "tar ext {}".format(bmp),
               "-ex", "mon jtag_scan",
               "-ex", "att {}".format(target+1),
               "-ex", "set mem inaccessible-by-default off",
               "-ex", "x/3xw 0x1FFF7A10"], stdout=PIPE, stderr=PIPE)
    out, err = p.communicate()
    out = out.decode()
    lines = out.split("\n")[:-1]
    target_id = lines[-1].split("\t")[1:]
    target_id = "".join(t[2:] for t in target_id)
    return target_id


def lookup_name(target_id):
    for key in registry:
        if target_id in registry[key]:
            return key
    return "Unknown"


def main():
    bmp = resolve_bmp(args.bmp)
    if args.find is None:
        print("Using {}".format(bmp))

    n_targets = get_n_targets(bmp)
    if args.find is None:
        print("Found {} targets".format(n_targets))

    for i in range(n_targets):
        target_id = get_id(bmp, i)
        target_name = lookup_name(target_id)
        if args.find is None:
            print("Target {}: {:<10} {}".format(i+1, target_name, target_id))
        else:
            if args.find == target_name:
                print(i+1)
                return

if __name__ == "__main__":
    main()
