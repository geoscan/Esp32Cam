# Droptest

Dead-simple testing framework. Test components of your projects in a few easy
steps:

1. Drop this into your `test/` dir;
2. Invoke `make test TEST_NAME=<somename>_test`, do not forget the suffix `_test`, or it will not be added into the running queue;
3. Set up your testing environment `<somename>_test` through use of relative symlinks to your project's sources, or otherwise;
4. run `make run` to run all tests

# Requirements

C++11, Make, *nix, STL

# See also

- [One header debug](https://github.com/damurashov/One-header-debug)
