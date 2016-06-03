#define CATCH_CONFIG_MAIN
#include <iostream>
#include <catch.hpp>
#include "ezcurl.h"

using namespace std;


TEST_CASE("test", "") {
    SECTION("s") {
        ezcurl::Curl cl;
        string blob;
        cl.get("http://www.google.com/", &blob);
        cout << blob << endl;
    }
}
