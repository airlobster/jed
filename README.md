# jed: Command-line JSON editor

## Project Description
**jed** is a command-line JSON editor which allows adding, changing and deleting elements in a JSON stream read from STDIN.
The **jed** project is based on plain C language and can be build using the attached makefile.
All development has been done on both OSX and Linux. It hasn't been checked on Windows.

## Installation and Build
Just copy the entire project tree into a designated directory, and run **_make clean all_** from inside it.

## Help Information
Running **jed** with the **_--help_** option will print out all help information.

## Notes
**1.**	Embedded help information is taken from the _src/help_ file which gets included into the executable at build time
	using the **_tostring_** script which is provided with the project (see _makefile_).

