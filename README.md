# Dash Unix Shell

Developed a custom unix shell known as Dash, as part of the CS 5348 - Operating Systems Course.

## Features -

- Basic Shell with batch and interactive modes
- Built-in commands
  - exit
  - cd
  - path
- Redirection
- Parallel commands

## Running The Shell

To run in interactive mode -
```
$ ./dash
$ dash>
```
To run in batch mode -
```
$ ./dash batch.txt
```
To redirect or run parallel commands -
```
$ dash> echo "Hi" > test.txt
$ dash> cmd1 & cmd2 args1 args2 & cmd3 args1
```


