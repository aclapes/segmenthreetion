//
//  main.cpp
//  Segmenthreetion
//
//  Created by Albert Clapés on 20/04/13.
//  Copyright (c) 2013 Albert Clapés. All rights reserved.
//

#include "TrimodalSegmentator.h"
#include "ColorParametrization.hpp"
#include "MotionParametrization.hpp"
#include "ThermalParametrization.hpp"
#include "DepthParametrization.hpp"
#include "StatTools.h"

#include <opencv2/opencv.hpp>

#include <iostream>

using namespace std;

int main(int argc, const char* argv[])
{
    //
    // Parametrization
    //
    
    // Dataset handling
    
    const unsigned char offsetID = 200;
    
    // Feature extraction
    
    const unsigned int hp = 2; // partitions in height
    const unsigned int wp = 2; // partitions in width
    
    ColorParametrization cParam;
    cParam.winSizeX = 64;
    cParam.winSizeY = 128;
    cParam.blockSizeX = 32;
    cParam.blockSizeY = 32;
    cParam.cellSizeX = 16;
    cParam.cellSizeY = 16;
    cParam.nbins = 9;
    
    MotionParametrization mParam;
    mParam.hoofbins = 8;
    mParam.pyr_scale = 0.5;
    mParam.levels = 3;
    mParam.winsize = 15;
    mParam.iterations = 3;
    mParam.poly_n = 5;
    mParam.poly_sigma = 1.2;
    mParam.flags = 0;
    
    DepthParametrization dParam;
    dParam.thetaBins        = 8;
    dParam.phiBins          = 8;
    dParam.normalsRadius    = 0.02;
    
    ThermalParametrization tParam;
    tParam.ibins    = 8;
    tParam.oribins  = 8;
    
    // Classification step
    
	int numMixtures = 3; // classification parameter (training step)
    
    // Validation procedure
    
    int k = 10; // number of folds of a cvpartition
    int seed = 74;
    
    
    //
    // Execution
    //
    
    TrimodalSegmentator tms(hp, wp, offsetID);
    
    tms.setDataPath("../../Sequences/");
    
    std::vector<GridMat> tGrids, tMasks; // the ones used to train
    cv::Mat tTags;
    GridMat tDescriptors;
    
    tms.getModalityData("Thermal", tGrids, tMasks, tTags);
    tms.extractThermalFeatures(tGrids, tMasks, tParam, tDescriptors);
    tGrids.clear();
    tMasks.clear();
    
    cv::Mat tPartitions;
    cvpartition(tTags, k, seed, tPartitions);
    
    GridMat tLogLikelihoods;
    tms.computeLogLikelihoods(tDescriptors, tTags, tLogLikelihoods);

    return 0;
}

