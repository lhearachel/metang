import json
from metang.mode import Mode
from metang.options import Options

def generate(enum: list[tuple[str, int]], opts: Options, maxlen: int, digits: int):
    data = {}
    prefix = "" if not opts.leader else f"{opts.leader}_"

    if opts.mode & Mode.ENUM:
        for idt, val in enum:
            data[f"{prefix}{idt}"] = val
    else:
        if len(enum) > 0:
            data[f"{prefix}{enum[0][0]}"] = 0
        
        for idt, val in enum[1:-1]:
            data[f"{prefix}{idt}"] = 1 << val
            
        if len(enum) > 1:
            data[f"{prefix}{enum[-1][0]}"] = (1 << enum[-1][1]) - 1

    print(json.dumps(data, indent=4), file=opts.fout)
