#include "catch/catch.hpp"

#include "libloomw/resourcem.h"

using namespace loom;

template<typename T>
static bool check_uvector(const std::vector<T> &v, const std::vector<T> &s)
{
   std::set<T> s1(v.begin() , v.end());
   std::set<T> s2(s.begin() , s.end());
   return s1 == s2;
}


TEST_CASE("resourcem-alloc-1cpu", "[resourcem]") {
    loom::ResourceManager rm;
    rm.init(1);
    bool r;

    SECTION("Alloc zero-cost") {
        ResourceAllocation ra = rm.allocate(0);
        REQUIRE(ra.is_valid());
        REQUIRE(ra.get_cpus().size() == 0);

        ResourceAllocation rb = rm.allocate(0);
        REQUIRE(rb.is_valid());
        REQUIRE(rb.get_cpus().size() == 0);

        ResourceAllocation rc = rm.allocate(0);
        REQUIRE(rc.is_valid());
        REQUIRE(rc.get_cpus().size() == 0);

        ResourceAllocation rd = rm.allocate(0);
        REQUIRE(!rd.is_valid());

        rm.free(ra);
        REQUIRE(!ra.is_valid());

        rd = rm.allocate(0);
        REQUIRE(rd.is_valid());
    }

    SECTION("Alloc 1cpu") {
        ResourceAllocation ra = rm.allocate(1);
        REQUIRE(ra.is_valid());
        REQUIRE(ra.get_cpus() == std::vector<int>{0});

        ResourceAllocation rb = rm.allocate(1);
        REQUIRE(!rb.is_valid());
        REQUIRE(rb.get_cpus().empty());

        rm.free(ra);
        REQUIRE(!ra.is_valid());

        rb = rm.allocate(1);
        REQUIRE(rb.is_valid());
        REQUIRE(rb.get_cpus() == std::vector<int>{0});
    }
}


TEST_CASE("resourcem-alloc-2cpu", "[resourcem]") {
    loom::ResourceManager rm;
    rm.init(2);
    bool r;

    SECTION("Alloc 1cpu") {
        ResourceAllocation ra = rm.allocate(1);
        REQUIRE(ra.is_valid());

        ResourceAllocation rb = rm.allocate(1);
        REQUIRE(ra.is_valid());

        REQUIRE(ra.get_cpus().size() == 1);
        REQUIRE(rb.get_cpus().size() == 1);
        REQUIRE(ra.get_cpus()[0] != rb.get_cpus()[0]);

        rm.free(ra);
        REQUIRE(!ra.is_valid());
        rm.free(rb);
        REQUIRE(!rb.is_valid());

        ra = rm.allocate(2);
        REQUIRE(ra.is_valid());
        REQUIRE(check_uvector(ra.get_cpus(), {0, 1}));

        rb = rm.allocate(1);
        REQUIRE(!rb.is_valid());

        rm.free(ra);
        REQUIRE(!ra.is_valid());

        rb = rm.allocate(2);
        REQUIRE(rb.is_valid());
        REQUIRE(check_uvector(rb.get_cpus(), {0, 1}));
    }
}


TEST_CASE("taskset", "[resourcem]") {
    std::vector<int> cpus3 = {1, 3, 5};
    std::vector<std::string> expected3 = { "taskset", "-c", "1,3,5" };
    std::vector<int> cpus1 = {123};
    std::vector<std::string> expected1 = { "taskset", "-c", "123" };

    REQUIRE(expected1 == loom::taskset_command(cpus1));
    REQUIRE(expected3 == loom::taskset_command(cpus3));
    REQUIRE(loom::taskset_command({}).empty());
}
