# SPI-eTPU
This project is a SPI eTPU driver that includes many enhancements beyond the original NXP SPI eTPU driver. Both a SPI master and slave drivers are included.  The master supports all CPOL/CPHA setting combinations as well as optional slave/chip select control.  The slave driver also supports all CPOL/CPHA config combinations as well as an optional select line.  Both eTPU code and host API code are included.  Currently the code is the doucmentation, but it is a fairly straight-forward function.

This software is built and simulated/tested by the following tools:
- ETEC C Compiler for eTPU/eTPU2/eTPU2+, version 2.62D, ASH WARE Inc. (older versions ok, but not tested)
- System Development Tool, version 2.72D, ASH WARE Inc.

Use of or collaboration on this project is welcomed. For any questions please contact:

ASH WARE Inc. John Diener john.diener@ashware.com
