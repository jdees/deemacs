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

Available keyboard commands can be found in the struct ``struct Binding bindings`` in file ```deemacs.c```.

Some important keybindings:

* Stop current command: ```CTRL+G```
* Save: ```CTRL+X CTRL+S```
* Exiting: ```CTRL+X CTRL+C```
