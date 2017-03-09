#define CATCH_CONFIG_RUNNER
#include "catch/catch.hpp"
#include "libloom/log.h"

int main( int argc, char* argv[] )
{
  spdlog::set_level(spdlog::level::err);
  int result = Catch::Session().run( argc, argv );
  return (result < 0xff ? result : 0xff);
}
