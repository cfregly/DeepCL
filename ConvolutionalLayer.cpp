// Copyright Hugh Perkins 2014 hughperkins at gmail
//
// This Source Code Form is subject to the terms of the Mozilla Public License, 
// v. 2.0. If a copy of the MPL was not distributed with this file, You can 
// obtain one at http://mozilla.org/MPL/2.0/.

#include "ConvolutionalLayer.h"
#include "NeuralNet.h"
#include "stringhelper.h"
#include "Propagate.h"
#include "WeightsHelper.h"
//#include "BackpropErrors.h"
#include "BackpropWeights2.h"

using namespace std;

#undef VIRTUAL
#define VIRTUAL 

ConvolutionalLayer::ConvolutionalLayer( Layer *previousLayer, ConvolutionalMaker const*maker ) :
        Layer( previousLayer, maker ),
//        filterSize( maker->_filterSize ),
//        filterSizeSquared( filterSize * filterSize ),
//        padZeros( maker->_padZeros ),
        weightsWrapper( 0 ),
        resultsWrapper( 0 ),
//        errorsForUpstreamWrapper( 0 ),
//        errorsForUpstream( 0 ),
        allocatedSpaceNumExamples( 0 ),
        resultsCopiedToHost( false ),
        results(0),
        weights(0),
        biasWeights(0),
