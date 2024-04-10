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
    for tests in testdir.iterdir():
        if not tests.is_dir():
            continue

        for test in tests.iterdir():
            if not test.is_file():
                continue
            if not test.suffix == ".ms":
                continue
            test_file = test.resolve()
            print(f"~~~ test {str(test.resolve().parent.stem)}/{str(test.resolve().stem)} ~~~")
            if show_contents:
                print("Contents:")
                print(test.read_text())
                print("---")
            out = subprocess.run([str(executable), str(test_file)], capture_output=True).stdout
            print(out.decode())
            print("\n~~~\n")

        print("Tests done")
