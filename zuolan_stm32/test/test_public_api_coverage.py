"""确保每个项目自有公共函数都有实现和当前用法示例。"""

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
HEADER_DIRS = (
    ROOT / "zuolan_stm32/MY_APP",
    ROOT / "zuolan_stm32/MY_Algorithms/Inc",
    ROOT / "zuolan_stm32/MY_Communication/Inc",
    ROOT / "zuolan_stm32/MY_Hardware_Drivers/Inc",
    ROOT / "zuolan_stm32/MY_Utilities/Inc",
)
SOURCE_DIRS = (
    ROOT / "zuolan_stm32/MY_APP",
    ROOT / "zuolan_stm32/MY_Algorithms/Src",
    ROOT / "zuolan_stm32/MY_Communication/Src",
    ROOT / "zuolan_stm32/MY_Hardware_Drivers/Src",
    ROOT / "zuolan_stm32/MY_Utilities/Src",
)
DECLARATION = re.compile(
    r"^\s*(?!static\b)(?:[A-Za-z_]\w*[\s*]+)+(?P<name>[A-Za-z_]\w*)"
    r"\s*\([^;{}]*\)\s*;",
    re.MULTILINE,
)


def public_functions():
    functions = {}
    for directory in HEADER_DIRS:
        for header in directory.rglob("*.h"):
            for match in DECLARATION.finditer(header.read_text(encoding="utf-8")):
                name = match.group("name")
                # HAL_ 回调由框架调用，不属于学员主动使用的项目接口。
                if not name.startswith("HAL_"):
                    functions[name] = header.relative_to(ROOT)
    return functions


def main():
    functions = public_functions()
    source = "\n".join(
        path.read_text(encoding="utf-8", errors="ignore")
        for directory in SOURCE_DIRS
        for path in directory.rglob("*.c")
    )
    examples = "\n".join(
        path.read_text(encoding="utf-8", errors="ignore")
        for path in Path(__file__).parent.rglob("*")
        if path.suffix in {".c", ".py", ".ps1"} and path != Path(__file__)
    )

    missing_implementation = []
    missing_example = []
    for name, header in sorted(functions.items()):
        definition = re.compile(
            rf"^\s*(?!static\b)[^;{{}}\n]*\b{re.escape(name)}\s*"
            r"\([^;{}]*\)\s*\{",
            re.MULTILINE,
        )
        call = re.compile(rf"\b{re.escape(name)}\s*\(")
        if not definition.search(source):
            missing_implementation.append(f"{name} ({header})")
        if not call.search(examples):
            missing_example.append(f"{name} ({header})")

    assert not missing_implementation, (
        "公共函数缺少非 static 实现：\n" + "\n".join(missing_implementation)
    )
    assert not missing_example, (
        "公共函数缺少 test 示例：\n" + "\n".join(missing_example)
    )
    print(f"公共接口覆盖检查通过：{len(functions)} 个函数均有实现和示例")


if __name__ == "__main__":
    main()