//        errorsForUpstreamCopiedToHost( false ),
        cl( maker->net->getCl() ),
        batchSize( batchSize ),
        activationFunction( maker->getActivationFunction() ) {
    dim.setInputPlanes( previousLayer->getOutputPlanes() )
        .setInputBoardSize( previousLayer->getOutputBoardSize() )
        .setNumFilters( maker->_numFilters )
        .setFilterSize( maker->_filterSize )
        .setBiased( maker->_biased )
        .setPadZeros( maker->_padZeros );
        if( dim.padZeros && dim.filterSize % 2 == 0 ) {
            throw std::runtime_error("filter size must be an odd number, if padZeros is true, so either turn off padZeros, or choose a different filtersize :-)");
        }

//    dim = LayerDimensions( upstreamNumPlanes, upstreamBoardSize, 
//        numPlanes, filterSize, padZeros, biased );
    propagateimpl = Propagate::instance( cl, dim, activationFunction );
    backpropWeightsImpl = BackpropWeights2::instance( cl, dim );
//    backpropErrorsImpl = BackpropErrors::instance( cl, dim, activationFunction );

    if( dim.filterSize > dim.inputBoardSize ) {
            throw std::runtime_error("filter size cannot be larger than upstream board size: " + toString( dim.filterSize) +
                " > " + toString(dim.inputBoardSize) );
    }
    biasWeights = new float[ getBiasWeightsSize() ];
    weights = new float[ getWeightsSize() ];
//    std::cout << " convolutional layer " << layerIndex << " allocating weights size " << getWeightsSize() << std::endl;
    randomizeWeights();
    weightsWrapper = cl->wrap( getWeightsSize(), weights );
    weightsWrapper->copyToDevice();
}
VIRTUAL ConvolutionalLayer::~ConvolutionalLayer() {
    if( weightsWrapper != 0 ) {
        delete weightsWrapper;
    }
    if( resultsWrapper != 0 ) {
        delete resultsWrapper;
    }
    if( results != 0 ) {
        delete[] results;
    }
    if( weights != 0 ) {
        delete[] weights;
    }
    if( biasWeights != 0 ) {
        delete[] biasWeights;
    }
//    if( errorsForUpstreamWrapper != 0 ) {
//        delete errorsForUpstreamWrapper;
//    }
//    if( errorsForUpstream != 0 ) {
//        delete[] errorsForUpstream;
//    }
    delete propagateimpl;
    delete backpropWeightsImpl;
//    delete backpropErrorsImpl;
}
//VIRTUAL float *ConvolutionalLayer::getErrorsForUpstream() {
//    if( !errorsForUpstreamCopiedToHost ) {
//        std::cout << "copying errorsForUpstream to host, from GPU" << std::endl;
//        errorsForUpstreamWrapper->copyToHost();
//        errorsForUpstreamCopiedToHost = true;
//    }
//    return errorsForUpstream;
//}
VIRTUAL ActivationFunction const*ConvolutionalLayer::getActivationFunction() {
    return activationFunction;
}
VIRTUAL bool ConvolutionalLayer::providesDriveLossBySumWrapper() const {
    return true;
}
//VIRTUAL CLWrapper *ConvolutionalLayer::getErrorsForUpstreamWrapper() {
//    return errorsForUpstreamWrapper;
//}
VIRTUAL void ConvolutionalLayer::initWeights( float*weights ) {
    Layer::initWeights( weights );
    weightsWrapper->copyToDevice();
}
VIRTUAL float const *ConvolutionalLayer::getWeights() const {
    return weights;
}
VIRTUAL float *ConvolutionalLayer::getWeights() {
    return weights;
}
VIRTUAL int ConvolutionalLayer::getResultsSize() const {
    return batchSize * dim.outputCubeSize;
}
VIRTUAL int ConvolutionalLayer::getOutputPlanes() const {
    return dim.numFilters;
}
VIRTUAL int ConvolutionalLayer::getOutputBoardSize() const {
    return dim.outputBoardSize;
}
// filters are organized like [filterid][plane][row][col]
void ConvolutionalLayer::randomizeWeights() {
//        std::cout << "convolutional layer randomzing weights" << std::endl;
    int fanin = dim.inputPlanes * dim.filterSize * dim.filterSize;
    const int numThisLayerWeights = getWeightsSize();
    for( int i = 0; i < numThisLayerWeights; i++ ) {
        weights[i] = WeightsHelper::generateWeight( fanin );
    }
    for( int i = 0; i < dim.numFilters; i++ ) {
        biasWeights[i] = WeightsHelper::generateWeight( fanin );
    }
}
VIRTUAL bool ConvolutionalLayer::hasResultsWrapper() const {
    return true;
}
VIRTUAL CLWrapper *ConvolutionalLayer::getResultsWrapper() {
    return resultsWrapper;
}
VIRTUAL void ConvolutionalLayer::print() const {
    std::cout << "ConvolutionalLayer " << dim << std::endl;
    printWeights();
    if( results != 0 ) {
        printOutput();
    }
}
VIRTUAL void ConvolutionalLayer::printWeights() const {
    std::cout << "  weights: " << std::endl;
    getWeights();
// filters are organized like [filterid][plane][row][col]
    for( int filter = 0; filter < std::min( 5, dim.numFilters ); filter++ ) {
       std::cout << "    filter " << filter << std::endl;
       if( dim.biased ) {
           std::cout << "       bias=" << biasWeights[filter] << std::endl;            
       }
       for( int plane = 0; plane < std::min(5, dim.inputPlanes); plane++ ) {
           if( dim.inputPlanes > 1 ) std::cout << "    inplane " << plane << std::endl;
            for( int i = 0; i < std::min(5, dim.filterSize); i++ ) {
                std::cout << "      ";
                for( int j = 0; j < std::min(5, dim.filterSize); j++ ) {
                   std::cout << getWeight( filter, plane, i, j ) << " ";
                }
                if( dim.filterSize > 5 ) {
                   std::cout << " ...";
                }
                std::cout << std::endl;
            }
            if( dim.filterSize > 5 ) {
               std::cout << " ..." << std::endl;
            }
        }
        if( dim.inputPlanes > 5 ) std::cout << " ... other inplanes ... " << std::endl;
    }
    if( dim.numFilters > 5 ) std::cout << " ... other filters ... " << std::endl;
 }
