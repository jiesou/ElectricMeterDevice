Import("env")
import re
from pathlib import Path

SRC = Path(env.subst("$PROJECT_DIR")) / "src"
MARK = "USE_COMPACT"
PAIRS = [
    ("wsclient.cpp", "wsclient-compact.cpp"),
    ("hal/matrix.cpp", "hal/matrix-compact.cpp"),
]


def use_compact(main):
    if not main.exists():
        return True
    return bool(re.search(rf"^\s*#\s*define\s+{MARK}\b", main.read_text(encoding="utf-8"), re.M))


filters = ["+<**/*.c>", "+<**/*.cpp>"]
for main, compact in PAIRS:
    on = use_compact(SRC / main)
    filters.append(f"-<{main if on else compact}>")
    print(f"===[select_impl]=== {compact if on else main}")

env.Replace(SRC_FILTER=filters)
