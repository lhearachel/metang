from enum import Enum, auto


class Lang(Enum):
    c = auto()
    py = auto()
    json = auto()

    def __str__(self):
        return self.name

    def __repr__(self):
        return str(self)

    @staticmethod
    def argparse(s: str):
        try:
            return Lang[s.lower()]
        except KeyError:
            return s
