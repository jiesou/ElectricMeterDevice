Import("env")
import re
from pathlib import Path

SRC = Path(env.subst("$PROJECT_DIR")) / "src"
MARK = "USE_COMPAT"
PAIRS = [
    ("wsclient.cpp", "wsclient-compat.cpp"),
    ("hal/matrix.cpp", "hal/matrix-compat.cpp"),
    ("hal/ztw_ddsu666.cpp", "hal/ztw_ddsu666-compat.cpp"),
]


def use_compat(main):
    text = main.read_text(encoding="utf-8") if main.is_file() else ""
    return bool(re.search(rf"^\s*#\s*define\s+{MARK}\b", text, re.M))


filters = ["+<**/*.c>", "+<**/*.cpp>"]
for main, compat in PAIRS:
    on = use_compat(SRC / main)
    filters.append(f"-<{main if on else compat}>")
    print(f"===[select_impl]=== {compat if on else main}")

env.Replace(SRC_FILTER=filters)
