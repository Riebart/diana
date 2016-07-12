#include "UnitTest++/UnitTest++.h"
#include "../vector.hpp"

#define ERROR_MARG 0.000000000001

using namespace Diana;

SUITE(Vector3Ds)
{
    
    class Vector3DFixture
    {
    public:
        Vector3 v3a = {0, 0, 0};
        Vector3 v3b = {0, 0, 0};
        Vector3 v3unita = {1, 1, 1};
        Vector3 v3unitb = {1, 1, 1};
        
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
        CHECK_CLOSE(v3a.length(), 0.0, ERROR_MARG);

    }
    
    TEST_FIXTURE(Vector3DFixture, Addition)
    {
        
    }
    
    TEST_FIXTURE(Vector3DFixture, Subtraction)
    {
        CHECK_CLOSE( 0, (v3a-v3b).length(), ERROR_MARG);
        CHECK_CLOSE( 0, (v3a-v3a).length(), ERROR_MARG);
    }
    
    TEST_FIXTURE(Vector3DFixture, Dot)
    {
        CHECK_CLOSE(0, v3a.dot(v3b), ERROR_MARG);
        CHECK_CLOSE(0, v3b.dot(v3a), ERROR_MARG);
        
        CHECK_CLOSE(0, v3a.dot(v3unita), ERROR_MARG);
        CHECK_CLOSE(0, v3unita.dot(v3a), ERROR_MARG);
        
        CHECK_CLOSE(3, v3unita.dot(v3unitb), ERROR_MARG);
        CHECK_CLOSE(3, v3unitb.dot(v3unita), ERROR_MARG);
    }
    
    //run some tests on zero vectors
    TEST_FIXTURE(Vector3DFixture, CheckZeroVectors)
    {
        
        CHECK_CLOSE(0, v3a.distance(v3b), ERROR_MARG);
        CHECK_CLOSE(0, v3b.distance(v3a), ERROR_MARG);
                 
        CHECK( (v3a - v3b).almost_zero());
        Vector3 v3c = v3a-v3b;
        
        CHECK(v3c.almost_zero());
        
        v3a-=v3b;
        CHECK( v3a.almost_zero() );
    }
    
}


SUITE(Vector3Is)
{
    
    class Vector3IFixture
    {
    public:
        Vector3I v3a = {0, 0, 0};
        Vector3I v3b = {0, 0, 0};
        Vector3I v3unita = {1, 1, 1};
        Vector3I v3unitb = {1, 1, 1};
        
        Vector3IFixture ()
        {
            //this->v3a = new Vector3();
            //this->v3b = new Vector3();
        }
    };
    
    //check if a zeero vector is, in fact, zero
    TEST_FIXTURE(Vector3IFixture, ZeroVector)
    {
        CHECK(v3a.almost_zero());
        CHECK_EQUAL(0, v3a.length());

    }
    
    TEST_FIXTURE(Vector3IFixture, Addition)
    {
        CHECK_EQUAL(0, (v3a+v3b).length() );
        CHECK_EQUAL(0, (v3a+v3a).length() );
        
        CHECK_EQUAL(1, (v3a+v3unita).length() );
        
        CHECK_EQUAL(3, (v3unita+v3unitb).length() );
        CHECK_EQUAL(3, (v3unitb+v3unitb).length() );        
    }
    
    TEST_FIXTURE(Vector3IFixture, Subtraction)
    {
        
    }
    
    TEST_FIXTURE(Vector3IFixture, Dot)
    {
        CHECK_EQUAL(0, v3a.dot(v3b));
        CHECK_EQUAL(0, v3b.dot(v3a));
        
        CHECK_EQUAL(0, v3a.dot(v3unita));
        CHECK_EQUAL(0, v3unita.dot(v3a));
        
        CHECK_EQUAL(3, v3unita.dot(v3unitb));
        CHECK_EQUAL(3, v3unitb.dot(v3unita));
    }
    
    //run some tests on zero vectors
    TEST_FIXTURE(Vector3IFixture, CheckZeroEquality)
    {
        
        CHECK_EQUAL(0, v3a.distance(v3b));
        CHECK_EQUAL(0, v3b.distance(v3a));
                 
        CHECK_EQUAL(0, (v3a - v3b).length());
        Vector3I v3c = v3a-v3b;
        
        CHECK_EQUAL(0, v3c.length());
        CHECK_EQUAL(0, v3a.dot(v3b));
        CHECK_EQUAL(0, v3b.dot(v3a));
        
        v3a-=v3b;
        CHECK_EQUAL(0, v3a.length());
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

