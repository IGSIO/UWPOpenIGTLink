# UWPOpenIGTLink
This project implements a client for OpenIGTLink implemented for the [Universal Windows Platform](https://docs.microsoft.com/en-us/windows/uwp/get-started/universal-application-platform-guide).

# Building
* UWPOpenIGTLink requires the OpenIGTLink library, which is a CMake'ified project. At the moment, UWPOpenIGTLink is hardcoded to search for headers in the folders `OpenIGTLink-bin-$(Platform)` where `$(Platform)` is either `Win32` or `x64`. Thus, you need to CMake the OpenIGTLink project into either of these two folders, depending on what architecture you want (or both).
  * So, build OpenIGTLink into `OpenIGTLink-bin-Win32` and/or `OpenIGTLink-bin-x64`
* Once OpenIGTLink is built, you can build the UWPOpenIGTLink solution as normal.

# Expected Usage
```c++

// Code samples

```

# Features
* Current supported messages are TRACKEDFRAME, TRANSFORM, TDATA, and COMMAND messages are supported.
* The UI project creates a simple 2D UWP application that receives image and transform data and displays it. At the moment, only a single slice can be visualized in the case of volumetric data.

# Authors
* [Adam Rankin](http://www.imaging.robarts.ca/petergrp/node/113), [Robarts Research Institute](http://www.imaging.robarts.ca/petergrp/), [Western University](http://www.uwo.ca)

# Issues
Please report issues using the [Issues](../../issues) tool.

# License
This project is released under the [Apache 2 license](LICENSE.md).
