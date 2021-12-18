#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow" // required because asio will trigger this warning on macOS builds
#include "asio.hpp"
#pragma GCC diagnostic pop

asio::io_context test;