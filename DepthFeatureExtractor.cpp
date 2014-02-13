//
//  DepthFeatureExtractor.cpp
//  Segmenthreetion
//
//  Created by Albert Clap�s on 24/05/13.
//  Copyright (c) 2013 Albert Clap�s. All rights reserved.
//

#include "DepthFeatureExtractor.h"
#include "DepthParametrization.h"

#include <opencv2/opencv.hpp>

#include <pcl/point_types.h>
#include <pcl/features/normal_3d.h>
#include <pcl/point_cloud.h>
#include <pcl/visualization/cloud_viewer.h>

#include <pcl/filters/statistical_outlier_removal.h>

#include <boost/thread.hpp>
#include <boost/timer.hpp>

#define __PI 3.14159265

DepthFeatureExtractor::DepthFeatureExtractor(int hp, int wp) : m_hp(hp), m_wp(wp)
{ }

DepthFeatureExtractor::DepthFeatureExtractor(int hp, int wp, DepthParametrization dParam)
	: m_hp(hp), m_wp(wp), m_DepthParam(dParam)
{ }

void DepthFeatureExtractor::setData(vector<GridMat> grids, vector<GridMat> masks)
{
    m_DepthGrids = grids;
    m_DepthMasks = masks;
}

void DepthFeatureExtractor::setParam(DepthParametrization depthParam)
{
    m_DepthParam = depthParam;
}

//void DepthFeatureExtractor::projectiveToRealWorld(float px, float py, float pz, float & rwx, float & rwy, float & rwz)
//{
//    
//}

