# Running Block Reordering #
**Block Reordering** - Alters the order in which cache lines are stored in memory after nodes are efficiently packed

    usage: python3 br.py [-h] [-p PROGRAM] [-m] [-w] input output

    Process input arguments

    positional arguments:
    input                 input graph file path
    output                output graph file path

    optional arguments:
    -h, --help            show this help message and exit
    -p PROGRAM, --program PROGRAM
                          block reordering program path
    -m, --maintain        maintain initial al vertex
    -w, --weighted        input is weighted graph
E.g. ```python3 br.py -p ./br --maintain ~/path/to/input/LiveJournal.el ~/path/to/output/LiveJournal.el```
- specify ```--maintain``` to prevent vertex ID 0 from reshuffling
- specify ```--weighted``` for weighted input graphs
- provide ```--program PROGRAM_PATH``` to specify block_reordering program path, defaults to br/br

PS. The original block reordering executable is modified so as to log new node mapping to a ```./ph/new_order.el``` file in the current directory. Modified code is provided in ```./block_reordering``` folder.