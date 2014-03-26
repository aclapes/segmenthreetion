//
//  ModalityReader.h
//  segmenthreetion
//
//  Created by Albert Clapés on 01/03/14.
//
//

#ifndef __segmenthreetion__ModalityReader__
#define __segmenthreetion__ModalityReader__

#include <iostream>
#include <vector>
#include <string>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "ModalityData.hpp"
#include "ModalityGridData.hpp"

class ModalityReader
{
public:
    ModalityReader();
    ModalityReader(std::string dataPath);
    
    void setDataPath(std::string dataPath);
    void setMasksOffset(unsigned char offset);
    
    void read(std::string modality, ModalityData& md);
	void read(std::string modality, std::string parentDir, const char* filetype, int hp, int wp, ModalityGridData& mgd);
    
private:
    std::string m_DataPath;
    std::vector<std::string> m_ScenesPaths;
    unsigned char m_MasksOffset;
    
	void loadFilenames(string dir, vector<string>& filenames);
    // Load frames of a modality within a directory
    void loadDataToMats(string dir, const char* format, vector<cv::Mat> & frames);
    // Load frames and frames' indices of a modality within a directory
    void loadDataToMats(string dir, const char* format, vector<cv::Mat> & frames, vector<string>& indices);
    // Load people bounding boxes (rects)
    void loadBoundingRects(std::string file, std::vector< std::vector<cv::Rect> >& rects, std::vector< std::vector<int> >& tags);
    // Save calibVars files directories
    void loadCalibVarsDir(string dir, vector<string>& calibVarsDirs);
};

#endif /* defined(__segmenthreetion__ModalityReader__) */
