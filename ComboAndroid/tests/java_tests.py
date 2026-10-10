"""Compile the app's plain-Java storage classes with one test's main() and run it.

RomImporter, DataLock and StorageCopy use only java.*, so no Android SDK or
Gradle is needed; the tests are ordinary main() programs that throw on failure.
"""
from pathlib import Path
import argparse
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--javac', default='javac')
parser.add_argument('--java', default='java')
parser.add_argument('--test', required=True, help='Test class in ComboAndroid/tests, e.g. RomImporterTest')
args = parser.parse_args()
classes = args.build_dir.resolve()
classes.mkdir(parents=True, exist_ok=True)
app = args.source.resolve()/'ComboAndroid/app/src/main/java/org/comboship/android'
sources = [app/'RomImporter.java', app/'DataLock.java', app/'StorageCopy.java',
           Path(__file__).resolve().parent/(args.test + '.java')]
subprocess.run([args.javac, '-encoding', 'UTF-8', '-d', str(classes), *map(str, sources)], check=True, timeout=600)
subprocess.run([args.java, '-ea', '-cp', str(classes), 'org.comboship.android.' + args.test], check=True, timeout=300)