VIRTUAL void ConvolutionalLayer::printOutput() const { 
    if( results == 0 ) {
        return;
    }
    //    getResults();
    std::cout << "  outputs: " << std::endl;
// results are organized like [imageid][filterid][row][col]
    for( int n = 0; n < std::min( 5, batchSize ); n++ ) {
        std::cout << "    n: " << n << std::endl;
        for( int plane = 0; plane < std::min(5, dim.numFilters ); plane++ ) {
            if( dim.numFilters > 1 ) std::cout << "      plane " << plane << std::endl;
            if( dim.outputBoardSize == 1 ) {
                 std::cout << "        " << getResult(n, plane, 0, 0 ) << std::endl;
            } else {
                for( int i = 0; i < std::min(5, dim.outputBoardSize); i++ ) {
                    std::cout << "      ";
                    for( int j = 0; j < std::min(5, dim.outputBoardSize); j++ ) {
                        std::cout << getResult( n, plane, i, j ) << " ";
                    }
                    if( dim.outputBoardSize > 5 ) std::cout << " ... ";
                    std::cout << std::endl;
                }
                if( dim.outputBoardSize > 5 ) std::cout << " ... " << std::endl;
            }
            if( dim.numFilters > 5 ) std::cout << " ... other planes ... " << std::endl;
        }
        if( batchSize > 5 ) std::cout << " ... other n ... " << std::endl;
    }
}
VIRTUAL void ConvolutionalLayer::setBatchSize( int batchSize ) {
    if( batchSize <= allocatedSpaceNumExamples ) {
        this->batchSize = batchSize;
        return;
    }
    if( results != 0 ) {
        delete[] results;
    }
    if( resultsWrapper != 0 ) {
        delete resultsWrapper;
    }
//    if( errorsForUpstream != 0 ) {
//        delete[] errorsForUpstream;
//    }
//    if( errorsForUpstreamWrapper != 0 ) {
//        delete errorsForUpstreamWrapper;
//    }
    this->batchSize = batchSize;
    results = new float[getResultsSize()];
    resultsWrapper = cl->wrap( getResultsSize(), results );
//        std::cout << " layer " << layerIndex << " allocating results size " << getResultsSize() << std::endl;
//    weOwnResults = true;
    if( layerIndex > 1 ) {
//        errorsForUpstream = new float[ previousLayer->getResultsSize() ];
//        errorsForUpstreamWrapper = cl->wrap( previousLayer->getResultsSize(), errorsForUpstream );
    }
    this->allocatedSpaceNumExamples = batchSize;
}
VIRTUAL void ConvolutionalLayer::propagate() {
//    if( boardSizeSquared <= cl->getMaxWorkgroupSize() ) {
////        propagate2();
//    } else {
//  //      propagate1();
//    }
//    propagate1();
    StatefulTimer::instance()->timeCheck("    propagate layer " + toString( layerIndex ) + ", START");

    CLWrapper *upstreamWrapper = 0;
    if( previousLayer->hasResultsWrapper() ) {
//            std::cout << "layer " << previousLayer->layerIndex << " has resultsWrapper" << std::endl;
        upstreamWrapper = previousLayer->getResultsWrapper();
    } else {
//            std::cout << "layer " << previousLayer->layerIndex << " has no resultsWrapper" << std::endl;
        upstreamWrapper = cl->wrap( previousLayer->getResultsSize(), (float *)previousLayer->getResults() );
        upstreamWrapper->copyToDevice();
    }
    CLFloatWrapper *biasWeightsWrapper = 0;
    if( dim.biased ) {
        biasWeightsWrapper = cl->wrap( getBiasWeightsSize(), biasWeights );
        biasWeightsWrapper->copyToDevice();
    }
    StatefulTimer::instance()->timeCheck("    propagate layer " + toString( layerIndex ) + ", copied to device");
    propagateimpl->propagate( batchSize, upstreamWrapper, weightsWrapper, biasWeightsWrapper, resultsWrapper );
    StatefulTimer::instance()->timeCheck("    propagate layer " + toString( layerIndex ) + ",  after clFinish");

    if( !previousLayer->hasResultsWrapper() ) {
        delete upstreamWrapper;
    }
    if( dim.biased ) {
        delete biasWeightsWrapper;
    }
    resultsCopiedToHost = false;
}
VIRTUAL float * ConvolutionalLayer::getResults() {
    if( !resultsCopiedToHost ) {
//            std::cout << "layer " << layerIndex << " copying results to host " << std::endl;
        resultsWrapper->copyToHost();
        resultsCopiedToHost = true;
    }
    return results;
};
VIRTUAL int ConvolutionalLayer::getWeightsSize() const {
    return dim.numFilters * dim.inputPlanes * dim.filterSize * dim.filterSize;
}
VIRTUAL int ConvolutionalLayer::getBiasWeightsSize() const {
    return dim.numFilters;
}

