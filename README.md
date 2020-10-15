## Dtool Utility

This CommandLine Tool aims at being a generic utility framework, with each tool as a shared library. It provides functions as library calls, so we can build tools more easily.

## How to build

``` bash
cd src
cmake -DCMAKE_INSTALL_PREFIX="$HOME/.local" -B build
make -C build
```

use `make -C build install` to install files to your `$HOME/.local` directory.

binaries will be put under `$HOME/.local/bin` and modules will be put under `$HOME/.local/.mod`.

remember executing `export LD_LIBRARYPATH_PATH` to add module directory path to shared libraries search path.

## Current modules

#### CMD process

It serves as a generic command line arguments parsing tool. It has dependency check between each command line option and values.

You can register function calls in a table and specific each dependency graph.

More Interfaces to be come in the future.

#### capstone wrapper

This wrapper is a simple function integration of raw capstone calls.

#### NET wrapper

> Provide basic socket manipulation wrapper. No need to handle those low level structures.

##### TODO

- [ ] Add basic socket operation

#### Dynamic module

add module support, keep function codes seperating from mainline development tree.
