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
    
    void setData(ModalityGridData& data);
    
    void setModelSelection(bool flag);
    void setModelSelectionParameters(int k, bool best);
    
    void setValidationParameters(int k, int seed);
    
protected:
    void computeGridPredictionsConsensus(GridMat individualPredictions, GridMat distsToMargin, GridMat& consensusPredictions);
    
    // Attributes
    
    ModalityGridData m_data;
    int m_hp, m_wp;
    
    int m_seed;
    
    int m_testK; // number of folds in outer cross-validation to obtain the test results
    
    bool m_bModelSelection; // wheter to perform the selection or re-use an old one (in disk)
                            // files would be named: goodnesses_1.yml, ..., goodnesses_N.yml
                            // where N is equal to m_testK
    int m_modelSelecK; // number of folds in inner cross-validation to perform model selection
    bool m_selectBest; // in model selection
};


template<typename Prediction>
class ModalityPrediction : public ModalityPredictionBase<Prediction>
{
    ModalityPrediction();// : ModalityPredictionBase<Prediction>() {}
};


template<>
class ModalityPrediction<cv::EM> : public ModalityPredictionBase<cv::EM>
{
public:
    ModalityPrediction();
    
    void setNumOfMixtures(int m);
    void setNumOfMixtures(vector<int> m);
    
    void setLoglikelihoodThresholds(float t);
    void setLoglikelihoodThresholds(vector<float> t);
    
    template<typename T>
    void modelSelection(cv::Mat descriptors, cv::Mat tags,
                        vector<vector<T> > params,
                        cv::Mat& goodness);
    
    void compute(GridMat& predictions, GridMat& loglikelihoods, GridMat& distsToMargin); // this
    
    void computeLoglikelihoodsDistribution(int nbins, double min, double max, cv::Mat& sbjDistribution, cv::Mat& objDistribution);
    
private:
    
    // Attributes
    
    vector<int> m_nmixtures;
    vector<float> m_logthresholds;
};

template<>
class ModalityPrediction<cv::Mat> : public ModalityPredictionBase<cv::Mat>
{
public:
    ModalityPrediction();
    
    void setPositiveClassificationRatios(float m);
    void setPositiveClassificationRatios(vector<float> m);
    
    void setScoreThresholds(float t);
    void setScoreThresholds(vector<float> t);
    
    template<typename T>
    void modelSelection(cv::Mat gridsIndices, cv::Mat tags,
                        unsigned int i, unsigned int j,
                        vector<vector<T> > params,
                        cv::Mat& goodness);
    
    void compute(GridMat& predictions, GridMat& scores, GridMat& distsToMargin); // this
    
private:
    
    // Attributes
    
    vector<float> m_ratios;
    vector<float> m_scores;
};


#endif /* defined(__segmenthreetion__ModalityPrediction__) */
