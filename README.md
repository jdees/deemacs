Deemacs
=======

A lightweight editor written in C mimicking keyboard shortcuts and behaviour of Emacs.

Install
-------

```
    $ git clone https://github.wdf.sap.corp/D061399/deemacs.git
    $ cd deemacs
    $ make
```

Getting started
---------------

Run ```deemacs myfile.txt``` to start the editor and load the file into the editor.
Available keyboard commands can be found in the file deemacs.c under ``struct Binding bindings[]``.

Exiting: CTRL+X CTRL+C
Save: CTRL+X CTRL+S
