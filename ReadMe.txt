/*******************************************************************************
 * Copyright (C) 2020 ASH WARE, Inc.
 *******************************************************************************/

This package contains the eTPU driver source code and host layer driver source code 
for a SPI master and slave communication interface.


Tools Used
==========
ASH WARE System DevTool, Version 2.72D
ASH WARE ETEC eTPU C Compiler Toolkit, version 2.62D


Package Contents
================
The main directory structure of the package is as follows:
.                  - contains the top-level project and test application code.
.\etpu\_etpu_set   - eTPU API files and auto-generated code from eTPU code compilation.
.\etpu\_utils      - eTPU module host utility functions.
.\etpu\etpucode    - eTPU driver code (SPI master and slave functions).
.\etpu\spi         - host API code for SPI eTPU drivers.
.\include          - chip-specific config header files.


eTPU Code Size
=========
Compile and see etpu_set.map or etpu_ab_ana.html for details.


Change History
==============
