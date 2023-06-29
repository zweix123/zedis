import os, re


if __name__ == "__main__":
    pattern = r"#ifdef DEBUG.*?#endif\s*\n"
    regex = re.compile(pattern, re.DOTALL)

    for file in [
        os.path.abspath(os.path.join(dirpath, filename))
        for dirpath, dirnames, filenames in os.walk(".")
        for filename in filenames
        if str(filename).endswith(".h") or str(filename).endswith(".cpp")
    ]:
        with open(file, "r") as f:
            content = f.read()

        content = regex.sub("", content)

        with open(file, "w") as f:
            f.write(content)
