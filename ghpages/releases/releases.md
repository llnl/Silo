---
title: Releases
---

Silo source code is released under the BSD 3-clause license.

Two compression libraries, [fpzip](https://computing.llnl.gov/projects/fpzip) and [hzip](https://computing.llnl.gov/projects/hzip) which are part of the release artifact and can be built-in to Silo, are not BSD licensed.
As of version 4.12.0, whether a BSD licensed or LLNL legacy licensed version is built can be specified by the `-DSILO_BUILD_FOR_BSD_LICENSE:BOOL` CMake option.

For details, see the [BSD_LICENSE_README](https://raw.githubusercontent.com/LLNL/Silo/main/BSD_LICENSE_README) file.
<br>

Date | Version | Sources | Documentation
:--- | ---:    | :---        | ---
Jul 2026 | 4.12.1 | [Legacy+BSD][4121] | [Release notes][4121] / [User Manual][4121um]
Nov 2025 | 4.12.0 | [Legacy+BSD][4120] | [Release notes][4120] / [User Manual][4120um]
Sep 2023 | 4.11.1 | [Legacy][4111] / [BSD][4111b] | [Release notes][4111rn] / [User Manual][4111um]
Sep 2022 | 4.11 | [Legacy][411] / [BSD][411b] | [Release notes][411rn]
Feb 2016 | 4.10.2 | [Legacy][4102] / [BSD][4102b] | [See 4.10.1 Release notes][4102rn]
Feb 2016 | 4.10.1 | Not available | Release was DOA
Jul 2014 | 4.10 | [Legacy][410] / [BSD][410b] | [Release notes][410rn] / [User Manual][410um]
Jun 2013 | 4.9.1  | [Legacy][491] / [BSD][491b] | [Release notes][491rn]
Mar 2012 | 4.9 | [Legacy][49] / [BSD][49b] | [Release notes][49rn]
Aug 2010 | 4.8 | [Legacy][48] / [BSD][48b] | [Release notes][48rn] / [User Manual][48um]
Nov 2009 | 4.7.2 | [Legacy][472] | [Release notes][472rn] / [User Manual][471um]
Nov 2009 | 4.7.1 | Not available | Release was DOA
Jan 2009 | 4.7 | [Legacy][47] | [User Manual][47um]
Jul 2008 | 4.6.2 | [Legacy][462] | [User Manual][462um]
Oct 2007 | 4.6.1 | [Legacy][461] | [User Manual][46um]

<!-- release asset links -->

[4121um]: https://silo.readthedocs.io/4.12.1/
[4121]: https://github.com/LLNL/Silo/releases/tag/4.12.1 "Legacy+BSD License"
[4120um]: https://silo.readthedocs.io/4.12.0/
[4120]: https://github.com/LLNL/Silo/releases/tag/4.12.0 "Legacy+BSD License"
[4111]: https://github.com/LLNL/Silo/releases/download/4.11.1/silo-4.11.1.tar.xz "Legacy License but includes all features"
[4111b]: https://github.com/LLNL/Silo/releases/download/4.11.1/silo-4.11.1-bsd.tar.xz "BSD License, fpzip/hzip compressors removed"
[4111rn]: https://github.com/LLNL/Silo/releases/tag/4.11.1
[4111um]: https://silo.readthedocs.io// 
[411]: https://github.com/LLNL/Silo/releases/download/v4.11/silo-4.11.tar.gz "Legacy License but includes all features"
[411b]: https://github.com/LLNL/Silo/releases/download/v4.11/silo-4.11-bsd.tar.gz "BSD License, fpzip/hzip compressors removed"
[411rn]:https://github.com/LLNL/Silo/wiki/4.11-Release-Notes-September,-2021
[4102]: silo-4.10.2.tar.xz "Legacy License but includes all features"
[4102b]: silo-4.10.2-bsd.tar.xz "BSD License, fpzip/hzip compressors removed"
[4102rn]: silo_relnotes_4.10.1.html
[410]: silo-4.10.tar.xz "Legacy License but includes all features"
[410b]: silo-4.10-bsd.tar.xz "BSD License, fpzip/hzip compressors removed"
[410rn]: silo_relnotes_4.10.html
[410um]: LLNL-SM-654357.pdf
[491]: silo-4.9.1.tar.xz "Legacy License but includes all features"
[491b]: silo-4.9.1-bsd.tar.xz "BSD License, fpzip/hzip compressors removed"
[491rn]: silo_relnotes_4.9.1.html
[49]: silo-4.9.tar.xz "Legacy License but includes all features"
[49b]: silo-4.9-bsd.tar.xz "BSD License, fpzip/hzip compressors removed"
[49rn]: silo_relnotes_4.9.html
[48]: silo-4.8.tar.xz "Legacy License but includes all features"
[48b]: silo-4.8-bsd.tar.xz "BSD License, fpzip/hzip compressors removed"
[48rn]: silo_relnotes_4.8.html
[48um]: LLNL-SM-453191.pdf
[472]: silo-4.7.2.tar.xz "Legacy License but includes all features"
[472rn]: silo_relnotes_4.7.2.html
[471um]: LLNL-SM-421083.pdf
[47]: silo-4.7.tar.xz "Legacy License but includes all features"
[47um]: LLNL-SM-410226.pdf
[462]: silo-4.6.2.tar.xz "Legacy License but includes all features"
[462um]: LLNL-SM-406516.pdf
[461]: silo-4.6.1.tar.xz "Legacy License but includes all features"
[46um]: LLNL-SM-401406.pdf
