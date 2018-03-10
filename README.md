# CS5218 - Program Analysis
## Assignment 1 - Taint Analysis

This program performs taint analysis over simple C programs, with strict requirements of the sink and source variable names.

## Dependencies

* This project compiles for macOS High Sierra 10.13.3.
* LLVM and Clang installed as specified by the instruction from the website.
* Xcode command line tool is properly installed.
* gcc and g++ exists and is referenced from Xcode's developer tools.

## Build and Run

The programs to be analyzed are located inside
```
resources/
```
The files currently there are

```
example1.c
example2.c
example3.c
```

You can build this project by running
```
./build.sh
```
on the root directory. This will convert the C programs inside resources into bytecode (.bc) and LLVM IR (.ll) files and compile the analyzer source code into an executable.

To run the analysis over the IR, you can run
```
./run.sh <file option>
```
This will perform taint analysis on example1.c, example2.c, or example3.c (file option being 1, 2, or 3 respectively).

## Helpers

Scripts to help with source code conversion and compiling are in
```
helpers/
```
To perform the program resource into LLVM IR, you can call
```
helpers/prep.sh
```
And to compile the analyzer you can call
```
helpers/compile.sh
```

## Other

Source code is in
```
src/
```

Compiled binary is in
```
bin/
```

## Author

* **Teekayu Klongtruajrok**
