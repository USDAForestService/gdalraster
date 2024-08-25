# Welcome

We're glad you're thinking about contributing to this open source project of the USDA Forest Service, Rocky Montain Research Station. If you're unsure about anything, just ask -- or submit an issue or pull request anyway. The worst that can happen is you'll be politely asked to change something. We welcome all friendly contributions.

## Development practices

[EditorConfig](https://editorconfig.org/) is in use in the development environment and a .editorconfig file is included in the repository. This will automatically tell popular code editors about the basic style settings like indentation, whitespaces and end-of-line markers for distinguished types of plain text files.

[lintr](https://lintr.r-lib.org/) is in use in the development environment for static analysis of R code, and a .lintr file is included in the repository.

[cppcheck](https://cppcheck.sourceforge.io/) is in use for static analysis of C++ code. **cppcheck** can be installed with various package managers (e.g., `sudo apt-get install cppcheck`), and may be used from the command-line. It is also integrated with many popular development tools including a [VSCode plugin](https://marketplace.visualstudio.com/items?itemName=NathanJ.cppcheck-plugin).


All new functionality must include tests added to the automated test suite under `tests/testthat/`. Documentation for **testthat** is available at [https://testthat.r-lib.org/].

Dynamic analysis of C++ code with [Valgrind](https://valgrind.org/) and [Clang Address Sanitizer](https://clang.llvm.org/docs/MemorySanitizer.html) is done regularly during development, and before any new version release. These analyses are performed in GitHub Actions using the workflows provided by [rhub](https://github.com/r-hub/rhub/).


## Policies

We want to ensure a welcoming environment for all of our projects. Our staff follow the General Services Administration (GSA)’s Technology Transformation Services (TTS) [Code of Conduct](https://18f.gsa.gov/code-of-conduct/) and all contributors should do the same.

## Public domain

This project is released under [MIT license](https://cran.r-project.org/web/licenses/MIT). Portions of the project are in the public domain within the United States, with copyright and related rights waived worldwide through the [CC0 1.0 Universal public domain dedication](https://creativecommons.org/publicdomain/zero/1.0/). See file inst/COPYRIGHTS for details.
