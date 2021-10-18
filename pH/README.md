# Running pH (Profit enhancing Heuristics) Reordering #
**pH** - It makes use of the fact that nodes are processed in the order of their indices and maintains a dynamic model of cache contents based on previously processed nodes

    usage: python3 ph.py [-h] [-p PROGRAM] [-m] [-w] input output

    Process input arguments

    positional arguments:
    input                 input graph file path
    output                output graph file path

    optional arguments:
    -h, --help            show this help message and exit
    -p PROGRAM, --program PROGRAM
                            ph reordering program path
    -m, --maintain        maintain initial al vertex
    -w, --weighted        input is weighted graph
E.g. ```python3 ph.py -p ./ph --maintain --weighted ~/path/to/input/Pokec.wel ~/path/to/output/Pokec.wel```
- provide ```--program PROGRAM_PATH``` to specify ph executable path, since this script uses an opensource ph reordering implementation. Defaults to ph/ph
- specify ```--maintain``` to prevent vertex ID 0 from reshuffling
- specify ```--weighted``` for weighted input graphs

PS. The original ph executable is modified so as to log new node mapping to ```./ph/new_order.el``` file. Modified code is provided in ```./pH``` folder.
