/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                        Intel License Agreement
//                For Open Source Computer Vision Library
//
// Copyright( C) 2000, Intel Corporation, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other cv::Materials provided with the distribution.
//
//   * The name of Intel Corporation may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
//(including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort(including negligence or otherwise) arising in any way out of
// the use of this software, even ifadvised of the possibility of such damage.
//
//M*/

#include "em30.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/ml/ml.hpp>

const double minEigenValue = DBL_EPSILON;


///////////////////////////////////////////////////////////////////////////////////////////////////////

EM30::EM30(int _nclusters, int _covMatType, const cv::TermCriteria& _termCrit)
{
    nclusters = _nclusters;
    covMatType = _covMatType;
    maxIters = (_termCrit.type & cv::TermCriteria::MAX_ITER) ? _termCrit.maxCount : DEFAULT_MAX_ITERS;
    epsilon = (_termCrit.type & cv::TermCriteria::EPS) ? _termCrit.epsilon : 0;
}

EM30::~EM30()
{
	//clear();
}

void EM30::clear()
{
    trainSamples.release();
    trainProbs.release();
    trainLogLikelihoods.release();
    trainLabels.release();

    weights.release();
    means.release();
    covs.clear();

    covsEigenValues.clear();
    invCovsEigenValues.clear();
    covsRotateMats.clear();

    logWeightDivDet.release();
}

    
bool EM30::train( cv::InputArray samples,
                cv::OutputArray logLikelihoods,
                cv::OutputArray labels,
                cv::OutputArray probs)
{
    cv::Mat samplesMat = samples.getMat();
    setTrainData(START_AUTO_STEP, samplesMat, 0, 0, 0, 0);
    return doTrain(START_AUTO_STEP, logLikelihoods, labels, probs);
}

bool EM30::trainE( cv::InputArray samples,
                 cv::InputArray _means0,
                 cv::InputArray _covs0,
                 cv::InputArray _weights0,
                 cv::OutputArray logLikelihoods,
                 cv::OutputArray labels,
                 cv::OutputArray probs)
{
    cv::Mat samplesMat = samples.getMat();
    std::vector<cv::Mat> covs0;
    _covs0.getMatVector(covs0);
    
    cv::Mat means0 = _means0.getMat(), weights0 = _weights0.getMat();

    setTrainData(START_E_STEP, samplesMat, 0, !_means0.empty() ? &means0 : 0,
                 !_covs0.empty() ? &covs0 : 0, _weights0.empty() ? &weights0 : 0);
    return doTrain(START_E_STEP, logLikelihoods, labels, probs);
}

bool EM30::trainM( cv::InputArray samples,
                 cv::InputArray _probs0,
                 cv::OutputArray logLikelihoods,
                 cv::OutputArray labels,
                 cv::OutputArray probs)
{
    cv::Mat samplesMat = samples.getMat();
    cv::Mat probs0 = _probs0.getMat();
    
    setTrainData(START_M_STEP, samplesMat, !_probs0.empty() ? &probs0 : 0, 0, 0, 0);
    return doTrain(START_M_STEP, logLikelihoods, labels, probs);
}

    
cv::Vec2d EM30::predict( cv::InputArray _sample,  cv::OutputArray _probs,  cv::OutputArray _loglikelihoods) const
{
    cv::Mat sample = _sample.getMat();
    CV_Assert(isTrained());

    CV_Assert(!sample.empty());
    if(sample.type() != CV_64FC1)
    {
        cv::Mat tmp;
        sample.convertTo(tmp, CV_64FC1);
        sample = tmp;
    }
    sample.reshape(1, 1);

    cv::Mat probs;
    if( _probs.needed() )
    {
        _probs.create(1, nclusters, CV_64FC1);
        probs = _probs.getMat();
    }
    cv::Mat loglikelihoods;
    if( _loglikelihoods.needed() )
    {
        _loglikelihoods.create(1, nclusters, CV_64FC1);
        loglikelihoods = _loglikelihoods.getMat();
    }

    return computeProbabilities(sample, !probs.empty() ? &probs : 0, !loglikelihoods.empty() ? &loglikelihoods : 0);
}

bool EM30::isTrained() const
{
    return !means.empty();
}


