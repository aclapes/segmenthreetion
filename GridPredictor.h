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
    GridPredictorBase();
    
    void setData(GridMat data);
    void setParameters(GridMat parameters);
    
    PredictorT& at(unsigned int i, unsigned int j);

protected:
    GridMat m_data;
    GridMat m_categories;
    
    unsigned int m_hp, m_wp;
    
    vector<PredictorT> m_predictors;
};


template<typename PredictorT>
class GridPredictor : public GridPredictorBase<PredictorT>
{
    GridPredictor();
    
    void setData(GridMat data);
    void setParameters(GridMat parameters);
    
    PredictorT& at(unsigned int i, unsigned int j);
};


template<>
class GridPredictor<cv::EM> : public GridPredictorBase<cv::EM>
{
public:
    GridPredictor();
    
    void setData(GridMat data);
    void setParameters(GridMat parameters);
    void setNumOfMixtures(cv::Mat nmixtures);
    void setLoglikelihoodThreshold(cv::Mat loglikes);
    
    cv::EM& at(unsigned int i, unsigned int j);
    
    void train();
    void predict(GridMat data, GridMat& predictions, GridMat& loglikelihoods);
    
private:
    cv::Mat m_logthreshold;
};



#endif /* defined(__segmenthreetion__GridPredictor__) */
