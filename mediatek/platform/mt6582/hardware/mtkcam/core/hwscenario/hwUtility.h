
#ifndef HW_UTILITY_H
#define HW_UTILITY_H

EScenarioFmt
mapSensorType(halSensorType_e const & type);

/*******************************************************************************
*
********************************************************************************/
MVOID 
mapPortCfg(EHwBufIdx const src, PortID &dst);

/*******************************************************************************
*
********************************************************************************/
MVOID
mapBufCfg(IhwScenario::PortBufInfo const &src, QBufInfo &dst);

/*******************************************************************************
*
********************************************************************************/
MVOID 
mapConfig(IhwScenario::PortBufInfo const &rsrc, PortID &rPortID, QBufInfo &rQbufInfo);

/*******************************************************************************
*
********************************************************************************/
MVOID 
mapFormat(const char * const src, EImageFormat &dst);


#endif


