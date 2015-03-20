
#ifndef _MTK_CAMERA_INC_CAMPIPE_ICAMIO_PIPE_H_
#define _MTK_CAMERA_INC_CAMPIPE_ICAMIO_PIPE_H_

/*******************************************************************************
*
********************************************************************************/
namespace NSCamPipe {

/**  
 * @enum ECamIOPipeCmd
 * @brief CamIO pipe command
 */
enum ECamIOPipeCmd {
    ECamIOPipeCmd_QUERY_BAYER_RAW_SRIDE  = 0x1001,         /*!<  for query bayer raw stride.  */
}; 
    

/**  
 * @class ICamIOPipe
 * @brief Interface of CamIOPipe 
 * @details 
 * ZSD Scenario is used to wraper the camIO pipe. \n
 * The data path will be TG --> ISP --> Mem, used to dump image from sensor \n
 *
 */
class ICamIOPipe : public IPipe
{
public:
    static EPipeID const ePipeID = ePipeID_1x2_Sensor_Tg_Isp_Mem;

public:     ////    Instantiation.
    /**
     * @brief Create the CamIOPipe instance 
     *
     * @details      
     *
     * @note 
     * 
     * @param[in] eSWScenarioID: The SW scenario 
     * @param[in] eScenarioFmt: The SW sensor scenario format 
     *      
     * @return 
     * The ICamIOPipe instance. 
     *
     */   
    static ICamIOPipe* createInstance(ESWScenarioID const eSWScenarioID, EScenarioFmt const eScenarioFmt);

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
    /**
     * @brief Wait for HW signal. 
     *
     * @details      
     *
     * @note 
     * Only support the pipe that IN port is from sensor 
     * 
     * @param[in] ePipeSignal: The pipe signal 
     * @param[in] u4TimeoutMs: The time out in ms 
     *      
     * @return 
     * None 
     */        
    virtual MVOID   waitSignal(EPipeSignal ePipeSignal, MUINT32 const u4TimeoutMs = 0xFFFFFFFF) = 0; 
    /**
     * @brief Start to do one shot. 
     *
     * @details      
     *
     * @note 
     * Only suuport the pipe that IN port is from sensor 
     *      
     * @return 
     * - MTRUE indicates success; 
     * - MFALSE indicates failure, and an error code can be retrived by getLastErrorCode().     
     *
     */      
    virtual MBOOL   startOne(); 


}; 

}; //namespace NCCamPipe
#endif  //  _MTK_CAMERA_INC_CAMPIPE_ICAMIO_PIPE_H_


