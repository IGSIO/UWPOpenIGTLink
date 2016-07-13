# UWPOpenIGTLink
This project implements a client for OpenIGTLink implemented for the Universal Windows Platform. Currently the client is read-only in that it does not send any messages.

# Features
* Currently the [TRACKEDFRAME](https://app.assembla.com/spaces/plus/subversion/source/HEAD/trunk/PlusLib/src/PlusOpenIGTLink/igtlPlusTrackedFrameMessage.h) message specified by the PLUS project is the only message type that is received.
* The UI project creates a simple 2D UWP application that receives image and transform data and displays it. At the moment, only a single slice can be visualized in the case of volumetric data.

# Authors
* [Adam Rankin](http://www.imaging.robarts.ca/petergrp/node/113), [Robarts Research Institute](http://www.imaging.robarts.ca/petergrp/), [Western University](http://www.uwo.ca)

# Issues
Please report issues using the [Issues](../../issues) tool.

# License
This project is released under the [Apache 2 license](LICENSE.md).