void DepthFeatureExtractor::describeNormalsOrients(const cv::Mat cell, const cv::Mat mask, cv::Mat & dNormalsOrientsHist)
{    
    pcl::PointCloud<pcl::PointXYZ>::Ptr pCloud ( new pcl::PointCloud<pcl::PointXYZ>() );
    pCloud->height = cell.rows;
    pCloud->width = cell.cols;
    pCloud->resize(pCloud->height * pCloud->width);
    pCloud->is_dense = true;
    
    float invfocal = 3.501e-3f; // Kinect inverse focal length. If depth map resolution of: 320 x 240

    int n = 0;
    for (unsigned int y = 0; y < pCloud->height; y++) for (unsigned int x = 0; x < pCloud->width; x++)
    {
        unsigned short uz = cell.at<unsigned short>(y,x);
        unsigned short z = uz >> 3;
        if ( z > 0 && z < 8191 && mask.at<unsigned char>(y,x) > 0 ) // not a depth error
        {
            float rwx, rwy, rwz;
            
            rwx = (x - 320.0) * invfocal * z;
            rwy = (y - 240.0) * invfocal * z;
            rwz = z;
            
            pcl::PointXYZ p(rwx/1000.f, rwy/1000.f, rwz/1000.f);
            pCloud->at(x,y) = p;
            
//            cout << pCloud->at(x,y).x << ", "
//            << pCloud->at(x,y).y << ", "
//            << pCloud->at(x,y).z << endl;
            
            n++;
        }
    }
    
//    cout << "bargain" << endl;
    
    if (n == 0)
        return;
    
    // Create the filtering object
//    pcl::PointCloud<pcl::PointXYZ>::Ptr pCloudFiltered ( new pcl::PointCloud<pcl::PointXYZ>() );
//    
//    pcl::StatisticalOutlierRemoval<pcl::PointXYZ> sor;
//    sor.setInputCloud (pCloud);
//    sor.setMeanK (50);
//    sor.setStddevMulThresh (1.0);
//    sor.filter (*pCloudFiltered);
//    
//    cout << pCloudFiltered->points.size() << endl;
    
    // Code performing normal estimation and calculus of the grid descriptor
    
//    if (pCloud->height > 70 && pCloud->width > 70)
//    {
//        pcl::visualization::PCLVisualizer viz("viz");
//        viz.addPointCloud<pcl::PointXYZ>(pCloudFiltered, "sample cloud");
//        viz.initCameraParameters();
//        viz.addCoordinateSystem(5.0, 0, 0, 0);
//        
//        while (!viz.wasStopped ())
//        {
//            viz.spinOnce (100);
//            boost::this_thread::sleep (boost::posix_time::microseconds (100000));
//        }
//    }
    
    // Compute the normals using pcl
    
    pcl::PointCloud<pcl::Normal>::Ptr pNormals (new pcl::PointCloud<pcl::Normal>);
    
    pcl::NormalEstimation<pcl::PointXYZ, pcl::Normal> ne;
    ne.setInputCloud(pCloud);
    
    pcl::search::KdTree<pcl::PointXYZ>::Ptr tree (new pcl::search::KdTree<pcl::PointXYZ> ());
    ne.setSearchMethod (tree);
    
    ne.setRadiusSearch (m_DepthParam.normalsRadius); // neighbors in a sphere of radius X meters
    
    ne.compute (*pNormals);
    
//    cout << pNormals->points.size() << endl;
//    cout << pNormals->height << endl;
//    cout << pNormals->width << endl;
    
    // Put it in a histogram
    
    cv::Mat thetas = cv::Mat::zeros(pNormals->height, pNormals->width, cv::DataType<float>::type);
    cv::Mat phis   = cv::Mat::zeros(pNormals->height, pNormals->width, cv::DataType<float>::type);
    
//    cout << endl;
    for (unsigned int y = 0; y < pNormals->height; y++) for (unsigned int x = 0; x < pNormals->width; x++)
    {
//        cout << pCloud->at(x,y).x << " " << pNormals->at(x,y).normal_x << ", " << pCloud->at(x,y).y << " " << pNormals->at(x,y).normal_y << ", " << pCloud->at(x,y).z << " " << pNormals->at(x,y).normal_z << endl;
   
        float nx = pNormals->at(x,y).normal_x;
        float ny = pNormals->at(x,y).normal_y;
        float nz = pNormals->at(x,y).normal_z;
        float r = sqrt(powf(nx,2) + powf(ny,2) + powf(nz,2));
//        cout << "pcl" << endl;
        
        thetas.at<float>(y,x) = acos(nz / r) * 180.0 / __PI;
        phis.at<float>(y,x) = atan(ny / nx) * 180.0 / __PI;
//        cout << "opencv" << endl;
    }
    
//    cout << "hist computation" << endl;
    
    cv::Mat thetasHist = cv::Mat::zeros(1, m_DepthParam.thetaBins, cv::DataType<float>::type);
    cv::Mat phisHist   = cv::Mat::zeros(1, m_DepthParam.phiBins, cv::DataType<float>::type);
    
    // Create an histogram for the cell region of blurred intensity values
    int thetasHistSize[] = { (int) m_DepthParam.thetaBins };
    int channels[] = { 0 }; // 1 channel, number 0
    float tranges[] = { 0, 360 }; // thermal intensity values range: [0, 256)
    const float* ranges[] = { tranges };
    
    cv::Mat tmpThetasHist;
    cv::calcHist(&thetas, 1, channels, mask, tmpThetasHist, 1, thetasHistSize, ranges, true, false);
    cv::transpose(tmpThetasHist, tmpThetasHist);
    hypercubeNorm(tmpThetasHist, thetasHist);
    tmpThetasHist.release();
    
    int phisHistSize[] = { (int) m_DepthParam.phiBins };
    
    cv::Mat tmpPhiHist;
    cv::calcHist(&phis, 1, channels, mask, tmpPhiHist, 1, phisHistSize, ranges, true, false);
    cv::transpose(tmpPhiHist, tmpPhiHist);
    hypercubeNorm(tmpPhiHist, phisHist);
    tmpPhiHist.release();
    
    // Join both descriptors in a row
    hconcat(thetasHist, phisHist, dNormalsOrientsHist);
    thetasHist.release();
    phisHist.release();
    
    return;
}

