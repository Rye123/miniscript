import subprocess
from pathlib import Path
from sys import exit

show_contents = True
testdir = Path("../test")
executable = Path("./miniscript")

if __name__ == "__main__":
    if not testdir.is_dir():
        print("could not find test directory")
        exit(1)
    if not executable.is_file():
        print("could not find miniscript executable, please run make")
        exit(1)

    executable = executable.resolve()

    # Loop through directories
    for test_file in testdir.rglob('*.ms'):
        if test_file.is_file():
            print(f"~~~ test {test_file.parent.stem}/{test_file.stem} ~~~")
            if show_contents:
                print("Contents:")
                print(test_file.read_text())
                print("---")
            out = subprocess.run([str(executable), str(test_file.resolve())], capture_output=True).stdout
            print(out.decode())
            print("\n~~~\n")

    print("Tests done")
