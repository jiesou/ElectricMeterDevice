Import("env")
import re
from pathlib import Path

SRC = Path(env.subst("$PROJECT_DIR")) / "src"
MARK = "USE_COMPAT"
PAIRS = [
    ("wsclient.cpp", "wsclient-compat.cpp"),
    ("hal/matrix.cpp", "hal/matrix-compat.cpp"),
]


def use_compat(main):
    if not main.exists():
        return True
    return bool(re.search(rf"^\s*#\s*define\s+{MARK}\b", main.read_text(encoding="utf-8"), re.M))


filters = ["+<**/*.c>", "+<**/*.cpp>"]
for main, compat in PAIRS:
    compat_on = use_compat(SRC / main)
    filters.append(f"-<{main if compat_on else compat}>")
    print(f"===[select_impl]=== {compat if compat_on else main}")

env.Replace(SRC_FILTER=filters)
