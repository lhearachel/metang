from enum import IntFlag, auto


class Mode(IntFlag):
    ENUM = auto()
    MASK = auto()
    ANY = ENUM | MASK
