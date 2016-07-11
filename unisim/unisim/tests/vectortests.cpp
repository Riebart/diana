#include "UnitTest++/UnitTest++.h"
#include "../vector.hpp"

using namespace Diana;

SUITE(Vector3Ds)
{
    
    class Vector3DFixture
    {
    public:
        Vector3 v3a = {0, 0, 0};
        Vector3 v3b = {0, 0, 0};
        
        Vector3DFixture ()
        {
            //this->v3a = new Vector3();
            //this->v3b = new Vector3();
        }
    };
    
    //check if a zeero vector is, in fact, zero (or thereabouts)
    TEST_FIXTURE(Vector3DFixture, ZeroVector)
    {
        CHECK(v3a.almost_zero());

    }
    
    //check if two zero vectors are equal
    TEST_FIXTURE(Vector3DFixture, CheckZeroEquality)
    {
        
        CHECK_CLOSE(v3a.distance(v3b), 0.0, 0.000000000001);
        CHECK_CLOSE(v3b.distance(v3a), 0.0, 0.000000000001);
        
        Vector3 v3c = v3a-v3b;
        CHECK(v3c.almost_zero());
        
        //Fails
        //CHECK( (v3a - v3b).almost_zero());
        
        v3a-=v3b;
        CHECK( v3a.almost_zero() );
    }
    
}




TEST(Sanity)
{
    CHECK_EQUAL(1, 1);
}

int main(int, const char *[])
{
    return UnitTest::RunAllTests();
}

