#pragma once

#include <iostream>
#include <string>

#include "OpenCLHelper.h"
#include "ActivationFunction.h"
#include "LayerDimensions.h"

#define STATIC static
#define VIRTUAL virtual

class BackpropWeights2 {
public:
    OpenCLHelper *cl;
    LayerDimensions dim;
    bool debug = false;

    virtual void backpropWeights( int batchSize, float learningRate, CLWrapper *derivLossBySumWrapper, CLWrapper *inputDataWrapper, CLWrapper *weightsWrapper, CLWrapper *biasWeightsWrapper ) = 0;

    // [[[cog
    // import cog_addheaders
    // cog_addheaders.add()
    // ]]]
    // classname: BackpropWeights2
    // cppfile: BackpropWeights2.cpp

    STATIC BackpropWeights2 *instance(OpenCLHelper *cl, LayerDimensions dim );
    STATIC BackpropWeights2 *instanceForTest(OpenCLHelper *cl, LayerDimensions layerDimensions );
    STATIC BackpropWeights2 *instanceSpecific( int idx, OpenCLHelper *cl, LayerDimensions layerDimensions );
    BackpropWeights2( OpenCLHelper *cl, LayerDimensions layerDimensions );
    VIRTUAL void backpropWeights( int batchSize, float learningRate, float *derivLossBySum, float *inputData, float *filters, float *biasWeights );
    float learningRateToMultiplier( int batchSize, float rate );

    // [[[end]]]
};

