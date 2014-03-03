//
//  GridPartitioner.cpp
//  segmenthreetion
//
//  Created by Albert Clapés on 01/03/14.
//
//

#include "GridPartitioner.h"


GridPartitioner::GridPartitioner()
        : m_hp(2), m_wp(2)
{

}


GridPartitioner::GridPartitioner(unsigned int hp, unsigned int wp)
: m_hp(hp), m_wp(wp)
{
    
}


void GridPartitioner::setGridPartitions(unsigned int hp, unsigned int wp)
{
    m_hp = hp;
    m_wp = wp;
}


void GridPartitioner::grid(ModalityData& md, ModalityGridData& mgd)
{
    if (!md.isFilled())
        return; // do nothing
    
    vector<GridMat> gframes;
    vector<GridMat> gmasks;
    cv::Mat tags;
    
    // Grid frames and masks
    grid(md, gframes);
    grid(md, gmasks, tags);
    //visualizeGridmats(gframes_train); // DEBUG
    //visualizeGridmats(gmasks_train); // DEBUG
    
    mgd.setGridFrames(gframes);
    mgd.setGridMasks(gmasks);
    mgd.setTags(tags);
}


/*
 * Trim subimages, defined by rects (bounding boxes), from image frames
 */
void GridPartitioner::grid(ModalityData& md, vector<GridMat>& grids)
{
    //namedWindow("grided subject");
    // Seek in each frame ..
    for (unsigned int f = 0; f < md.getFrames().size(); f++)
    {
        vector<cv::Rect> rects = md.getBoundingRectsInFrame(f);
        // .. all the people appearing
        for (unsigned int r = 0; r < rects.size(); r++)
        {
            if (rects[r].height >= m_hp && rects[r].width >= m_wp)
            {
                cv::Mat subject (md.getFrame(f), rects[r]); // Get a roi in frame defined by the rectangle.
                cv::Mat maskedSubject;
                subject.copyTo(maskedSubject, md.getMask(f,r));
                subject.release();
                
                GridMat g (maskedSubject, m_hp, m_wp);
                grids.push_back( g );
            }
        }
    }
}


/*
 * Trim subimages, defined by rects (bounding boxes), from image frames
 */
void GridPartitioner::grid(ModalityData& md, vector<GridMat>& grids, cv::Mat& tags)
{
    vector<int> tagsAux;
    
    // Seek in each frame ..
    for (unsigned int f = 0; f < md.getFrames().size(); f++)
    {
        vector<cv::Rect> rects = md.getBoundingRectsInFrame(f);
        vector<int> tags = md.getTagsInFrame(f);
        // .. all the people appearing
        for (unsigned int r = 0; r < rects.size(); r++)
        {
            if (rects[r].height >= m_hp && rects[r].width >= m_wp)
            {
                cv::Mat subject (md.getFrame(f), rects[r]); // Get a roi in frame defined by the rectangle.
                cv::Mat maskedSubject;
                subject.copyTo(maskedSubject, md.getMask(f,r));
                subject.release();
                
                GridMat g (maskedSubject, m_hp, m_wp);
                grids.push_back( g );
                
                tagsAux.push_back(tags[r]);
            }
        }
    }
    
    cv::Mat tmp (tagsAux.size(), 1, cv::DataType<int>::type, tagsAux.data());
    tmp.copyTo(tags);
}