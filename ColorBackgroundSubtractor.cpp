//
//  ColorBackgroundSubtractor.cpp
//  segmenthreetion
//
//  Created by Cristina Palmero Cantariño on 07/03/14.
//
//

#include "BackgroundSubtractor.h"
#include "ColorBackgroundSubtractor.h"

#include <opencv2/opencv.hpp>


ColorBackgroundSubtractor::ColorBackgroundSubtractor()
: BackgroundSubtractor()
{ }


/*
void ColorBackgroundSubtractor::setMasksOffset(unsigned char masksOffset) {
    m_masksOffset = masksOffset;
}
 */


void ColorBackgroundSubtractor::getMasks(ModalityData& mdOutput, ModalityData& mdInput) {
    
    mdOutput.setPredictedMasks(mdInput.getPredictedMasks());
    
}

void ColorBackgroundSubtractor::getBoundingRects(ModalityData& mdOutput, ModalityData& mdInput) {

    mdOutput.setPredictedBoundingRects(mdInput.getPredictedBoundingRects());
    
    cout << "Color bounding boxes: " << this->countBoundingBoxes(mdOutput.getPredictedBoundingRects()) << endl;

}

void ColorBackgroundSubtractor::adaptGroundTruthToReg(ModalityData& mdOutput, ModalityData& mdInput) {
    
    mdOutput.setGroundTruthMasks(mdInput.getGroundTruthMasks());
    
}

void ColorBackgroundSubtractor::getRoiTags(ModalityData& mdOutput, ModalityData& mdInput) {
    
    mdOutput.setTags(mdInput.getTags());
}

void ColorBackgroundSubtractor::getGroundTruthBoundingRects(ModalityData& mdOutput, ModalityData& mdInput) {
    
    mdOutput.setGroundTruthBoundingRects(mdInput.getGroundTruthBoundingRects());
    
}