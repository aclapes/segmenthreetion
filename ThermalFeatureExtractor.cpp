//
//  ThermalFeatureExtractor.cpp
//  Segmenthreetion
//
//  Created by Albert Clapés on 24/05/13.
//  Copyright (c) 2013 Albert Clapés. All rights reserved.
//

#include "FeatureExtractor.h"
#include "ThermalFeatureExtractor.h"

#include <opencv2/opencv.hpp>

#include <pcl/point_types.h>
#include <pcl/features/normal_3d.h>
#include <pcl/point_cloud.h>
#include <pcl/visualization/cloud_viewer.h>

#include <boost/thread.hpp>
#include <boost/timer.hpp>


ThermalFeatureExtractor::ThermalFeatureExtractor(int hp, int wp)
    : FeatureExtractor(hp, wp)
{ }


ThermalFeatureExtractor::ThermalFeatureExtractor(int hp, int wp, ThermalParametrization tParam)
    : FeatureExtractor(hp, wp), m_ThermalParam(tParam)
{ }


void ThermalFeatureExtractor::setParam(ThermalParametrization thermalParam)
{
    m_ThermalParam = thermalParam;
}


void ThermalFeatureExtractor::describe(vector<GridMat> grids, vector<GridMat> gmasks, vector<cv::Mat> gtags, GridMat & descriptors, GridMat & tags)
{
    //    namedWindow("god");
    
    for (int k = 0; k < grids.size(); k++)
    {
        GridMat & grid = grids[k];
        GridMat & gmask = gmasks[k];
        cv::Mat & gtag = gtags[k];
        
        for (int i = 0; i < grid.crows(); i++) for (int j = 0; j < grid.ccols(); j++)
        {
            cv::Mat & cell = grid.get(i,j);
            cv::Mat & cellMask = gmask.get(i,j);
            
            // Intensities descriptor
            cv::Mat tIntensitiesHist;
            describeThermalIntesities(cell, cellMask, tIntensitiesHist);
            
            // Gradient orientation descriptor
            cv::Mat tGradOrientsHist;
            describeThermalGradOrients(cell, cellMask, tGradOrientsHist);
            
            // Join both descriptors in a row
            cv::Mat tHist;
            hconcat(tIntensitiesHist, tGradOrientsHist, tHist);
            
            descriptors.vconcat(tHist, i, j); // row in a matrix of descriptors
            
            GridMat t (gtag, grid.crows(), grid.ccols());
            tags.vconcat(t);
        }
    }
}


void ThermalFeatureExtractor::describe(vector<GridMat> grids, vector<GridMat> masks,
                                       GridMat & subDescriptors, GridMat & objDescriptors, GridMat & unkDescriptors)
{
    //    namedWindow("god");
    
    for (int k = 0; k < grids.size(); k++)
    {
        GridMat & grid = grids[k];
        GridMat & mask = masks[k];
        
        for (int i = 0; i < grid.crows(); i++) for (int j = 0; j < grid.ccols(); j++)
        {
            cv::Mat & cell = grid.get(i,j);
            cv::Mat & cellMask = mask.get(i,j);
            
            // Intensities descriptor
            cv::Mat tIntensitiesHist;
            describeThermalIntesities(cell, cellMask, tIntensitiesHist);
            
            // Gradient orientation descriptor
            cv::Mat tGradOrientsHist;
            describeThermalGradOrients(cell, cellMask, tGradOrientsHist);
            
            // Join both descriptors in a row
            cv::Mat tHist;
            hconcat(tIntensitiesHist, tGradOrientsHist, tHist);
            
            if (grid.type() == GridMat::SUBJECT) subDescriptors.vconcat(tHist, i, j); // row in a matrix of descriptors
            else if (grid.type() == GridMat::OBJECT) objDescriptors.vconcat(tHist, i, j);
            else if (grid.type() == GridMat::UNKNOWN) unkDescriptors.vconcat(tHist, i, j);
        }
    }
}


void ThermalFeatureExtractor::describeThermalIntesities(const cv::Mat cell, const cv::Mat cellMask, cv::Mat & tIntensitiesHist)
{
    int ibins = m_ThermalParam.ibins;
    
    // Create an histogram for the cell region of blurred intensity values
    int histSize[] = { (int) ibins };
    int channels[] = { 0 }; // 1 channel, number 0
    float tranges[] = { 0, 256 }; // thermal intensity values range: [0, 256)
    const float* ranges[] = { tranges };
    
    cv::Mat tmpHist;
    calcHist(&cell, 1, channels, cellMask, tmpHist, 1, histSize, ranges, true, false);
    transpose(tmpHist, tmpHist);
    
    hypercubeNorm(tmpHist, tIntensitiesHist);
    tmpHist.release();
}


void ThermalFeatureExtractor::describeThermalGradOrients(const cv::Mat cell, const cv::Mat cellMask, cv::Mat & tGradOrientsHist)
{
    cv::Mat cellSeg = cv::Mat::zeros(cell.rows, cell.cols, cell.depth());
    cell.copyTo(cellSeg, cellMask);
    
    // First derivatives
    cv::Mat cellDervX, cellDervY;
    cv::Sobel(cellSeg, cellDervX, CV_32F, 1, 0);
    cv::Sobel(cellSeg, cellDervY, CV_32F, 0, 1);
    
    cv::Mat cellGradOrients;
    cv::phase(cellDervX, cellDervY, cellGradOrients, true);
    
    int oribins = m_ThermalParam.oribins;
    
    cv::Mat tmpHist = cv::Mat::zeros(1, oribins, cv::DataType<float>::type);
    
    for (int i = 0; i < cellSeg.rows; i++) for (int j = 0; j < cellSeg.cols; j++)
    {
        float g_x = cellDervX.at<float>(i,j);//unsigned short>(i,j);
        float g_y = cellDervY.at<float>(i,j);
        
        float orientation = cellGradOrients.at<float>(i,j);
        float bin = static_cast<int>((orientation/360.0) * oribins) % oribins;
        tmpHist.at<float>(0, bin) += sqrtf(g_x * g_x + g_y * g_y);
    }
    
    cellSeg.release();
    cellDervX.release();
    cellDervY.release();
    cellGradOrients.release();
    
    hypercubeNorm(tmpHist, tGradOrientsHist);
    tmpHist.release();
}