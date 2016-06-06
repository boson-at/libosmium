#include "catch.hpp"

#include <sstream>
#include <type_traits>

#include <osmium/osm/location.hpp>

TEST_CASE("Location") {

// fails on MSVC and doesn't really matter
// static_assert(std::is_literal_type<osmium::Location>::value, "osmium::Location not literal type");

    SECTION("instantiation_with_default_parameters") {
        osmium::Location loc;
        REQUIRE(!loc);
        REQUIRE_THROWS_AS(loc.lon(), osmium::invalid_location);
        REQUIRE_THROWS_AS(loc.lat(), osmium::invalid_location);
    }

    SECTION("instantiation_with_double_parameters") {
        osmium::Location loc1(1.2, 4.5);
        REQUIRE(!!loc1);
        REQUIRE(12000000 == loc1.x());
        REQUIRE(45000000 == loc1.y());
        REQUIRE(1.2 == loc1.lon());
        REQUIRE(4.5 == loc1.lat());

        osmium::Location loc2(loc1);
        REQUIRE(4.5 == loc2.lat());

        osmium::Location loc3 = loc1;
        REQUIRE(4.5 == loc3.lat());
    }

    SECTION("instantiation_with_double_parameters_constructor_with_universal_initializer") {
        osmium::Location loc { 2.2, 3.3 };
        REQUIRE(2.2 == loc.lon());
        REQUIRE(3.3 == loc.lat());
    }

    SECTION("instantiation_with_double_parameters_constructor_with_initializer_list") {
        osmium::Location loc({ 4.4, 5.5 });
        REQUIRE(4.4 == loc.lon());
        REQUIRE(5.5 == loc.lat());
    }

    SECTION("instantiation_with_double_parameters_operator_equal") {
        osmium::Location loc = { 5.5, 6.6 };
        REQUIRE(5.5 == loc.lon());
        REQUIRE(6.6 == loc.lat());
    }

    SECTION("equality") {
        osmium::Location loc1(1.2, 4.5);
        osmium::Location loc2(1.2, 4.5);
        osmium::Location loc3(1.5, 1.5);
        REQUIRE(loc1 == loc2);
        REQUIRE(loc1 != loc3);
    }

    SECTION("order") {
        REQUIRE(osmium::Location(-1.2, 10.0) < osmium::Location(1.2, 10.0));
        REQUIRE(osmium::Location(1.2, 10.0) > osmium::Location(-1.2, 10.0));

        REQUIRE(osmium::Location(10.2, 20.0) < osmium::Location(11.2, 20.2));
        REQUIRE(osmium::Location(10.2, 20.2) < osmium::Location(11.2, 20.0));
        REQUIRE(osmium::Location(11.2, 20.2) > osmium::Location(10.2, 20.0));
    }

    SECTION("validity") {
        REQUIRE(osmium::Location(0.0, 0.0).valid());
        REQUIRE(osmium::Location(1.2, 4.5).valid());
        REQUIRE(osmium::Location(-1.2, 4.5).valid());
        REQUIRE(osmium::Location(-180.0, -90.0).valid());
        REQUIRE(osmium::Location(180.0, -90.0).valid());
        REQUIRE(osmium::Location(-180.0, 90.0).valid());
        REQUIRE(osmium::Location(180.0, 90.0).valid());

        REQUIRE(!osmium::Location(200.0, 4.5).valid());
        REQUIRE(!osmium::Location(-1.2, -100.0).valid());
        REQUIRE(!osmium::Location(-180.0, 90.005).valid());
    }


    SECTION("output_to_iterator_comma_separator") {
        char buffer[100];
        osmium::Location loc(-3.2, 47.3);
        *loc.as_string(buffer, ',') = 0;
        REQUIRE(std::string("-3.2,47.3") == buffer);
    }

    SECTION("output_to_iterator_space_separator") {
        char buffer[100];
        osmium::Location loc(0.0, 7.0);
        *loc.as_string(buffer, ' ') = 0;
        REQUIRE(std::string("0 7") == buffer);
    }

    SECTION("output_to_iterator_check_precision") {
        char buffer[100];
        osmium::Location loc(-179.9999999, -90.0);
        *loc.as_string(buffer, ' ') = 0;
        REQUIRE(std::string("-179.9999999 -90") == buffer);
    }

    SECTION("output_to_iterator_undefined_location") {
        char buffer[100];
        osmium::Location loc;
        REQUIRE_THROWS_AS(loc.as_string(buffer, ','), osmium::invalid_location);
    }

    SECTION("output_to_string_comman_separator") {
        std::string s;
        osmium::Location loc(-3.2, 47.3);
        loc.as_string(std::back_inserter(s), ',');
        REQUIRE(s == "-3.2,47.3");
    }

    SECTION("output_to_string_space_separator") {
        std::string s;
        osmium::Location loc(0.0, 7.0);
        loc.as_string(std::back_inserter(s), ' ');
        REQUIRE(s == "0 7");
    }

    SECTION("output_to_string_check_precision") {
        std::string s;
        osmium::Location loc(-179.9999999, -90.0);
        loc.as_string(std::back_inserter(s), ' ');
        REQUIRE(s == "-179.9999999 -90");
    }

    SECTION("output_to_string_undefined_location") {
        std::string s;
        osmium::Location loc;
        REQUIRE_THROWS_AS(loc.as_string(std::back_inserter(s), ','), osmium::invalid_location);
    }

    SECTION("output_defined") {
        osmium::Location p(-3.20, 47.30);
        std::stringstream out;
        out << p;
        REQUIRE(out.str() == "(-3.2,47.3)");
    }

    SECTION("output_undefined") {
        osmium::Location p;
        std::stringstream out;
        out << p;
        REQUIRE(out.str() == "(undefined,undefined)");
    }

}

