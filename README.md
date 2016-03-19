Deemacs
=======

A lightweight editor written in C mimicking keyboard shortcuts and behaviour of Emacs.

Install
-------

```
    $ git clone https://github.com/jdees/deemacs.git
    $ cd deemacs
    $ make
```

Getting started
---------------

Run ```deemacs myfile.txt``` to start the editor and load the file into the editor.

Available keyboard commands can be found in the struct ``struct Binding bindings`` in file ```deemacs.c```.

Some important keybindings:

  * Stop current command: ```CTRL+G```
  * Save: ```CTRL+X CTRL+S```
  * Exit: ```CTRL+X CTRL+C```
  * Show Keybindings: ```M-?``` or ```CTRL+H B```

Coding Standards
----------------

Follow GNU coding starndard.
It includes using

  * guile
  * perror
  * TMPDIR with fallback to /tmp
  * error msg variants:
```
    SOURCE-FILE-NAME:LINENO: MESSAGE
    SOURCE-FILE-NAME:LINENO:COLUMN: MESSAGE
    SOURCE-FILE-NAME:LINENO.COLUMN: MESSAGE
    SOURCE-FILE-NAME:LINENO-1.COLUMN-1-LINENO-2.COLUMN-2: MESSAGE
    SOURCE-FILE-NAME:LINENO-1.COLUMN-1-COLUMN-2: MESSAGE
    SOURCE-FILE-NAME:LINENO-1-LINENO-2: MESSAGE
    FILE-1:LINENO-1.COLUMN-1-FILE-2:LINENO-2.COLUMN-2: MESSAGE
```
  * use getopt for command line options
    getopt_long
    support "--verbose", "--help" and "--version" as long option

License
-------

Copyright (C) 2016 by Jonathan Dees.

Deemacs is licensed under GPLv3 (see [COPYING](./COPYING) ).

This is free software: you are free to change and redistribute it.

There is NO WARRANTY, to the extent permitted by law.
