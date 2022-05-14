# CSVR
A simple CLI CSV Reader.

![Demo CSVR](resources/demo_csvr.gif)

The current suppported feature is to view the files with arrow keys, vim 'hjkl' navigation style, PgUp/PgDown, vim Ctrl-D/Ctrl-U page navigation style, and cell resizing.

## Compilation
To compile on Linux, simply compile with make command.
```Shell
make
```

## Usage
On the same directory as you compile it, simply execute the command by specifying the separator with <code>-t</code> and the csv file.
```Shell
./csvr -t ',' <csv file>
```