TEST_CASE("Location hash") {
    if (sizeof(size_t) == 8) {
        REQUIRE(std::hash<osmium::Location>{}({0, 0}) == 0);
        REQUIRE(std::hash<osmium::Location>{}({0, 1}) == 1);
        REQUIRE(std::hash<osmium::Location>{}({1, 0}) == 0x100000000);
        REQUIRE(std::hash<osmium::Location>{}({1, 1}) == 0x100000001);
    } else {
        REQUIRE(std::hash<osmium::Location>{}({0, 0}) == 0);
        REQUIRE(std::hash<osmium::Location>{}({0, 1}) == 1);
        REQUIRE(std::hash<osmium::Location>{}({1, 0}) == 1);
        REQUIRE(std::hash<osmium::Location>{}({1, 1}) == 0);
    }
}

#define C(s, v) REQUIRE(osmium::detail::string_to_location_coordinate(s)     == v); \
                REQUIRE(osmium::detail::string_to_location_coordinate("-" s) == -v); \
                REQUIRE(atof(s)     == Approx( v / 10000000.0)); \
                REQUIRE(atof("-" s) == Approx(-v / 10000000.0));

#define F(s) REQUIRE_THROWS_AS(osmium::detail::string_to_location_coordinate(s),     osmium::invalid_location); \
             REQUIRE_THROWS_AS(osmium::detail::string_to_location_coordinate("-" s), osmium::invalid_location);

TEST_CASE("Parsing coordinates from strings") {
    F("x");
    F(".");
    F("--");
    F("");
    F(" ");
    F(" 123");
    F("123 ");
    F("123x");
    F("1.2x");

    C("0", 0);

    C("1",       10000000);
    C("2",       20000000);

    C("9",       90000000);
    C("10",     100000000);
    C("11",     110000000);

    C("90",     900000000);
    C("100",   1000000000);
    C("101",   1010000000);

    C("00",             0);
    C("01",      10000000);
    C("001",     10000000);

    F("0001");
    F("1234");
    F("1234.");

    C("0.",             0);
    C("0.0",            0);
    C("1.",      10000000);
    C("1.0",     10000000);
    C("1.2",     12000000);
    C("0.1",      1000000);

    C("1.1234567",  11234567);
    C("1.12345670", 11234567);
    C("1.12345674", 11234567);
    C("1.12345675", 11234568);
    C("1.12345679", 11234568);
    C("1.12345680", 11234568);
    C("1.12345681", 11234568);

    C("180.0000000",  1800000000);
    C("180.0000001",  1800000001);
    C("179.9999999",  1799999999);
    C("179.99999999", 1800000000);
    C("200.123",      2001230000);

}

#undef C
#undef F

#define CW(v, s) buffer.clear(); \
                 osmium::detail::append_location_coordinate_to_string(std::back_inserter(buffer), v); \
                 CHECK(buffer == s); \
                 buffer.clear(); \
                 osmium::detail::append_location_coordinate_to_string(std::back_inserter(buffer), -v); \
                 CHECK(buffer == "-" s);

TEST_CASE("Writing coordinates into string") {
    std::string buffer;

    osmium::detail::append_location_coordinate_to_string(std::back_inserter(buffer), 0);
    CHECK(buffer == "0");

    CW(  10000000, "1");
    CW(  90000000, "9");
    CW( 100000000, "10");
    CW(1000000000, "100");
    CW(2000000000, "200");

    CW(   1000000, "0.1");
    CW(   1230000, "0.123");
    CW(   9999999, "0.9999999");
    CW(  40101010, "4.010101");
    CW( 494561234, "49.4561234");
    CW(1799999999, "179.9999999");
}

#undef CW

