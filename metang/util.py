from re import sub


def snake(s: str):
    return "_".join(
        sub(
            "([A-Z][a-z]+)",
            r" \1",
            sub("([A-Z]+)", r" \1", s.replace("-", " ").replace(".", " ")),
        ).split()
    )
