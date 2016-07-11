#include "UnitTest++/UnitTest++.h"
#include "../physics.hpp"

#define ERROR_MARG 0.000000000001
#define NUM_SPECTRUM 5

using namespace Diana;

SUITE(Physics)
{
    class PhysicsFixture
    {
    public:
        
        struct Spectrum * dummySpectrum;
        
        PhysicsFixture ()
        {
            dummySpectrum = Spectrum_allocate(NUM_SPECTRUM);
            for (int i=0; i<NUM_SPECTRUM; i++)
            {
                (&(dummySpectrum->components))[i].wavelength = i;
                (&(dummySpectrum->components))[i].power = i;
            }
        }
    };
    
    TEST_FIXTURE(PhysicsFixture, HugeSpectrum)
    {
        CHECK_THROW(Spectrum_allocate(0xFFFFFFFF), std::runtime_error);
    }
        
    TEST_FIXTURE(PhysicsFixture, testSpectrum)
    {
        CHECK_CLOSE(total_spectrum_power(dummySpectrum), 10, ERROR_MARG);
        
        //Capture the value, then test against (for regressions)
        //printf ("%0.18f\n", radiates_strong_enough(dummySpectrum, 0.0001));
        CHECK_CLOSE(radiates_strong_enough(dummySpectrum, 0.01), 79.577471545948, ERROR_MARG);
        CHECK_CLOSE(radiates_strong_enough(dummySpectrum, 0.0001), 7957.7471545947673803, ERROR_MARG);
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