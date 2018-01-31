Pynac is a C++ library for symbolic manipulation based on GiNaC. It can use arbitrary Python objects for numeric types. Since Pynac is closely tied to Sage, setting up a development environment and finding your way around initially can be troublesome. This is a short tutorial to provide help with the first steps.

GiNaC (Pynac's ancestor) has great documentation. It's a good idea to read the [tutorial](http://www.ginac.de/tutorial/) to get familiar with the library. The [reference manual](http://www.ginac.de/reference/) for GiNaC can also be useful to find your way around.

(This is a copy of https://github.com/pynac/pynac/wiki/Setting-up-a-development-environment which might be newer)

### Setting up a development environment

One way to set up a development pipeline is to follow these steps:

0. have Sage built from git source.
**In the following `SAGE_ROOT` means the full path to the directory where you installed Sage.**
1. if the current branch from https://github.com/pynac/sage is newer than Sage develop then fetch and build it
2. `cd` into `SAGE_ROOT`
3. clone the pynac repository: `git clone git@github.com:pynac/pynac.git` or extract the source from the `upstream` subdirectory with `tar xvfj upstream/pynac-...tar.bz2`
4. `cd` into the created directory
5. if you cloned the repo this is the time to create files needed for build (they are included in the tarball). This needs `autoconf`, `automake`, `m4`, `libtoolize`, and `autoreconf` installed on your machine. Do:
```
user@host: aclocal
user@host: autoconf
user@host: autoreconf --install
user@host: automake
```
Now type
```
export CFLAGS=-I$SAGE_ROOT/local/include
./configure --includedir=$SAGE_ROOT/local/include --libdir=$SAGE_ROOT/local/lib` PKG_CONFIG_PATH=$SAGE_ROOT/local/lib/pkgconfig
```
and the development environment is set up (to compile Pynac outside Sage you need to give configure a hint where the headers of dependent packages are if you only have installed those packages via Sage, as opposed to a system installation of e.g. flint).

After having changed Pynac to your liking you want to test it inside the newest Sage.

1. inside `SAGE_ROOT/pynac` say `make dist`
2. copy the resulting `bz2` tarball to `SAGE_ROOT/upstream`
3. `cd` to `SAGE_ROOT` and:
```
user@host: ./sage --package fix-checksum
user@host: ./sage -p pynac
user@host: make
user@host: ./sage -tp -a
```
If no doctests fail you're ready to go! Instead of `make` which also builds the full documentation you might want to use `make start` if you are not interested in that.


(Previously the install instructions went like this:)

In order to start working on the source code go to your `SAGE_ROOT` and run
`./sage -f -m spkg/standard/pynac-*.spkg`.

This will install the latest version of Pynac that comes with Sage and leave the package files in the directory `spkg/build/pynac-<version>`. You will find the Pynac sources in `spkg/build/pynac-<version>/src`, most files you'll want to work on are in the subdirectory `ginac`.
After you make changes to the Pynac files, in order to build your changes and install them so Sage will see them, get in a Sage shell with
```
./sage -sh
```
Go to the Pynac source directory
```
cd spkg/build/pynac-*/src
```
and run
```
make install
```

(copy of https://github.com/pynac/pynac/wiki/Development-protocol)

### Respect the existing C++ style
In particular:
 * 8-space expanded tabs (no TAB characters)
 * Code blocks:
    * The braces opening a **function block** should be on a new line.
    * The braces opening a **condition block** should be on the end of the line with the condition. The closing braces should be indented such that they are on the same column as the start of the condition statement.

### Pull requests and the release cycle
 * Pull requests must compile without warning to be accepted.
 * Make sure your master branch is up-to-date so that fixes from other developers are applied.
 * There is a branch at https://github.com/pynac/sage which contains modifications to SageMath's source code and/or documentation such that (ideally) the current development version of pynac passes all of SageMath's doctests.
    * If one of your PRs requires a change in SageMath's doctests, please add the diff from **the changes to SageMath** as a code comment to your PR, or consider opening a PR to the upgrade branch at https://github.com/pynac/sage itself.
    * Before submitting a PR to pynac you should test your changes at least against the doctests in `src/sage/symbolic`, `src/sage/functions`, `src/sage/calculus`, `src/sage/tests/`,and `src/doc/`.
    * The release maintainer will do a full test in the freezing phase.
 * In the freezing phase no PRs get accepted, and developers are required to fix remaining failing doctests.
 * After the freezing phase, a new version of pynac is released and the development phase for the next pynac version begins.
 * freezing is announced in https://groups.google.com/forum/?hl=en#!forum/pynac-devel so please subscribe.
