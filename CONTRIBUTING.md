# Welcome

We're glad you're thinking about contributing to this open source project of the USDA Forest Service, Rocky Montain Research Station. If you're unsure about anything, just ask -- or submit an issue or pull request anyway. The worst that can happen is you'll be politely asked to change something. We welcome all friendly contributions.

## Policies

We want to ensure a welcoming environment for all of our projects. Our staff follow the General Services Administration (GSA)’s Technology Transformation Services (TTS) [Code of Conduct](https://18f.gsa.gov/code-of-conduct/) and all contributors should do the same.

## Public domain

This project is released under [MIT license](https://cran.r-project.org/web/licenses/MIT). Portions of the project are in the public domain within the United States, with copyright and related rights waived worldwide through the [CC0 1.0 Universal public domain dedication](https://creativecommons.org/publicdomain/zero/1.0/). See file inst/COPYRIGHTS for details.

## Development practices

[EditorConfig](https://editorconfig.org/) is in use in the development environment and a `.editorconfig` file is included in the repository. This will automatically tell popular code editors about the basic style settings like indentation, whitespaces and end-of-line markers for distinguished types of plain text files.

[lintr](https://lintr.r-lib.org/) is in use in the development environment for static analysis of R code, and a `.lintr` file is included in the repository.

[Cpplint](https://github.com/cpplint/cpplint) is in use in the development environment for C++ code style checking. A [VSCode plugin](https://marketplace.visualstudio.com/items?itemName=mine.cpplint) is available.

**cppcheck** is also used for static analysis of C++ code. It can be installed with various package managers, e.g., `sudo apt-get install cppcheck` on Debian/Ubuntu. Documentation, source code and a Windows installer are available on the [cppcheck website](https://cppcheck.sourceforge.io/).

All new functionality must include tests added to the automated test suite under `tests/testthat/`. Documentation for **testthat** is available at https://testthat.r-lib.org/.

Dynamic analysis of C++ code using [Valgrind](https://valgrind.org/) and [Clang Address Sanitizer](https://clang.llvm.org/docs/MemorySanitizer.html) is done regularly during development, and before any new version release. These analyses are performed in GitHub Actions using the workflows provided by [rhub](https://github.com/r-hub/rhub/).

