#pragma once

#include <random>

class WeightRandomizer {
public:
    static void randomize( float *values, int numValues, float minvalue = -0.1f, float maxvalue = 0.1f ) {
        std::mt19937 random;
        random.seed(0); // so always gives same results
        for( int i = 0; i < numValues; i++ ) {
            values[i] = random() / (float)std::mt19937::max() * (maxvalue-minvalue) - maxvalue;
        }
    }
    static void randomizeInts( float *values, int numValues, int minvalue = 0, int maxvalue = 9 ) {
        std::mt19937 random;
        random.seed(0); // so always gives same results
        for( int i = 0; i < numValues; i++ ) {
            values[i] = ( random() % (maxvalue-minvalue) ) - maxvalue;
        }
    }
};