static
void checkTrainData(int startStep, const cv::Mat& samples,
                    int nclusters, int covMatType, const cv::Mat* probs, const cv::Mat* means,
                    const std::vector<cv::Mat>* covs, const cv::Mat* weights)
{
    // Check samples.
    CV_Assert(!samples.empty());
    CV_Assert(samples.channels() == 1);

    int nsamples = samples.rows;
    int dim = samples.cols;

    // Check training params.
    CV_Assert(nclusters > 0);
    CV_Assert(nclusters <= nsamples);
    CV_Assert(startStep == EM30::START_AUTO_STEP ||
              startStep == EM30::START_E_STEP ||
              startStep == EM30::START_M_STEP);
    CV_Assert(covMatType == EM30::COV_MAT_GENERIC ||
              covMatType == EM30::COV_MAT_DIAGONAL ||
              covMatType == EM30::COV_MAT_SPHERICAL);

    CV_Assert(!probs ||
        (!probs->empty() &&
         probs->rows == nsamples && probs->cols == nclusters &&
         (probs->type() == CV_32FC1 || probs->type() == CV_64FC1)));

    CV_Assert(!weights ||
        (!weights->empty() &&
         (weights->cols == 1 || weights->rows == 1) && static_cast<int>(weights->total()) == nclusters &&
         (weights->type() == CV_32FC1 || weights->type() == CV_64FC1)));

    CV_Assert(!means ||
        (!means->empty() &&
         means->rows == nclusters && means->cols == dim &&
         means->channels() == 1));

    CV_Assert(!covs ||
        (!covs->empty() &&
         static_cast<int>(covs->size()) == nclusters));
    if(covs)
    {
        const cv::Size covSize(dim, dim);
        for(size_t i = 0; i < covs->size(); i++)
        {
            const cv::Mat& m = (*covs)[i];
            CV_Assert(!m.empty() && m.size() == covSize && (m.channels() == 1));
        }
    }

    if(startStep == EM30::START_E_STEP)
    {
        CV_Assert(means);
    }
    else if(startStep == EM30::START_M_STEP)
    {
        CV_Assert(probs);
    }
}

static
void preprocessSampleData(const cv::Mat& src, cv::Mat& dst, int dstType, bool isAlwaysClone)
{
    if(src.type() == dstType && !isAlwaysClone)
        dst = src;
    else
        src.convertTo(dst, dstType);
}

static
void preprocessProbability(cv::Mat& probs)
{
    max(probs, 0., probs);

    const double uniformProbability = (double)(1./probs.cols);
    for(int y = 0; y < probs.rows; y++)
    {
        cv::Mat sampleProbs = probs.row(y);

        double maxVal = 0;
        minMaxLoc(sampleProbs, 0, &maxVal);
        if(maxVal < FLT_EPSILON)
            sampleProbs.setTo(uniformProbability);
        else
            normalize(sampleProbs, sampleProbs, 1, 0, cv::NORM_L1);
    }
}

void EM30::setTrainData(int startStep, const cv::Mat& samples,
                      const cv::Mat* probs0,
                      const cv::Mat* means0,
                      const std::vector<cv::Mat>* covs0,
                      const cv::Mat* weights0)
{
    clear();

    checkTrainData(startStep, samples, nclusters, covMatType, probs0, means0, covs0, weights0);

    bool isKMeansInit = (startStep == EM30::START_AUTO_STEP) || (startStep == EM30::START_E_STEP && (covs0 == 0 || weights0 == 0));
    // Set checked data
    preprocessSampleData(samples, trainSamples, isKMeansInit ? CV_32FC1 : CV_64FC1, false);

    // set probs
    if(probs0 && startStep == EM30::START_M_STEP)
    {
        preprocessSampleData(*probs0, trainProbs, CV_64FC1, true);
        preprocessProbability(trainProbs);
    }

    // set weights
    if(weights0 && (startStep == EM30::START_E_STEP && covs0))
    {
        weights0->convertTo(weights, CV_64FC1);
        weights.reshape(1,1);
        preprocessProbability(weights);
    }

    // set means
    if(means0 && (startStep == EM30::START_E_STEP/* || startStep == EM30::START_AUTO_STEP*/))
        means0->convertTo(means, isKMeansInit ? CV_32FC1 : CV_64FC1);

    // set covs
    if(covs0 && (startStep == EM30::START_E_STEP && weights0))
    {
        covs.resize(nclusters);
        for(size_t i = 0; i < covs0->size(); i++)
            (*covs0)[i].convertTo(covs[i], CV_64FC1);
    }
}

