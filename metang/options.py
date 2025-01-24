from dataclasses import dataclass
from typing import TextIO

from metang.mode import Mode


@dataclass
class Options:
    fin: TextIO
    fin_name: str
    fout: TextIO
    fout_name: str
    lang: str
    leader: str
    tag: str
    guard: str
    mode: Mode