// weights:     [outPlane][upstreamPlane][filterRow][filterCol]
//       aggregate over:  [outRow][outCol][n]
// biasweights: [outPlane]
//       aggregate over:  [upstreamPlane][filterRow][filterCol][outRow][outCol][n]

VIRTUAL void ConvolutionalLayer::backProp( float learningRate ) {
//        Timer timer;
    StatefulTimer::instance()->timeCheck("backprop(): start, layer " + toString( layerIndex ) );

    CLWrapper *biasWeightsWrapper = 0;
    if( dim.biased ) {
        biasWeightsWrapper = cl->wrap( getBiasWeightsSize(), biasWeights );
        biasWeightsWrapper->copyToDevice();
    }

    CLWrapper *imagesWrapper = 0;
    if( previousLayer->hasResultsWrapper() ) {
        imagesWrapper = previousLayer->getResultsWrapper();
    } else {
        imagesWrapper = cl->wrap( previousLayer->getResultsSize(), previousLayer->getResults() );
        imagesWrapper->copyToDevice();
    }

    CLWrapper *derivLossBySumWrapper = 0;
    bool weOwnDerivLossBySumWrapper = false;
//    if( nextLayer->providesErrorsWrapper() ) {
//        derivLossBySumWrapper = nextLayer->getDerivLossBySumWrapper();
//    } else {
        derivLossBySumWrapper = cl->wrap( getResultsSize(), nextLayer->getDerivLossBySum() );
        derivLossBySumWrapper->copyToDevice();
        weOwnDerivLossBySumWrapper = true;
//    }
/* temporarily remove for now, till we at least get weight backprop working again :-)
    if( layerIndex > 1 ) {
        backpropErrorsImpl->backpropErrors( batchSize, resultsWrapper, weightsWrapper, biasWeightsWrapper, errorsWrapper, derivLossBySumWrapper );
        StatefulTimer::instance()->timeCheck("backproperrors(): calced errors for upstream, layer " + toString( layerIndex ) );
    }
*/
    backpropWeightsImpl->backpropWeights( batchSize, learningRate, derivLossBySumWrapper, imagesWrapper,   weightsWrapper, biasWeightsWrapper );
    StatefulTimer::instance()->timeCheck("backproperrors(): done weight backprop, layer " + toString( layerIndex ) );

    if( dim.biased ) {
        biasWeightsWrapper->copyToHost();
        delete biasWeightsWrapper;
    }
    if( !previousLayer->hasResultsWrapper() ) {
        delete imagesWrapper;
    }
    if( weOwnDerivLossBySumWrapper ) {
        delete derivLossBySumWrapper;
    }
    StatefulTimer::instance()->timeCheck("backproperrors(): updated weights, layer " + toString( layerIndex ) );
}

ostream &operator<<( ostream &os, ConvolutionalLayer &layer ) {
    os << "ConvolutionalLayer { " << layer.dim << " }";
    return os;
}