void EM30::decomposeCovs()
{
    CV_Assert(!covs.empty());
    covsEigenValues.resize(nclusters);
    if(covMatType == EM30::COV_MAT_GENERIC)
        covsRotateMats.resize(nclusters);
    invCovsEigenValues.resize(nclusters);
    for(int clusterIndex = 0; clusterIndex < nclusters; clusterIndex++)
    {
        CV_Assert(!covs[clusterIndex].empty());

        cv::SVD svd(covs[clusterIndex], cv::SVD::MODIFY_A + cv::SVD::FULL_UV);

        if(covMatType == EM30::COV_MAT_SPHERICAL)
        {
            double maxSingularVal = svd.w.at<double>(0);
            covsEigenValues[clusterIndex] = cv::Mat(1, 1, CV_64FC1, cv::Scalar(maxSingularVal));
        }
        else if(covMatType == EM30::COV_MAT_DIAGONAL)
        {
            covsEigenValues[clusterIndex] = svd.w;
        }
        else //EM30::COV_MAT_GENERIC
        {
            covsEigenValues[clusterIndex] = svd.w;
            covsRotateMats[clusterIndex] = svd.u;
        }
        max(covsEigenValues[clusterIndex], minEigenValue, covsEigenValues[clusterIndex]);
        invCovsEigenValues[clusterIndex] = 1./covsEigenValues[clusterIndex];
    }
}

void EM30::clusterTrainSamples()
{
    int nsamples = trainSamples.rows;

    // Cluster samples, compute/update means

    // Convert samples and means to 32F, because kmeans requires this type.
    cv::Mat trainSamplesFlt, meansFlt;
    if(trainSamples.type() != CV_32FC1)
        trainSamples.convertTo(trainSamplesFlt, CV_32FC1);
    else
        trainSamplesFlt = trainSamples;
    if(!means.empty())
    {
        if(means.type() != CV_32FC1)
            means.convertTo(meansFlt, CV_32FC1);
        else
            meansFlt = means;
    }

    cv::Mat labels;
    cv::kmeans(trainSamplesFlt, nclusters, labels, cv::TermCriteria(cv::TermCriteria::COUNT, means.empty() ? 10 : 1, 0.5), 10, cv::KMEANS_PP_CENTERS, meansFlt);

    // Convert samples and means back to 64F.
    CV_Assert(meansFlt.type() == CV_32FC1);
    if(trainSamples.type() != CV_64FC1)
    {
        cv::Mat trainSamplesBuffer;
        trainSamplesFlt.convertTo(trainSamplesBuffer, CV_64FC1);
        trainSamples = trainSamplesBuffer;
    }
    meansFlt.convertTo(means, CV_64FC1);

    // Compute weights and covs
    weights = cv::Mat(1, nclusters, CV_64FC1, cv::Scalar(0));
    covs.resize(nclusters);
    for(int clusterIndex = 0; clusterIndex < nclusters; clusterIndex++)
    {
        cv::Mat clusterSamples;
        for(int sampleIndex = 0; sampleIndex < nsamples; sampleIndex++)
        {
            if(labels.at<int>(sampleIndex) == clusterIndex)
            {
                const cv::Mat sample = trainSamples.row(sampleIndex);
                clusterSamples.push_back(sample);
            }
        }
        CV_Assert(!clusterSamples.empty());

        calcCovarMatrix(clusterSamples, covs[clusterIndex], means.row(clusterIndex),
            CV_COVAR_NORMAL + CV_COVAR_ROWS + CV_COVAR_USE_AVG + CV_COVAR_SCALE, CV_64FC1);
        weights.at<double>(clusterIndex) = static_cast<double>(clusterSamples.rows)/static_cast<double>(nsamples);
    }

    decomposeCovs();
}

