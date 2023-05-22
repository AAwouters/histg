# histg

HIST stands for homeomorphically irreducible spanning tree, which is a spanning tree without vertices of degree 2.

## Building locally

Build using ```make```, binary and build files are found in ./bin/. A UNIX-like system is required.

## Usage

```histg --help``` provides all possible options.

The most common use case is to read graphs in [graph6 format](https://users.cecs.anu.edu.au/~bdm/data/formats.txt) from either stdin or a file provided by ```-i```.
Histg will then report the number of HISTs in each graph to stdout or a file provided by ```-o```.