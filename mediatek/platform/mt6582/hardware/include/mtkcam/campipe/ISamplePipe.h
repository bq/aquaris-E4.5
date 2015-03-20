
#ifndef _MTK_CAMERA_INC_CAMPIPE_ISAMPLE_PIPE_H_
#define _MTK_CAMERA_INC_CAMPIPE_ISAMPLE_PIPE_H_

/*******************************************************************************
*
********************************************************************************/
namespace NSCamPipe {

/**  
 * @class ISamplePipe
 * @brief Interface of SamplePipe 
 * @details 
 * This is just a sample pipe.  
 *
 */
class ISamplePipe : public IPipe
{
public:
    static EPipeID const ePipeID = ePipeID_0x0_NONE;

public:     ////    Instantiation.
    /**
     * @brief Create the Sample pipe instance 
     *
     * @details      
     *
     * @note 
     * 
     * @param[in] eSWScenarioID: The SW scenario 
     * @param[in] eScenarioFmt: The SW sensor scenario format 
     *      
     * @return 
     * The IPostProcPipe instance. 
     *
     */   
    static ISamplePipe* createInstance(ESWScenarioID const eSWScenarioID, EScenarioFmt const eScenarioFmt);

public:
    /**
     * @brief The old style sendcommand for extension
     *
     * @details      
     *
     * @note 
     * 
     * @param[in] cmd: the input command 
     * @param[in] arg1: input argument 1        
     * @param[in] arg2: input argument 2             
     * @param[in] arg3: input argument 3                
     *      
     * @return 
     * - MTRUE indicates success; 
     * - MFALSE indicates failure, and an error code can be retrived by getLastErrorCode().     
     *
     */ 		
    virtual MBOOL   sendCommand(MINT32 cmd, MINT32 arg1, MINT32 arg2, MINT32 arg3) = 0; 
}; 

}; //namespace NCCamPipe
#endif  //  _MTK_CAMERA_INC_CAMPIPE_IPOSTPROC_PIPE_H_