void EM30::computeLogWeightDivDet()
{
    CV_Assert(!covsEigenValues.empty());

    cv::Mat logWeights;
    cv::max(weights, DBL_MIN, weights);
    log(weights, logWeights);

    logWeightDivDet.create(1, nclusters, CV_64FC1);
    // note: logWeightDivDet = log(weight_k) - 0.5 * log(|det(cov_k)|)

    for(int clusterIndex = 0; clusterIndex < nclusters; clusterIndex++)
    {
        double logDetCov = 0.;
        for(int di = 0; di < covsEigenValues[clusterIndex].cols; di++)
            logDetCov += std::log(covsEigenValues[clusterIndex].at<double>(covMatType != EM30::COV_MAT_SPHERICAL ? di : 0));

        logWeightDivDet.at<double>(clusterIndex) = logWeights.at<double>(clusterIndex) - 0.5 * logDetCov;
    }
}

bool EM30::doTrain(int startStep,  cv::OutputArray logLikelihoods,  cv::OutputArray labels,  cv::OutputArray probs)
{
    int dim = trainSamples.cols;
    // Precompute the empty initial train data in the cases of EM30::START_E_STEP and START_AUTO_STEP
    if(startStep != EM30::START_M_STEP)
    {
        if(covs.empty())
        {
            CV_Assert(weights.empty());
            clusterTrainSamples();
        }
    }

    if(!covs.empty() && covsEigenValues.empty() )
    {
        CV_Assert(invCovsEigenValues.empty());
        decomposeCovs();
    }

    if(startStep == EM30::START_M_STEP)
        mStep();

    double trainLogLikelihood, prevTrainLogLikelihood = 0.;
    for(int iter = 0; ; iter++)
    {
        eStep();
        trainLogLikelihood = sum(trainLogLikelihoods)[0];

        if(iter >= maxIters - 1)
            break;

        double trainLogLikelihoodDelta = trainLogLikelihood - prevTrainLogLikelihood;
        if( iter != 0 &&
            (trainLogLikelihoodDelta < -DBL_EPSILON ||
             trainLogLikelihoodDelta < epsilon * std::fabs(trainLogLikelihood)))
            break;

        mStep();

        prevTrainLogLikelihood = trainLogLikelihood;
    }

    if( trainLogLikelihood <= -DBL_MAX/10000. )
    {
        clear();
        return false;
    }

    // postprocess covs
    covs.resize(nclusters);
    for(int clusterIndex = 0; clusterIndex < nclusters; clusterIndex++)
    {
        if(covMatType == EM30::COV_MAT_SPHERICAL)
        {
            covs[clusterIndex].create(dim, dim, CV_64FC1);
            setIdentity(covs[clusterIndex], cv::Scalar(covsEigenValues[clusterIndex].at<double>(0)));
        }
        else if(covMatType == EM30::COV_MAT_DIAGONAL)
        {
            covs[clusterIndex] = cv::Mat::diag(covsEigenValues[clusterIndex]);
        }
    }
    
    if(labels.needed())
        trainLabels.copyTo(labels);
    if(probs.needed())
        trainProbs.copyTo(probs);
    if(logLikelihoods.needed())
        trainLogLikelihoods.copyTo(logLikelihoods);
    
    trainSamples.release();
    trainProbs.release();
    trainLabels.release();
	trainLogLikelihoods.release();

    return true;
}

