//
//  Validation.cpp
//  segmenthreetion
//
//  Created by Cristina Palmero Cantariño on 21/03/14.
//
//

#include "Validation.h"
#include "StatTools.h"


using namespace std;

Validation::Validation()
{ }

void Validation::getOverlap(vector<cv::Mat> predictedMasks, vector<cv::Mat> gtMasks, vector<int> dcRange, cv::Mat& overlapIDs) {
    
    cv::resize(overlapIDs, overlapIDs, cvSize(predictedMasks.size(), dcRange.size()+1));
    int idx = 0;
    for(int f = 0; f < predictedMasks.size(); f++)
    {
        if(cv::countNonZero(predictedMasks[f]) > 0 || cv::countNonZero(gtMasks[f]) > 0)
        {
            overlapIDs.at<float>(0, idx) = getMaskOverlap(predictedMasks[f], gtMasks[f], cv::Mat());
            
            for(int dc = 0; dc < dcRange.size(); dc++)
            {
                cv::Mat dontCare;
                createDontCareRegion(gtMasks[f], dontCare, dcRange[dc]);
                threshold(dontCare, dontCare, 128, 255, CV_THRESH_BINARY);
                
                overlapIDs.at<float>(dc+1, idx) = getMaskOverlap(predictedMasks[f], gtMasks[f], dontCare);
            }
            idx++;
        }
    }
    
    //TODO: Check if all iDs are finite or treat it when computing the mean (outside this function ?)
    
}

/*
 * Get overlap value between predicted mask and ground truth mask based on Jaccard Similarity/Index
 */
float Validation::getMaskOverlap(cv::Mat predictedMask, cv::Mat gtMask, cv::Mat dontCareRegion)
{
    bool useDontCare = false;
    if(!dontCareRegion.empty()) useDontCare = true;
    
    float overlap = 0.0;
    
    vector<int> gtMaskPersonId;
    findUniqueValues(gtMask, gtMaskPersonId);
    
    
    
    //TODO: continue...
    
    return overlap;
}

void Validation::createDontCareRegion(cv::Mat inputMask, cv::Mat& outputMask, float dcRange)
{
    
}