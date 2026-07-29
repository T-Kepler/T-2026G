"""运行无需开发板的 STM32 自动测试。"""

import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


TEST_DIR = Path(__file__).resolve().parent
PROJECT_DIR = TEST_DIR.parent


def run(command):
    print(">", " ".join(map(str, command)), flush=True)
    subprocess.run(command, cwd=PROJECT_DIR.parent, check=True)


def main():
    gcc = shutil.which("gcc")
    if gcc is None:
        raise SystemExit("未找到 gcc，无法编译 PC 端 C 算法测试")

    python_tests = sorted(
        path for path in TEST_DIR.glob("test_*.py")
        if path.name != "test_public_api_coverage.py"
    )
    for test in python_tests:
        run([sys.executable, test])

    builds = {
        "algorithms": {
            "sources": [
                TEST_DIR / "test_algorithms.c",
                PROJECT_DIR / "MY_Algorithms/Src/kalman.c",
                PROJECT_DIR / "MY_Algorithms/Src/phase_measure.c",
            ],
            "includes": [PROJECT_DIR / "MY_Algorithms/Inc"],
        },
        "fir": {
            "sources": [
                TEST_DIR / "test_fir.c",
                PROJECT_DIR / "MY_Algorithms/Src/my_filter.c",
            ],
            "includes": [PROJECT_DIR / "MY_Algorithms/Inc"],
        },
        "usart_pack": {
            "sources": [
                TEST_DIR / "test_usart_pack_host.c",
                PROJECT_DIR / "MY_Communication/Src/my_usart_pack.c",
            ],
            "includes": [
                PROJECT_DIR / "MY_Communication/Inc",
                PROJECT_DIR / "Core/Inc",
                PROJECT_DIR / "Drivers/STM32F4xx_HAL_Driver/Inc",
                PROJECT_DIR / "Drivers/CMSIS/Device/ST/STM32F4xx/Include",
                PROJECT_DIR / "Drivers/CMSIS/Include",
            ],
            "defines": ["STM32F429xx", "USE_HAL_DRIVER"],
        },
    }
    with tempfile.TemporaryDirectory(prefix="zuolan_stm32_test_") as temp_dir:
        for name, build in builds.items():
            executable = Path(temp_dir) / f"test_{name}.exe"
            run([
                gcc,
                "-std=c99",
                *(f"-D{define}" for define in build.get("defines", [])),
                *build["sources"],
                *(option for include in build["includes"]
                  for option in ("-I", include)),
                "-lm",
                "-o",
                executable,
            ])
            run([executable])

    run([sys.executable, TEST_DIR / "test_public_api_coverage.py"])
    print("STM32 自动测试全部通过")


if __name__ == "__main__":
    main()
