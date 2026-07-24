#!/usr/bin/env python3
from collections import defaultdict
import os
import time
from pathlib import Path
import sys
from subprocess import check_call, CalledProcessError

THIS_DIR = Path(__file__).absolute().parent


def reset() -> None:
    check_call(["pebble", "kill"])
    check_call(["pebble", "wipe"])


def run(platforms: list[str]) -> int:
    reset()
    screenshot_fnames = defaultdict(list)
    hour = 12
    for minute in range(60):
        env = os.environ.copy()
        env["CFLAGS"] = f"-DHOUR_OVERRIDE={hour} -DMINUTE_OVERRIDE={minute}"
        check_call(["pebble", "build"], env=env)
        success_count = 0
        for platform in platforms:
            while len(screenshot_fnames[platform]) < (minute + 1):
                try:
                    check_call(["pebble", "install", "--emulator", platform])
                    time.sleep(0.05)
                    screenshot_fname = f"screenshot_{platform}_{hour:02d}{minute:02d}.png"
                    check_call(["pebble", "screenshot", "--emulator", platform, screenshot_fname])
                    screenshot_fnames[platform].append(screenshot_fname)
                except CalledProcessError:
                    # Sometimes install fails and I don't know why (real watch and emulator both)
                    # Sometimes screenshot fails and I don't know why (emulator only?)
                    reset()
    reset()

    # Combine all the pngs into one gif with ImageMagick
    # https://joeldare.com/combining-multiple-images-into-an-animated-gif-with-imagemagick
    for platform, fnames in screenshot_fnames.items():
        check_call(
            [
                "convert",
                "-delay",
                "80",
                "-loop",
                "0",
                "-dispose",
                "previous",
                *fnames,
                f"screenshots_{platform}_{hour:02d}xx.gif",
            ]
        )

    return 0


def main() -> int:
    return run(["flint", "emery", "gabbro"])


if __name__ == "__main__":
    sys.exit(main())
