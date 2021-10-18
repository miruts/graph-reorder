# Running Rabbit Reordering #
    usage: python3 rabbit.py [-h] [-p PROGRAM] [-m] [-w] input output

    Process input arguments

    positional arguments:
    input                 input graph file path
    output                output graph file path

    optional arguments:
    -h, --help            show this help message and exit
    -p PROGRAM, --program PROGRAM
                            path to rabbit executable, defaults to rabbit/rabbit
    -m, --maintain        maintain inital vertex
    -w, --weighted        input is weighted graph
E.g. ```python3 rabbit.py -p ./rabbit --maintain --weighted ~/path/to/input/roadNet-PA.wel ~/path/to/output/roadNet-PA.wel```
- specify ```--weighted``` if input graph is weighted edgelist
- specify ```--maintain``` to prevent vertex ID 0 from reshuffling
- provide ```--program PROGRAM_PATH``` to specify rabbit reordering executable path, defaults to rabbit/rabbit