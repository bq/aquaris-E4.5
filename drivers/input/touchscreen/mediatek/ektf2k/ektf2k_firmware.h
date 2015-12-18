#ifdef IAP_PORTION

/*
const u8 shenyue_fw[]=
{
#include "shenyue_03.i"
};
const u8 dijing_fw[]=
{
#include "dijing_S4010_02.i"
};
*/
const u8 ERA_huaruichuan_fw[]=
{
#include "Wally_for_ckt_ERA02A_fwvga_0xc303.i"
};

const u8 ERA_helitai_fw[]=
{
#include "Holitech_for_ckt_ERA02A_fwvga_0xc303.i"
};

struct vendor_map
{
	int vendor_id;
	char vendor_name[30];
	uint8_t* fw_array;
};
const struct vendor_map g_vendor_map[]=
{
	{0x14f0,"huaruichuan",ERA_huaruichuan_fw},
	{0x14f3,"holitech",ERA_helitai_fw}
};

#endif/*IAP_PORTION*/
