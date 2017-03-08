# Introduction

Shell++ is a programming language that aims bring features from modern languages, 
as facility to manipulate data structures, object oriented programming, 
functional programming and others, to shell script.
https://alexst07.github.io/shell-plus-plus/

# How it looks like:
```php
# shell commands
echo hello world
rm *.tmp
cat file | grep -e "a.b" > test.txt

# functions declaration
func ftest(a) {
  v = ["echo", "ls", a]
  
  # closures
  return func(x) {
    return v.append(x)
  }
}

func make_map() {
  # return hash map
  return {"a": "apple", "ok":7}
}

# executes the shell commands from the vector 
# returned by the clousure
for c in ftest("pwd")("uname") {
  ${c}
}

# glob example
for file in %*.txt% {
  # rename all files to upper case
  # handle strings like python
  mv ${file} ${file[:-4].to_upper() + file[-4:]}
}
```

# Features
 * easy to manipulate data structure, as in Python
 * lambda functions
 * classes
 * executes commands like in Bash
 * pipe, redirection to or from files, sub shell
 * dynamic typing
 * glob and recursive glob
 * closures
 * reference counter
 * operator overriding

# Why another programming language?
I wanted a language that runs shell commands like Bash, and manipulate data structure with the ease of Python, I searched, but I did not find any that met my requirements, so I decided to use my free time to create one.

This is still in development, but it has already made life easier for me in many day-to-day tasks, maybe this might be useful for someone else, so I shared it on github.

However everything here can change without warning, because to date, the only purpose of it is MAKE MY LIFE EASIER AND ONLY THAT.

# Building

## Requirements:
  * A compiler that supports C++ 14 (gcc or clang)
  * Boost
  * Readline
  * CMake
  * Linux
  
## Compiling

### Fedora
```
# dnf install gcc-c++ clang
# dnf install boost boost-devel readline readline-devel cmake git
$ git clone https://github.com/alexst07/shell-plus-plus
$ cd shell-plus-plus
$ mkdir build && cd build
$ cmake ..
$ make
# make install
```

### Ubuntu
```
# apt-get install -y build-essential
# apt-get install -y libboost-all-dev libreadline6 libreadline6-dev git cmake
$ git clone https://github.com/alexst07/shell-plus-plus
$ cd shell-plus-plus
$ mkdir build && cd build
$ cmake ..
$ make
# make install
```

# Running
## Hello world
```
$ shpp
> echo hello world
```
## Running a file
```
$ shpp file.shpp
```