cv::Vec2d EM30::computeProbabilities(const cv::Mat& sample, cv::Mat* probs, cv::Mat* logLikelihoods) const
{
    // L_ik = log(weight_k) - 0.5 * log(|det(cov_k)|) - 0.5 *(x_i - mean_k)' cov_k^(-1) (x_i - mean_k)]
    // q = arg(max_k(L_ik))
    // probs_ik = exp(L_ik - L_iq) / (1 + sum_j!=q (exp(L_ij - L_iq))
    // see Alex Smola's blog http://blog.smola.org/page/2 for
    // details on the log-sum-exp trick

    CV_Assert(!means.empty());
    CV_Assert(sample.type() == CV_64FC1);
    CV_Assert(sample.rows == 1);
    CV_Assert(sample.cols == means.cols);

    int dim = sample.cols;

    cv::Mat L (1, nclusters, CV_64FC1);

    int label = 0;
    for(int clusterIndex = 0; clusterIndex < nclusters; clusterIndex++)
    {
        const cv::Mat centeredSample = sample - means.row(clusterIndex);

        cv::Mat rotatedCenteredSample = covMatType != EM30::COV_MAT_GENERIC ?
                centeredSample : centeredSample * covsRotateMats[clusterIndex];

        double Lval = 0;
        for(int di = 0; di < dim; di++)
        {
            double w = invCovsEigenValues[clusterIndex].at<double>(covMatType != EM30::COV_MAT_SPHERICAL ? di : 0);
            double val = rotatedCenteredSample.at<double>(di);
            Lval += w * val * val;
        }
        CV_DbgAssert(!logWeightDivDet.empty());
        L.at<double>(clusterIndex) = logWeightDivDet.at<double>(clusterIndex) - 0.5 * Lval;

        if(L.at<double>(clusterIndex) > L.at<double>(label))
            label = clusterIndex;
    }

    double maxLVal = L.at<double>(label);
    cv::Mat expL_Lmax;
	L.copyTo(expL_Lmax); // exp(L_ij - L_iq)
    for(int i = 0; i < L.cols; i++)
        expL_Lmax.at<double>(i) = std::exp(L.at<double>(i) - maxLVal);

    double expDiffSum = sum(expL_Lmax)[0]; // sum_j(exp(L_ij - L_iq))

    if(probs)
    {
        probs->create(1, nclusters, CV_64FC1);
        double factor = 1./expDiffSum;
        expL_Lmax *= factor;
        expL_Lmax.copyTo(*probs);
    }

	if(logLikelihoods)
	{
		for(int clusterIndex = 0; clusterIndex < nclusters; clusterIndex++)
		{
			logLikelihoods->at<double>(clusterIndex) = std::log(expDiffSum)  + L.at<double>(clusterIndex) - 0.5 * dim * CV_LOG2PI;
			//std::cout << logWeightDivDet.at<double>(clusterIndex) << std::endl;
			//std::cout << L.at<double>(clusterIndex) << std::endl;
			//std::cout << expL_Lmax.at<double>(clusterIndex) << std::endl;
			//std::cout << logLikelihoods->at<double>(clusterIndex) << std::endl;
			//std::cout << std::endl;
		}
		//std::cout << std::endl;
	}

    cv::Vec2d res;
    res[0] = std::log(expDiffSum)  + maxLVal - 0.5 * dim * CV_LOG2PI;
    res[1] = label;

    return res;
}

void EM30::eStep()
{
    // Compute probs_ik from means_k, covs_k and weights_k.
    trainProbs.create(trainSamples.rows, nclusters, CV_64FC1);
    trainLabels.create(trainSamples.rows, 1, CV_32SC1);
    trainLogLikelihoods.create(trainSamples.rows, 1, CV_64FC1);

    computeLogWeightDivDet();

    CV_DbgAssert(trainSamples.type() == CV_64FC1);
    CV_DbgAssert(means.type() == CV_64FC1);

    for(int sampleIndex = 0; sampleIndex < trainSamples.rows; sampleIndex++)
    {
        cv::Mat sampleProbs = trainProbs.row(sampleIndex);
        cv::Mat sampleLogLikelihoods (1, nclusters, CV_64FC1);
        cv::Vec2d res = computeProbabilities(trainSamples.row(sampleIndex), &sampleProbs, &sampleLogLikelihoods);
        trainLogLikelihoods.at<double>(sampleIndex) = res[0];
        trainLabels.at<int>(sampleIndex) = static_cast<int>(res[1]);
    }
}