void DepthFeatureExtractor::describe(GridMat & descriptors)
{
	descriptors.release();
    descriptors   = GridMat(m_hp, m_wp);

    //    namedWindow("god");
    
    for (int k = 0; k < m_DepthGrids.size(); k++)
    {
        GridMat & grid = m_DepthGrids[k];
        GridMat & mask = m_DepthMasks[k];
        
        cout << k << "/" << m_DepthGrids.size() << endl;
        
        for (int i = 0; i < grid.crows(); i++) for (int j = 0; j < grid.ccols(); j++)
        {
//            cout << i << "," << j << endl;
            cv::Mat & cell = grid.get(i,j);
            cv::Mat & cellMask = mask.get(i,j);
            
//            imshow("god", cell);
//            waitKey(0);
            
            // Normals orientation descriptor
            cv::Mat dNormalsOrientsHist(1, (m_DepthParam.thetaBins + m_DepthParam.phiBins), CV_32F);
            dNormalsOrientsHist.setTo(NAN);
            
            describeNormalsOrients(cell, cellMask, dNormalsOrientsHist);
            
//            cout << dNormalsOrientsHist << endl; // DEBUG
            
            // Consider the descriptor only if does not contain nans
            if (checkRange(dNormalsOrientsHist) && !dNormalsOrientsHist.empty())
            {
                cout << dNormalsOrientsHist << endl;
            }
//            {
//                cout << dNormalsOrientsHist << endl; // DEBUG
                descriptors.vconcat(dNormalsOrientsHist, i, j); // row in a matrix of descriptors
//            }
        }
    }
}


void DepthFeatureExtractor::describe(GridMat & subjectDescriptors, GridMat & objectDescriptors)
{
	subjectDescriptors.release();
    objectDescriptors.release();
    
    subjectDescriptors   = GridMat(m_hp, m_wp, GridMat::SUBJECT);
    objectDescriptors   = GridMat(m_hp, m_wp, GridMat::OBJECT);
    
    //    namedWindow("god");
    
    for (int k = 0; k < m_DepthGrids.size(); k++)
    {
        GridMat & grid = m_DepthGrids[k];
        GridMat & mask = m_DepthMasks[k];
        
        cout << k << "/" << m_DepthGrids.size() << endl;
        
        for (int i = 0; i < grid.crows(); i++) for (int j = 0; j < grid.ccols(); j++)
        {
            //            cout << i << "," << j << endl;
            cv::Mat & cell = grid.get(i,j);
            cv::Mat & cellMask = mask.get(i,j);
            
            //            imshow("god", cell);
            //            waitKey(0);
            
            // Normals orientation descriptor
            cv::Mat dNormalsOrientsHist(1, (m_DepthParam.thetaBins + m_DepthParam.phiBins), CV_32F);
            dNormalsOrientsHist.setTo(NAN);
            //cout << dNormalsOrientsHist << endl;
            describeNormalsOrients(cell, cellMask, dNormalsOrientsHist);
            
            cout << dNormalsOrientsHist << endl; // DEBUG
            
            // Consider the descriptor only if does not contain nans
            //            if (checkRange(dNormalsOrientsHist) && !dNormalsOrientsHist.empty())
            //            {
            //                cout << dNormalsOrientsHist << endl; // DEBUG
            if (grid.type() == GridMat::SUBJECT)
                subjectDescriptors.vconcat(dNormalsOrientsHist, i, j); // row in a matrix of descriptors
            else
                objectDescriptors.vconcat(dNormalsOrientsHist, i, j);
            //            }
        }
    }
}


/*
 * Hypercube normalization
 */
void DepthFeatureExtractor::hypercubeNorm(cv::Mat & src, cv::Mat & dst)
{
    src.copyTo(dst);
    double z = sum(src).val[0]; // partition function :D
    dst = dst / z;
}