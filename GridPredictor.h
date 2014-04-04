//
//  GridPredictor.h
//  segmenthreetion
//
//  Created by Albert Clapés on 02/03/14.
//
//

#ifndef __segmenthreetion__GridPredictor__
#define __segmenthreetion__GridPredictor__


#include <iostream>
#include <vector>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/ml/ml.hpp>

#include "GridMat.h"

using namespace std;

template<typename PredictorT>
class GridPredictorBase
{
public:
    GridPredictorBase(int hp, int wp);
    
    void setParameters(GridMat parameters);
    
    PredictorT* at(unsigned int i, unsigned int j);

protected:
    GridMat m_data;
    GridMat m_categories;
    
    unsigned int m_hp, m_wp;
    
    vector<PredictorT*> m_pPredictors;
};


template<typename PredictorT>
class GridPredictor : public GridPredictorBase<PredictorT>
{

};

template<>
class GridPredictor<cv::EM> : public GridPredictorBase<cv::EM>
{
public:
    GridPredictor(int hp, int wp);
    
    void setParameters(GridMat parameters);
    void setNumOfMixtures(cv::Mat nmixtures);
    void setLoglikelihoodThreshold(cv::Mat loglikes);
    
    void train(GridMat data);
    void predict(GridMat data, GridMat& loglikelihoods);
    void predict(GridMat data, GridMat& predictions, GridMat& loglikelihoods, GridMat& distsToMargin);
    
private:
    cv::Mat m_nmixtures;
    cv::Mat m_logthreshold;
};

#endif /* defined(__segmenthreetion__GridPredictor__) */
