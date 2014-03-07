//
//  ModalityPrediction.h
//  segmenthreetion
//
//  Created by Albert Clapés on 02/03/14.
//
//

#ifndef __segmenthreetion__ModalityPrediction__
#define __segmenthreetion__ModalityPrediction__

#include <iostream>
#include <vector>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/ml/ml.hpp>

#include "GridMat.h"
#include "ModalityGridData.hpp"

using namespace std;

template<typename Prediction>
class ModalityPredictionBase
{
public:
    ModalityPredictionBase();
    
    void setData(ModalityGridData data, GridMat descriptors);
    
    void setModelSelection(int k, bool best);
    void setModelValidation(int k, int seed);
    
    void accuracy(cv::Mat actuals, GridMat predictions, cv::Mat& accuracies);
    
protected:
    ModalityGridData m_data;
    GridMat m_descriptors;
    
    int m_modelSelecK; // number of folds in inner cross-validation to perform model selection
    bool m_selectBest; // in model selection
    
    int m_testK; // number of folds in outer cross-validation to obtain the test results
    int m_seed;
};


template<typename Prediction>
class ModalityPrediction : public ModalityPredictionBase<Prediction>
{
    ModalityPrediction();// : ModalityPredictionBase<Prediction>() {}
    
    void setData(ModalityGridData data, GridMat descriptors);
    
    void setModelSelection(int k = 3, bool best = 0);
    void setModelValidation(int k = 10, int seed = 74);
    
    void accuracy(cv::Mat actuals, GridMat predictions, cv::Mat& accuracies);
};


template<>
class ModalityPrediction<cv::EM> : public ModalityPredictionBase<cv::EM>
{
public:
    ModalityPrediction(); // : ModalityPredictionBase<cv::EM>() {}
    
    void setData(ModalityGridData data, GridMat descriptors);
    
    void setModelSelection(int k = 3, bool best = 0);
    void setModelValidation(int k = 10, int seed = 74);
    
    void setNumOfMixtures(int m);
    void setNumOfMixtures(vector<int> m);
    
    void setLoglikelihoodThresholds(int t);
    void setLoglikelihoodThresholds(vector<int> t);
    
    void modelSelection(ModalityGridData data, GridMat descriptors,
                        vector<int> nmixtures, vector<int> loglikelihoods,
                        GridMat& selection);
    void predict(GridMat& predictions, GridMat& loglikelihoods);
    
    void accuracy(cv::Mat actuals, GridMat predictions, cv::Mat& accuracies);
    
private:
    vector<int> m_nmixtures;
    vector<int> m_logthresholds;
};

#endif /* defined(__segmenthreetion__ModalityPrediction__) */
