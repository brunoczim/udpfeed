# Overview

Source code is divided in three modules:
- `client`
- `server`
- `shared` (code shared between client and server)

Besides that, there's also a test suite.

# Building

To build the application client and server executables, simply run:

```sh
make
```

To clear the build in case of (for example) inconsistencies, run:

```sh
make clean
```

# Running

## Client

To run the client, this is the interface:

```sh
./app_client <username> <server-address> <server-port>
```

## Server

To run the server, this is the interface:

```sh
./app_server <bind-address> <bind-port>
```

# Testing

## Run All Tests

To run all tests, run this command:

```sh
make test
```

Currently, there is no way to run a single test isolated.

## Writing a Test

Depending on the module you want to test, you should edit either
`src/test/shared.cpp`, `src/test/client.cpp` or `src/test/server.cpp`.
Inside one of these files, create or update the test suite for the submodule you
want to test. Adding a test to the test suite is simply:

```c++
.test("test identifier", [] {
    // test code goes here
})
```

To assert a condition holds, use the `TEST_ASSERT` macro:

```c++
TEST_ASSERT("message in case of failure", my_condition());
```

To create a test suite, please take a look into the existing test suites
(`src/test/shared.cpp` surely contains test suites as examples).
