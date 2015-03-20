#include "camera_custom_ot.h"

void get_ot_CustomizeData(OT_Customize_PARA  *OTDataOut)
{
    OTDataOut->OBLoseTrackingFrm = 90;
    OTDataOut->OCLoseTrackingFrm = 90;
    OTDataOut->LtOcOb_ColorSimilarity_TH = 0.37;
    OTDataOut->ARFA = 0.00;
    OTDataOut->Numiter_shape_F = 1;
    OTDataOut->LightResistance = 0;
    OTDataOut->MaxObjHalfSize = 40;
    OTDataOut->MinObjHalfSize = 12;
    OTDataOut->IniwinW = 15;
    OTDataOut->IniwinH = 15;
    OTDataOut->AEAWB_LOCK = 1;
}