void EM30::mStep()
{
    // Update means_k, covs_k and weights_k from probs_ik
    int dim = trainSamples.cols;

    // Update weights
    // not normalized first
    reduce(trainProbs, weights, 0, CV_REDUCE_SUM);

    // Update means
    means.create(nclusters, dim, CV_64FC1);
    means = cv::Scalar(0);

    const double minPosWeight = trainSamples.rows * DBL_EPSILON;
    double minWeight = DBL_MAX;
    int minWeightClusterIndex = -1;
    for(int clusterIndex = 0; clusterIndex < nclusters; clusterIndex++)
    {
        if(weights.at<double>(clusterIndex) <= minPosWeight)
            continue;

        if(weights.at<double>(clusterIndex) < minWeight)
        {
            minWeight = weights.at<double>(clusterIndex);
            minWeightClusterIndex = clusterIndex;
        }

        cv::Mat clusterMean = means.row(clusterIndex);
        for(int sampleIndex = 0; sampleIndex < trainSamples.rows; sampleIndex++)
            clusterMean += trainProbs.at<double>(sampleIndex, clusterIndex) * trainSamples.row(sampleIndex);
        clusterMean /= weights.at<double>(clusterIndex);
    }

    // Update covsEigenValues and invCovsEigenValues
    covs.resize(nclusters);
    covsEigenValues.resize(nclusters);
    if(covMatType == EM30::COV_MAT_GENERIC)
        covsRotateMats.resize(nclusters);
    invCovsEigenValues.resize(nclusters);
    for(int clusterIndex = 0; clusterIndex < nclusters; clusterIndex++)
    {
        if(weights.at<double>(clusterIndex) <= minPosWeight)
            continue;

        if(covMatType != EM30::COV_MAT_SPHERICAL)
            covsEigenValues[clusterIndex].create(1, dim, CV_64FC1);
        else
            covsEigenValues[clusterIndex].create(1, 1, CV_64FC1);

        if(covMatType == EM30::COV_MAT_GENERIC)
            covs[clusterIndex].create(dim, dim, CV_64FC1);

        cv::Mat clusterCov = covMatType != EM30::COV_MAT_GENERIC ?
            covsEigenValues[clusterIndex] : covs[clusterIndex];

        clusterCov = cv::Scalar(0);

        cv::Mat centeredSample;
        for(int sampleIndex = 0; sampleIndex < trainSamples.rows; sampleIndex++)
        {
            centeredSample = trainSamples.row(sampleIndex) - means.row(clusterIndex);

            if(covMatType == EM30::COV_MAT_GENERIC)
                clusterCov += trainProbs.at<double>(sampleIndex, clusterIndex) * centeredSample.t() * centeredSample;
            else
            {
                double p = trainProbs.at<double>(sampleIndex, clusterIndex);
                for(int di = 0; di < dim; di++ )
                {
                    double val = centeredSample.at<double>(di);
                    clusterCov.at<double>(covMatType != EM30::COV_MAT_SPHERICAL ? di : 0) += p*val*val;
                }
            }
        }

        if(covMatType == EM30::COV_MAT_SPHERICAL)
            clusterCov /= dim;

        clusterCov /= weights.at<double>(clusterIndex);

        // Update covsRotateMats for EM30::COV_MAT_GENERIC only
        if(covMatType == EM30::COV_MAT_GENERIC)
        {
            cv::SVD svd(covs[clusterIndex], cv::SVD::MODIFY_A + cv::SVD::FULL_UV);
            covsEigenValues[clusterIndex] = svd.w;
            covsRotateMats[clusterIndex] = svd.u;
        }

        max(covsEigenValues[clusterIndex], minEigenValue, covsEigenValues[clusterIndex]);

        // update invCovsEigenValues
        invCovsEigenValues[clusterIndex] = 1./covsEigenValues[clusterIndex];
    }

    for(int clusterIndex = 0; clusterIndex < nclusters; clusterIndex++)
    {
        if(weights.at<double>(clusterIndex) <= minPosWeight)
        {
            cv::Mat clusterMean = means.row(clusterIndex);
            means.row(minWeightClusterIndex).copyTo(clusterMean);
            covs[minWeightClusterIndex].copyTo(covs[clusterIndex]);
            covsEigenValues[minWeightClusterIndex].copyTo(covsEigenValues[clusterIndex]);
            if(covMatType == EM30::COV_MAT_GENERIC)
                covsRotateMats[minWeightClusterIndex].copyTo(covsRotateMats[clusterIndex]);
            invCovsEigenValues[minWeightClusterIndex].copyTo(invCovsEigenValues[clusterIndex]);
        }
    }

    // Normalize weights
    weights /= trainSamples.rows;
}

void EM30::read(const cv::FileNode& fn)
{
    Algorithm::read(fn);

    decomposeCovs();
    computeLogWeightDivDet();
}

/* End of file. */