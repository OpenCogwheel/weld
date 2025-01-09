# weld
A cargo inspired toml build system for c and c++

## todo:
- [x] multi-threaded build system
- [x] project type (DynamicLib, StaticLib, ...)
- [x] custom file extensions
- [x] exclude files/folders
- [x] implement recursive building
- [ ] progress bar, and more info
- [ ] proper error system
- [x] workspace system
- [x] dependency system
- [x] project install system
    - [ ] Add more customizability
- [ ] build only changed files

## Install information
To compile **weld** you need cmake or an already existing weld installation.
##### cmake (ninja recommended)
```
$ cmake -G Ninja -B bin
$ cmake --build bin
```
##### weld (can be done after cmake)
```
$ weld install
```
