#!/usr/local/bin/perl -w
#
#****************************************************************************/
#*   This script will generate partition layout files
#* Author: Kai Zhu (MTK81086)
#* 
#****************************************************************************/

#****************************************************************************
# Included Modules
#****************************************************************************
use File::Basename;
use File::Path;
my $Version=3.7;
#my $ChangeHistory="3.1 AutoDetect eMMC Chip and Set MBR_Start_Address_KB\n";
#my $ChangeHistory = "3.2 Support OTP\n";
#my $ChangeHistory = "3.3 Support Shared SD Card\n";
#my $ChangeHistory = "3.4 CIP support\n";
#my $ChangeHistory = "3.5 Fix bug\n";
#my $ChangeHistory = "3.6 Support YAML format scatter file\n";
my $ChangeHistory = "3.7 change output file path\n";
# Partition_table.xls arrays and columns
my @PARTITION_FIELD ;
my @START_FIELD_Byte ;
my @START_ADDR_PHY_Byte_HEX;
my @START_FIELD_Byte_HEX;
my @SIZE_FIELD_KB ;
my @TYPE_FIELD;
my @DL_FIELD ;
my @PARTITION_IDX_FIELD ;
my @REGION_FIELD ;
my @RESERVED_FIELD;
my @BR_INDEX;
my @FB_ERASE_FIELD;
my @FB_DL_FIELD;

my $COLUMN_PARTITION                = 1 ;
my $COLUMN_TYPE                     = $COLUMN_PARTITION + 1 ;
my $COLUMN_SIZE                     = $COLUMN_TYPE + 1 ;
my $COLUMN_SIZEKB                   = $COLUMN_SIZE + 1 ;
my $COLUMN_SIZE2                    = $COLUMN_SIZEKB + 1 ;
my $COLUMN_SIZE3                    = $COLUMN_SIZE2 + 1 ;
my $COLUMN_DL                       = $COLUMN_SIZE3 + 1 ;
my $COLUMN_FB_ERASE                 = $COLUMN_DL + 1;  # fastboot support
my $COLUMN_FB_DL                    = $COLUMN_FB_ERASE + 1;  # fastboot support
# emmc support
my $COLUMN_REGION		    = $COLUMN_FB_DL + 1;
my $COLUMN_RESERVED		    = $COLUMN_REGION + 1;

my $PMT_END_NAME;  #PMT_END_NAME
#eMMC 
#EXT4 Partition Size
my $SECRO_SIZE;
my $USERDATA_SIZE;
my $SYSTEM_SIZE;
my $CACHE_SIZE;

my $total_rows = 0 ; #total_rows in partition_table
my $User_Region_Size_KB; #emmc USER region start_address
my $MBR_Start_Address_KB;	#KB
my $Page_Size	=2; # default NAND page_size of nand
my $AutoModify	=0;
my $DebugPrint    = 1; # 1 for debug; 0 for non-debug
my $LOCAL_PATH;
my $SCAT_NAME;
my $SHEET_NAME;
my $Min_user_region = 0;
my %preloader_alias; #alias for preloader c and h files modify
my %kernel_alias;	#alias for kernel c  file modify
my $PRODUCT_OUT;
my $custom_out_prefix;
my $configs_out_prefix;
my $preloader_out_prefix;


BEGIN
{
  $LOCAL_PATH = dirname($0);
}

use lib "$LOCAL_PATH/../../Spreadsheet";
use lib "$LOCAL_PATH/../../";
require 'ParseExcel.pm';
use pack_dep_gen;

#parse argv from alps/mediatek/config/{project}/ProjectConfig.mk
$PLATFORM = $ENV{MTK_PLATFORM};
$platform = lc($PLATFORM);
$PROJECT = $ENV{PROJECT};
$FULL_PROJECT = $ENV{FULL_PROJECT};
$LCA_PRJ = $ENV{MTK_LCA_SUPPORT};
$PAGE_SIZE = $ENV{MTK_NAND_PAGE_SIZE};
$EMMC_SUPPORT= $ENV{MTK_EMMC_SUPPORT};
$LDVT_SUPPORT= $ENV{MTK_LDVT_SUPPORT};
$MTK_EMMC_OTP_SUPPORT= $ENV{MTK_EMMC_SUPPORT_OTP};
$MTK_SHARED_SDCARD=$ENV{MTK_SHARED_SDCARD};
$MTK_DRM_KM_SUPPORT = $ENV{MTK_DRM_KEY_MNG_SUPPORT};
$MTK_IH_TEE_SUPPORT = $ENV{MTK_IN_HOUSE_TEE_SUPPORT};
$TARGET_BUILD_VARIANT=$ENV{TARGET_BUILD_VARIANT};
$MTK_CIP_SUPPORT = $ENV{MTK_CIP_SUPPORT};
$TRUSTONIC_TEE_SUPPORT = $ENV{TRUSTONIC_TEE_SUPPORT};
$MTK_PERSIST_PARTITION_SUPPORT=$ENV{MTK_PERSIST_PARTITION_SUPPORT};
$MTK_NAND_UBIFS_SUPPORT=$ENV{MTK_NAND_UBIFS_SUPPORT};
$PL_MODE=$ENV{PL_MODE};
$YAML_SUPPORT=$ENV{MTK_YAML_SCATTER_FILE_SUPPORT};
&InitGlobalValue();

my $PART_TABLE_FILENAME   = "mediatek/build/tools/ptgen/$PLATFORM/partition_table_${PLATFORM}.xls"; # excel file name
my $REGION_TABLE_FILENAME = "mediatek/build/tools/emigen/$PLATFORM/MemoryDeviceList_${PLATFORM}.xls";  #eMMC region information
my $CUSTOM_MEMORYDEVICE_H_NAME  = "mediatek/custom/$PROJECT/preloader/inc/custom_MemoryDevice.h";

my $EMMC_COMPO	= "mediatek/config/$PROJECT/mbr_addr.pl" ;

my $PARTITION_DEFINE_H_NAME     = "$custom_out_prefix/common/partition_define.h"; 
my $PARTITION_DEFINE_C_NAME		= "$custom_out_prefix/kernel/partition_define_private.h";
my $EMMC_PART_SIZE_LOCATION		= "$configs_out_prefix/configs/partition_size.mk";
my $PMT_H_NAME          = "$custom_out_prefix/common/pmt.h";
my $PreloaderC	="$preloader_out_prefix/cust_part.c";
my $KernelH 	="$custom_out_prefix/kernel/partition.h";
my $LK_MT_PartitionH = "$custom_out_prefix/lk/inc/mt_partition.h";
my $LK_PartitionC = "$custom_out_prefix/lk/partition.c";

my $SCAT_NAME_DIR   = $PRODUCT_OUT; 

#Set SCAT_NAME
mkdir($SCAT_NAME_DIR) if (!-d $SCAT_NAME_DIR);
if($YAML_SUPPORT eq "yes"){
	$SCAT_NAME = "${SCAT_NAME_DIR}/${PLATFORM}_Android_scatter.txt";
}else{
if ($LCA_PRJ eq "yes")
{
    $SCAT_NAME ="$SCAT_NAME_DIR/${PLATFORM}_Android_scatter_LCA.txt" ;
}
if ($EMMC_SUPPORT eq "yes") 
{
     $SCAT_NAME = "$SCAT_NAME_DIR/${PLATFORM}_Android_scatter_emmc.txt" ;
}else{
     $SCAT_NAME = "$SCAT_NAME_DIR/${PLATFORM}_Android_scatter.txt" ;
}
}

#Set SHEET_NAME
if($EMMC_SUPPORT eq "yes"){
	$SHEET_NAME = "emmc";
	if($MTK_EMMC_OTP_SUPPORT eq "yes"){
		$SHEET_NAME = $SHEET_NAME ." otp" ;
	}
}else{
	$SHEET_NAME = "nand " . $PAGE_SIZE ;
	if($PAGE_SIZE=~/(\d)K/){
		$Page_Size=$1;
	}else{
		$Page_Size=2;	
	}
}


if( ($MTK_DRM_KM_SUPPORT eq "yes") || ($MTK_IH_TEE_SUPPORT eq "yes") ){
    $SHEET_NAME = $SHEET_NAME . " drm";
}

if(!defined $TARGET_BUILD_VARIANT || $TARGET_BUILD_VARIANT eq ""){
	$SHEET_NAME = $SHEET_NAME . " eng";
}else{
	if($TARGET_BUILD_VARIANT eq "eng"){
		$SHEET_NAME = $SHEET_NAME . " eng";
	}else{
		$SHEET_NAME = $SHEET_NAME . " user"; 
}
}
if($LDVT_SUPPORT eq "yes"){
	$SHEET_NAME = "ldvt";	
}

if($PL_MODE ne ""){
	$SHEET_NAME = lc($PL_MODE);
}

#****************************************************************************
# main thread
#****************************************************************************
# get already active Excel application or open new
PrintDependModule($0);
print "*******************Arguments*********************\n" ;
print "Version=$Version ChangeHistory:$ChangeHistory\n";
print "PLATFORM = $ENV{MTK_PLATFORM};
PROJECT = $ENV{PROJECT};
FULL_PROJECT =$FULL_PROJECT
platform = $ENV{MTK_PLATFORM};
LCA_PRJ = $ENV{MTK_LCA_SUPPORT};
PAGE_SIZE = $ENV{MTK_NAND_PAGE_SIZE};
EMMC_SUPPORT= $ENV{MTK_EMMC_SUPPORT};
LDVT_SUPPORT= $ENV{MTK_LDVT_SUPPORT};
TARGET_BUILD_VARIANT= $ENV{TARGET_BUILD_VARIANT};
MTK_EMMC_OTP_SUPPORT= $ENV{MTK_EMMC_SUPPORT_OTP};
MTK_SHARED_SDCARD=$ENV{MTK_SHARED_SDCARD};
MTK_CIP_SUPPORT=$ENV{MTK_CIP_SUPPORT};
TRUSTONIC_TEE_SUPPORT=$ENV{TRUSTONIC_TEE_SUPPORT};
MTK_NAND_UBIFS_SUPPORT=$ENV{MTK_NAND_UBIFS_SUPPORT};
PL_MODE=$ENV{PL_MODE};
MTK_YAML_SCATTER_FILE_SUPPORT=$YAML_SUPPORT;
\n";
print "SHEET_NAME=$SHEET_NAME\n";
print "SCAT_NAME=$SCAT_NAME\n" ;

print "*******************Arguments*********************\n\n\n\n" ;
&clear_files();
if ($EMMC_SUPPORT eq "yes"){
	&GetMBRStartAddress();
}

$PartitonBook = Spreadsheet::ParseExcel->new()->Parse($PART_TABLE_FILENAME);
PrintDependency($PART_TABLE_FILENAME);
&InitAlians();

&ReadExcelFile () ;

&GenHeaderFile () ;

if($YAML_SUPPORT eq "yes"){
&GenYAMLScatFile();
}else{
&GenScatFile () ;
}

if ($EMMC_SUPPORT eq "yes" && $PL_MODE eq ""){
	&GenMBRFile ();
	&GenPartSizeFile ();
}
&GenPerloaderCust_partC();
&GenPmt_H();

if($PL_MODE ne ""){
	print "**********Ptgen Done********** ^_^\n" ;

	print "\n\nPtgen Generated files list:\n$SCAT_NAME\n$PARTITION_DEFINE_H_NAME\n$PreloaderC\n$PMT_H_NAME  \n\n\n\n\n";

	exit ;
}
&GenLK_PartitionC();
&GenLK_MT_PartitionH();
if($EMMC_SUPPORT ne "yes"){
	&GenKernel_PartitionC();
}

print "**********Ptgen Done********** ^_^\n" ;

print "\n\nPtgen modified or Generated files list:\n$SCAT_NAME\n$PARTITION_DEFINE_H_NAME\n$PART_SIZE_LOCATION\n/out/MBR EBR1 EBR2 \n\n\n\n\n";

exit ;

sub GetMBRStartAddress(){
	my %REGION_TABLE;
	my $BOOT1;
	my $BOOT2;
	my $RPMB;
	my $USER;
# @REGION_TABLE = 
#{ EMMC_CHIP_PART_NUM=>{
#   PART_NUM;
#	VENDER;
#	USER;
#	BOOT1;
#	BOOT2;
#	RPMB; 
#  },
#...
#]
	my $EMMC_REGION_SHEET_NAME = "emmc_region";
	my $emmc_sheet;
	my $region_name;
	my $region = 0;
	my $boot1 = 2;
	my $boot2 = 3;
	my $rpmb = 4;
	my $user = 9;
	my $EMMC_RegionBook = Spreadsheet::ParseExcel->new()->Parse($REGION_TABLE_FILENAME);
	PrintDependency($REGION_TABLE_FILENAME);

	$emmc_sheet = get_sheet($EMMC_REGION_SHEET_NAME,$EMMC_RegionBook) ;
	unless ($emmc_sheet)
	{
		my $error_msg="Ptgen CAN NOT find sheet=$EMMC_REGION_SHEET_NAME in $REGION_TABLE_FILENAME\n";
		print $error_msg;
		die $error_msg;
	}

	my $row = 1;
    $region_name = &xls_cell_value($emmc_sheet, $row, $region,$EMMC_REGION_SHEET_NAME);
	while($region_name ne "END"){
		$region_name	=~ s/\s+//g;
		$BOOT1     = &xls_cell_value($emmc_sheet, $row, $boot1,$EMMC_REGION_SHEET_NAME);
		$BOOT2     = &xls_cell_value($emmc_sheet, $row, $boot2,$EMMC_REGION_SHEET_NAME);
		$RPMB   = &xls_cell_value($emmc_sheet, $row, $rpmb,$EMMC_REGION_SHEET_NAME);
		$USER	= &xls_cell_value($emmc_sheet, $row, $user,$EMMC_REGION_SHEET_NAME);
		$REGION_TABLE{$region_name}	= {BOOT1=>$BOOT1,BOOT2=>$BOOT2,RPMB=>$RPMB,USER=>$USER};
		print "In $region_name,$BOOT1,$BOOT2,$RPMB,$USER\n";
		$row++;
		$region_name = &xls_cell_value($emmc_sheet, $row, $region,$EMMC_REGION_SHEET_NAME);
	}

	if (-e $CUSTOM_MEMORYDEVICE_H_NAME) {
		`chmod 777 $CUSTOM_MEMORYDEVICE_H_NAME`;
	}
    open (CUSTOM_MEMORYDEVICE_H_NAME, "<$CUSTOM_MEMORYDEVICE_H_NAME") or &error_handler("Ptgen open CUSTOM_MEMORYDEVICE_H_NAME fail!\n", __FILE__, __LINE__);
    PrintDependency($CUSTOM_MEMORYDEVICE_H_NAME);
	my @lines;
	my $iter = 0;
	my $part_num;	
	my $MAX_address = 0;
	my $combo_start_address = 0;
	my $cur=0;
	my $cur_user=0;
	while (<CUSTOM_MEMORYDEVICE_H_NAME>) {
		my($line) = $_;
  		chomp($line);
		if ($line =~ /^#define\sCS_PART_NUMBER\[[0-9]\]/) {
#			print "$'\n";
			$lines[$iter] = $';
			$lines[$iter] =~ s/\s+//g;
			if ($lines[$iter] =~ /(.*)\/\/(.*)/) {
				$lines[$iter] =$1;
			}
			#print "$lines[$iter] \n";
			$iter ++;
		}
			
	}
	foreach $part_num (@lines) {
		if(exists $REGION_TABLE{$part_num}){
			$cur = $REGION_TABLE{$part_num}{BOOT1} + $REGION_TABLE{$part_num}{BOOT2} + $REGION_TABLE{$part_num}{RPMB};
			$cur_user = $REGION_TABLE{$part_num}{USER};
			print "Chose region layout: $part_num, $REGION_TABLE{$part_num}{BOOT1} + $REGION_TABLE{$part_num}{BOOT2} + $REGION_TABLE{$part_num}{RPMB}=$cur \$REGION_TABLE{\$part_num}{USER}=$cur_user\n";
			if ($cur > $MAX_address) {
				$MAX_address = $cur;
			}
			if($cur_user < $Min_user_region || $Min_user_region == 0){
				$Min_user_region = 	$cur_user;			
			}
			#print "\$Min_user_region=$Min_user_region\n";
		}else{
			$MAX_address = 20*1024; #default Fix me!!!
			my $error_msg="ERROR:Ptgen CAN NOT find $part_num in $REGION_TABLE_FILENAME, we set MAB addr $MAX_address as default value\n";
			print $error_msg;
#			die $error_msg;
		}
	}
	print "The MAX BOOT1+BOOT2+RPMB=$MAX_address  \$Min_user_region=$Min_user_region in $CUSTOM_MEMORYDEVICE_H_NAME\n";	

	if (-e $EMMC_COMPO)
	{
		`chmod 777 $EMMC_COMPO`;
		$combo_start_address = do "$EMMC_COMPO";
		PrintDependency($EMMC_COMPO);
	}else{
#		print "No $EMMC_COMPO Set MBR_Start_Address_KB=6MB\n";
#		$combo_start_address = 6*1024; #Fix Me!!!
		print "No $EMMC_COMPO\n";
	}

	if ($MAX_address < $combo_start_address) {
		$MBR_Start_Address_KB = $combo_start_address;
		print "Get MBR_Start_Address_KB from $EMMC_COMPO = $combo_start_address\n";
	}else{
		$MBR_Start_Address_KB = $MAX_address;
		print "Get MBR_Start_Address_KB from $CUSTOM_MEMORYDEVICE_H_NAME = $MAX_address\n";
	}
}

#****************************************************************************
# subroutine:  InitAlians
# return:      
#****************************************************************************
sub InitAlians(){
	$preloader_alias{"SECCFG"}="SECURE";
	$preloader_alias{"SEC_RO"}="SECSTATIC";
	$preloader_alias{"ANDROID"}="ANDSYSIMG";
	$preloader_alias{"USRDATA"}="USER";

	$lk_xmodule_alias{"DSP_BL"}="DSP_DL";
	$lk_xmodule_alias{"SECCFG"}="SECURE";
	$lk_xmodule_alias{"SEC_RO"}="SECSTATIC";
	$lk_xmodule_alias{"EXPDB"}="APANIC";
	$lk_xmodule_alias{"ANDROID"}="ANDSYSIMG";
	$lk_xmodule_alias{"USRDATA"}="USER";
	$kernel_alias{"SECCFG"}="seccnfg";
	$kernel_alias{"BOOTIMG"}="boot";
	$kernel_alias{"SEC_RO"}="secstatic";
	$kernel_alias{"ANDROID"}="system";
	$kernel_alias{"USRDATA"}="userdata";

	$lk_alias{"BOOTIMG"}="boot";
	$lk_alias{"ANDROID"}="system";
	$lk_alias{"USRDATA"}="userdata";
}

#****************************************************************************
# subroutine:  ReadExcelFile
# return:      
#****************************************************************************

sub ReadExcelFile()
{
    my $sheet = get_sheet($SHEET_NAME,$PartitonBook) ;
	PrintDependency($REGION_TABLE_FILENAME);
    unless ($sheet)
    {
		my $error_msg="Ptgen CAN NOT find sheet=$SHEET_NAME in $PART_TABLE_FILENAME\n";
		print $error_msg;
		die $error_msg;
    }
    my $row_t = 1;
	my $row = 1 ;
    my $pt_name = &xls_cell_value($sheet, $row, $COLUMN_PARTITION,$SHEET_NAME);
	my $px_index = 1;
	my $px_index_t = 1;
	my $br_index = 0;
	my $p_count = 0;
	my $br_count =0;
	while($pt_name ne "END"){
		$type		 = &xls_cell_value($sheet, $row, $COLUMN_TYPE,$SHEET_NAME) ;
		if($type eq "EXT4" || $type eq "FAT" ){
			if($pt_name eq "FAT" && $MTK_SHARED_SDCARD eq "yes"){
				print "Skip FAT because of MTK_SHARED_SDCARD On\n";
  			}elsif($pt_name eq "CUSTOM" && $MTK_CIP_SUPPORT ne "yes"){
	  		print "Skip CUSTOM because of MTK_CIP_SUPPORT off\n";
            }elsif($pt_name eq "TEE1" && ($TRUSTONIC_TEE_SUPPORT ne "yes" && $MTK_IH_TEE_SUPPORT ne "yes")){
	  			print "Skip TEE1 because of TRUSTONIC_TEE_SUPPORT and MTK_IH_TEE_SUPPORT off\n";
	  	    }elsif($pt_name eq "TEE2" && ($TRUSTONIC_TEE_SUPPORT ne "yes" && $MTK_IH_TEE_SUPPORT ne "yes")){
	  			print "Skip TEE2 because of TRUSTONIC_TEE_SUPPORT and MTK_IH_TEE_SUPPORT off\n";
			}elsif($pt_name eq "PERSIST" && $MTK_PERSIST_PARTITION_SUPPORT ne "yes"){
	  			print "Skip persist because of MTK_PERSIST_PARTITION_SUPPORT off\n";
			}else{
						$p_count++;			
			}			
		}
		$row++;
		$pt_name = &xls_cell_value($sheet, $row, $COLUMN_PARTITION,$SHEET_NAME);
	}
	$br_count = int(($p_count+2)/3)-1;
	
	$row =1;
	$pt_name = &xls_cell_value($sheet, $row, $COLUMN_PARTITION,$SHEET_NAME);
	my $tmp_index=1;
	while($pt_name ne "END"){
		if($pt_name eq "FAT" && $MTK_SHARED_SDCARD eq "yes"){
			print "Skip FAT because of MTK_SHARED_SDCARD On\n";
		}elsif($pt_name eq "CUSTOM" && $MTK_CIP_SUPPORT ne "yes"){
			print "Skip CUSTOM because of MTK_CIP_SUPPORT off\n";
                }elsif($pt_name eq "TEE1" && ($TRUSTONIC_TEE_SUPPORT ne "yes" && $MTK_IH_TEE_SUPPORT ne "yes")){
			print "Skip TEE1 because of TRUSTONIC_TEE_SUPPORT and MTK_IH_TEE_SUPPORT off\n";
                }elsif($pt_name eq "TEE2" && ($TRUSTONIC_TEE_SUPPORT ne "yes" && $MTK_IH_TEE_SUPPORT ne "yes")){
			print "Skip TEE2 because of TRUSTONIC_TEE_SUPPORT and MTK_IH_TEE_SUPPORT off\n";
		}elsif($pt_name eq "PERSIST" && $MTK_PERSIST_PARTITION_SUPPORT ne "yes"){
			print "Skip persist because of MTK_PERSIST_PARTITION_SUPPORT off\n";
		}else{
		$PARTITION_FIELD[$row_t -1] = $pt_name;
		$SIZE_FIELD_KB[$row_t -1]    = &xls_cell_value($sheet, $row, $COLUMN_SIZEKB,$SHEET_NAME) ;
		$DL_FIELD[$row_t-1]        = &xls_cell_value($sheet, $row, $COLUMN_DL,$SHEET_NAME) ;
		$TYPE_FIELD[$row_t -1]		 = &xls_cell_value($sheet, $row, $COLUMN_TYPE,$SHEET_NAME) ;
		$FB_DL_FIELD[$row_t-1]    = &xls_cell_value($sheet, $row, $COLUMN_FB_DL,$SHEET_NAME) ;
 		$FB_ERASE_FIELD[$row_t-1]    = &xls_cell_value($sheet, $row, $COLUMN_FB_ERASE,$SHEET_NAME) ;
		if ($EMMC_SUPPORT eq "yes")
		{
        		$REGION_FIELD[$row_t-1]        = &xls_cell_value($sheet, $row, $COLUMN_REGION,$SHEET_NAME) ;
        		$RESERVED_FIELD[$row_t-1]		= &xls_cell_value($sheet, $row, $COLUMN_RESERVED,$SHEET_NAME) ;
			if($TYPE_FIELD[$row_t -1] eq "EXT4" || $TYPE_FIELD[$row_t -1] eq "FAT"){
				$PARTITION_IDX_FIELD[$row_t-1] = $px_index;
				$BR_INDEX[$px_index] = int(($px_index_t+2)/3)-1;
				$px_index++;
				$px_index_t++;
			}else{
				$PARTITION_IDX_FIELD[$row_t-1] = 0;
			}
	##add EBR1 after MBR
			if($pt_name =~ /MBR/ && ($br_count >= 1)){
				
					$row_t++;
					$PARTITION_FIELD[$row_t-1] = "EBR1";
					$SIZE_FIELD_KB[$row_t -1] = 512;
					$DL_FIELD[$row_t-1] = 1;
					if($TARGET_BUILD_VARIANT eq "user"){
						$FB_DL_FIELD[$row_t-1] = 0;
						$FB_ERASE_FIELD[$row_t-1] = 0;
					}else{
						$FB_DL_FIELD[$row_t-1] = 1;
						$FB_ERASE_FIELD[$row_t-1] = 1;					
					}

					$REGION_FIELD[$row_t-1] = "USER";
					$TYPE_FIELD[$row_t -1] = "Raw data";
					$RESERVED_FIELD[$row_t-1]= 0 ;

					print "ebr $px_index $br_count\n";
					$PARTITION_IDX_FIELD[$row_t-1]=$px_index;
					$BR_INDEX[$px_index] = 0;	
					$px_index++;			

				
			}
          ##add EBR2~ after LOGO
			if(($br_count >= 2) && $pt_name =~ /LOGO/){
				for($tmp_index=2;$tmp_index<=$br_count;$tmp_index++){
					$row_t++;
					$PARTITION_FIELD[$row_t-1] = sprintf("EBR%d",$tmp_index);
					$SIZE_FIELD_KB[$row_t -1] = 512;
					$DL_FIELD[$row_t-1] = 1;
					if($TARGET_BUILD_VARIANT eq "user"){
						$FB_DL_FIELD[$row_t-1] = 0;
						$FB_ERASE_FIELD[$row_t-1] = 0;
					}else{
						$FB_DL_FIELD[$row_t-1] = 1;
						$FB_ERASE_FIELD[$row_t-1] = 1;					
					}				

					$REGION_FIELD[$row_t-1] = "USER";
					$TYPE_FIELD[$row_t -1] = "Raw data";
					$RESERVED_FIELD[$row_t-1] = 0 ;

					$PARTITION_IDX_FIELD[$row_t-1]=0;
					
				}
			}
		}
		$row_t++;
		}
		$row++;

		$pt_name = &xls_cell_value($sheet, $row, $COLUMN_PARTITION,$SHEET_NAME);
		
	}
	
	    #modify size for some part
    my $board_config = &open_for_read("mediatek/config/$PROJECT/BoardConfig.mk");
    my $iter;
    if ($board_config)
    {
        my $line;
        while (defined($line = <$board_config>))
        {
            for($iter=0;$iter< @PARTITION_FIELD;$iter++)
            {
                my $part_name = $PARTITION_FIELD[$iter];
                if ($line =~ /\A\s*BOARD_MTK_${part_name}_SIZE_KB\s*:=\s*(\d+)/)
                {
                    $SIZE_FIELD_KB[$iter] = $1;
                    print "by project size $part_name = $1 KB\n";
                }
            }
        }
        close $board_config;
    }
    if ($FULL_PROJECT ne $PROJECT)
    {
        my $flavor_board_config = &open_for_read("mediatek/config/$FULL_PROJECT/BoardConfig.mk");
        if ($flavor_board_config)
        {
            my $line;
            while (defined($line = <$flavor_board_config>))
            {
            	for($iter=0;$iter< @PARTITION_FIELD;$iter++)
                {
                    my $part_name = $PARTITION_FIELD[$iter];
                    if ($line =~ /\A\s*BOARD_MTK_${part_name}_SIZE_KB\s*:=\s*(\d+)/)
                    {
                    	$SIZE_FIELD_KB[$iter] = $1;
                        print "by flavor project size $part_name = $1 KB\n";
                    }
                }
            }
            close $flavor_board_config;
        }
    }
    
    
#init start_address of partition
	$START_FIELD_Byte[0] = 0;	
	$PARTITION_IDX_FIELD[0] = 0;	
	my $otp_row;
	my $reserve_size;
	my $first_user = 1;
	for($row=1;$row < @PARTITION_FIELD;$row++){
		if($PL_MODE ne "" && $EMMC_SUPPORT eq "yes"){
			if($PARTITION_FIELD[$row] ne "MBR" && $REGION_FIELD[$row] eq "USER" && ($first_user == 1)){
				$START_FIELD_Byte[$row] = $MBR_Start_Address_KB*1024;
				$SIZE_FIELD_KB[$row-1] = ($START_FIELD_Byte[$row] - $START_FIELD_Byte[$row-1])/1024;
				$first_user = 0;
				next;			
			}		
		}
		if($PARTITION_FIELD[$row] eq "MBR"){
			$START_FIELD_Byte[$row] = $MBR_Start_Address_KB*1024;
			$SIZE_FIELD_KB[$row-1] = ($START_FIELD_Byte[$row] - $START_FIELD_Byte[$row-1])/1024;
			next;
		}
		if($PARTITION_FIELD[$row] eq "BMTPOOL" || $PARTITION_FIELD[$row] eq "OTP"){
		#	$START_FIELD_Byte[$row] = &xls_cell_value($sheet, $row+1, $COLUMN_START,$SHEET_NAME);
			$START_FIELD_Byte[$row] = $SIZE_FIELD_KB[$row]*1024;
			if($PARTITION_FIELD[$row] eq "OTP"){
					$otp_row = $row;
				}
			next; 
		}
		
		$START_FIELD_Byte[$row] = $START_FIELD_Byte[$row-1]+$SIZE_FIELD_KB[$row-1]*1024;
		
	}
	if($MTK_EMMC_OTP_SUPPORT eq "yes"){
		for($row=$otp_row+1;$row < @PARTITION_FIELD;$row++){
			 	if($RESERVED_FIELD[$row] == 1){
			 				$reserve_size += $START_FIELD_Byte[$row];
			 		}
			}
			$START_FIELD_Byte[$otp_row] += $reserve_size;
		}
#if($MTK_SHARED_SDCARD eq "yes"){
if(0){
	for($row=1;$row < @PARTITION_FIELD;$row++){
		if($REGION_FIELD[$row] eq "USER" && $PARTITION_FIELD[$row] ne "USRDATA"){
			$Min_user_region -= $SIZE_FIELD_KB[$row];
			print "$PARTITION_FIELD[$row]=$SIZE_FIELD_KB[$row] \$Min_user_region=$Min_user_region \n";
		}
	}
	$Min_user_region -= $MBR_Start_Address_KB;
	for($row=1;$row < @PARTITION_FIELD;$row++){
		if($PARTITION_FIELD[$row] eq "USRDATA" ){
			$SIZE_FIELD_KB[$row] = int($Min_user_region/1024)*1024;			
		}
	}
}
#convert dec start_address to hex start_address
	$START_FIELD_Byte_HEX[0]=0;
my $InUser=0;
	for($row=0;$row < @PARTITION_FIELD;$row++){
		if($PARTITION_FIELD[$row] eq "BMTPOOL" || $PARTITION_FIELD[$row] eq "OTP"){
			$START_FIELD_Byte_HEX[$row] = sprintf("0xFFFF%04x",$START_FIELD_Byte[$row]/(64*$Page_Size*1024));#$START_FIELD_Byte[$row];
			$START_ADDR_PHY_Byte_DEC[$row]=hex($START_FIELD_Byte_HEX[$row]);
		}else{
			$START_FIELD_Byte_HEX[$row] = sprintf("0x%x",$START_FIELD_Byte[$row]);
			if($PARTITION_FIELD[$row] eq "MBR"){
				$InUser=1;
			}
			if($InUser == 1){
			$START_ADDR_PHY_Byte_DEC[$row]=$START_FIELD_Byte[$row]-$MBR_Start_Address_KB*1024;
			}else{
			$START_ADDR_PHY_Byte_DEC[$row]=$START_FIELD_Byte[$row];
		}
			
		}
	}
	
	if($DebugPrint eq 1){
		for($row=0;$row < @PARTITION_FIELD;$row++){
			print "START=$START_FIELD_Byte_HEX[$row],		Partition=$PARTITION_FIELD[$row],		SIZE=$SIZE_FIELD_KB[$row],	DL_=$DL_FIELD[$row]" ;
			if ($EMMC_SUPPORT eq "yes"){
            	print ", 	Partition_Index=$PARTITION_IDX_FIELD[$row],	REGION =$REGION_FIELD[$row],RESERVE = $RESERVED_FIELD[$row]";
        	} 
			print "\n";
		}

	}

    $total_rows = @PARTITION_FIELD ;
	
	if ($total_rows == 0)
    {
        die "error in excel file no data!\n" ;
    } 
    print "There are $total_rows Partition totally!.\n" ;
}
#****************************************************************************
# subroutine:  GenHeaderFile
# return:      
#****************************************************************************
sub GenHeaderFile ()
{
    my $iter = 0 ;
    my $temp ;
	my $t;
	my $partition_define_h = &open_for_rw($PARTITION_DEFINE_H_NAME);
    
#write header
    print $partition_define_h &copyright_file_header_for_c();
    print $partition_define_h "\n#ifndef __PARTITION_DEFINE_H__\n#define __PARTITION_DEFINE_H__\n\n" ;
    print $partition_define_h "\n\n\n#define KB  (1024)\n#define MB  (1024 * KB)\n#define GB  (1024 * MB)\n\n" ;
#write part_name define
 	for ($iter=0; $iter< $total_rows; $iter++){
		$temp = "#define PART_$PARTITION_FIELD[$iter] \"$PARTITION_FIELD[$iter]\" \n";
		print $partition_define_h $temp ;
 	}
#preloader re-name
	print $partition_define_h "/*preloader re-name*/\n";
	for ($iter=0; $iter< $total_rows; $iter++){
		if($preloader_alias{$PARTITION_FIELD[$iter]}){
			$temp = "#define PART_$preloader_alias{$PARTITION_FIELD[$iter]} \"$preloader_alias{$PARTITION_FIELD[$iter]}\" \n";
			print $partition_define_h $temp ;
		}
	}
#Uboot re-name
	print $partition_define_h "/*Uboot re-name*/\n";
	for ($iter=0; $iter< $total_rows; $iter++){
		if($lk_xmodule_alias{$PARTITION_FIELD[$iter]}&&($lk_xmodule_alias{$PARTITION_FIELD[$iter]} ne $preloader_alias{$PARTITION_FIELD[$iter]})){
			$temp = "#define PART_$lk_xmodule_alias{$PARTITION_FIELD[$iter]} \"$lk_xmodule_alias{$PARTITION_FIELD[$iter]}\" \n";
			print $partition_define_h $temp ;
		}
	}
    print $partition_define_h "\n#define PART_FLAG_NONE              0 \n";   
    print $partition_define_h "#define PART_FLAG_LEFT             0x1 \n";  
    print $partition_define_h "#define PART_FLAG_END              0x2 \n";  
    print $partition_define_h "#define PART_MAGIC              0x58881688 \n\n";  
    for ($iter=0; $iter< $total_rows; $iter++)
    {
        if($PARTITION_FIELD[$iter] eq "BMTPOOL")
        {
			my $bmtpool=sprintf("%x",$SIZE_FIELD_KB[$iter]/64/$Page_Size);
			$temp = "#define PART_SIZE_$PARTITION_FIELD[$iter]\t\t\t(0x$bmtpool)\n" ;
    		print $partition_define_h $temp ;
        }else
        {
    		$temp = "#define PART_SIZE_$PARTITION_FIELD[$iter]\t\t\t($SIZE_FIELD_KB[$iter]*KB)\n" ;
			print $partition_define_h $temp ;
        }
	if($PARTITION_FIELD[$iter] eq "SECCFG" || $PARTITION_FIELD[$iter] eq "SEC_RO"){
		$temp = "#define PART_OFFSET_$PARTITION_FIELD[$iter]\t\t\t($START_FIELD_Byte_HEX[$iter])\n"; 
		print $partition_define_h $temp ;
	}
        
    }
    
    print $partition_define_h "\n\n#define PART_NUM\t\t\t$total_rows\n\n";
    print $partition_define_h "\n\n#define PART_MAX_COUNT\t\t\t 40\n\n";
	print $partition_define_h "#define MBR_START_ADDRESS_BYTE\t\t\t($MBR_Start_Address_KB*KB)\n\n";
	if($EMMC_SUPPORT eq "yes"){
		print $partition_define_h "#define WRITE_SIZE_Byte		512\n";
	}else{
		print $partition_define_h "#define WRITE_SIZE_Byte		($Page_Size*1024)\n";
	}
	my $ExcelStruct = <<"__TEMPLATE";
typedef enum  {
	EMMC = 1,
	NAND = 2,
} dev_type;

typedef enum {
	USER = 0,
	BOOT_1,
	BOOT_2,
	RPMB,
	GP_1,
	GP_2,
	GP_3,
	GP_4,
} Region;


struct excel_info{
	char * name;
	unsigned long long size;
	unsigned long long start_address;
	dev_type type ;
	unsigned int partition_idx;
	Region region;
};
#ifdef  MTK_EMMC_SUPPORT
/*MBR or EBR struct*/
#define SLOT_PER_MBR 4
#define MBR_COUNT 8

struct MBR_EBR_struct{
	char part_name[8];
	int part_index[SLOT_PER_MBR];
};

extern struct MBR_EBR_struct MBR_EBR_px[MBR_COUNT];
#endif
__TEMPLATE

	print $partition_define_h $ExcelStruct;
	print $partition_define_h "extern struct excel_info *PartInfo;\n";
	print $partition_define_h "\n\n#endif\n" ; 
   	close $partition_define_h ;

my $partition_define_c = &open_for_rw($PARTITION_DEFINE_C_NAME);
	print $partition_define_c &copyright_file_header_for_c();
	print $partition_define_c "#include \"partition_define.h\"\n";
	print $partition_define_c "static const struct excel_info PartInfo_Private[PART_NUM]={";
	
	for ($iter=0; $iter<$total_rows; $iter++)
    {
    	$t = lc($PARTITION_FIELD[$iter]);
		$temp = "\t\t\t{\"$t\",";
		$t = ($SIZE_FIELD_KB[$iter])*1024;
		$temp .= "$t,$START_FIELD_Byte_HEX[$iter]";
		
		if($EMMC_SUPPORT eq "yes"){
			$temp .= ", EMMC, $PARTITION_IDX_FIELD[$iter],$REGION_FIELD[$iter]";
		}else{
			$temp .= ", NAND";	
		}
		$temp .= "},\n";
		print $partition_define_c $temp;
	}
	print $partition_define_c " };\n"; 
#generate MBR struct
	print $partition_define_c "\n#ifdef  MTK_EMMC_SUPPORT\n";
	print $partition_define_c "struct MBR_EBR_struct MBR_EBR_px[MBR_COUNT]={\n";
	my $iter_p=0;
	my $iter_c = @BR_INDEX;
	print "BR COUNT is $iter_c $BR_INDEX[$iter_c-1]\n";
	for($iter_p=0;$iter_p<=$BR_INDEX[$iter_c-1];$iter_p++){
		if($iter_p ==0){
			print $partition_define_c "\t{\"mbr\", {";
		}else{
			print $partition_define_c "\t{\"ebr$iter_p\", {";
		}
		for ($iter=1; $iter<$iter_c; $iter++){
			if($iter == 1){
				$BR_INDEX[$iter] = 0;			
			}
			if($iter_p == $BR_INDEX[$iter]){
				print $partition_define_c "$iter, ";
			}		
		}
		print $partition_define_c "}},\n";			
	}
	print $partition_define_c "};\n\n";
	print $partition_define_c "EXPORT_SYMBOL(MBR_EBR_px);\n";
	print $partition_define_c "#endif\n\n";
   	close $partition_define_c ;
}
#****************************************************************************
# subroutine:  GenScatFile
# return:      
#****************************************************************************
sub GenScatFile ()
{
    my $iter = 0 ;
	`chmod 777 $SCAT_NAME_DIR` if (-e $SCAT_NAME_DIR);
    open (SCAT_NAME, ">$SCAT_NAME") or &error_handler("Ptgen open $SCAT_NAME Fail!", __FILE__, __LINE__) ;

    for ($iter=0; $iter<$total_rows; $iter++)
    {   
		my $temp;
        if ($DL_FIELD[$iter] == 0)
        {
            $temp .= "__NODL_" ;
        }
	if($EMMC_SUPPORT eq "yes" && $RESERVED_FIELD[$iter] == 1 && $PLATFORM eq "MT6582"){
			$temp .= "RSV_";
	}
	$temp .= "$PARTITION_FIELD[$iter]" ;

	if($MTK_SHARED_SDCARD eq "yes" && $PARTITION_FIELD[$iter] =~ /USRDATA/){
		$PMT_END_NAME = "$temp";
	}elsif($PARTITION_FIELD[$iter] =~ /FAT/){
		$PMT_END_NAME = "$temp";	
	}

	$temp .= " $START_FIELD_Byte_HEX[$iter]\n{\n}\n";


        print SCAT_NAME $temp ;
    }

    print SCAT_NAME "\n\n" ;
    close SCAT_NAME ;
}
sub GenYAMLScatFile(){
	my $iter = 0 ;
	if(!-e $SCAT_NAME_DIR)
	{
		`mkdir -p $SCAT_NAME_DIR `;
	}
	if(-e $SCAT_NAME)
	{
		`chmod 777 $SCAT_NAME`;
	}
    open (SCAT_NAME, ">$SCAT_NAME") or &error_handler("Ptgen open $SCAT_NAME Fail!", __FILE__, __LINE__) ;
	my %fileHash=(
		PRELOADER=>"preloader_$PROJECT.bin",
		DSP_BL=>"DSP_BL",
		SRAM_PRELD=>"sram_preloader_$PROJECT.bin",
		MEM_PRELD=>"mem_preloader_$PROJECT.bin",
		UBOOT=>"lk.bin",
		BOOTIMG=>"boot.img",
		TEE1=>"mobicore_$platform.bin",
		TEE2=>"mobicore_$platform.bin",
		RECOVERY=>"recovery.img",
		SEC_RO=>"secro.img",
		LOGO=>"logo.bin",
		CUSTOM=>"custom.img",
		ANDROID=>"system.img",
		CACHE=>"cache.img",
		USRDATA=>"userdata.img"
		);
	if ($MTK_IH_TEE_SUPPORT eq "yes")
	{
		$fileHash{TEE1}="tz.img";
		$fileHash{TEE2}="tz.img";
	}
	my %sepcial_operation_type=(
		PRELOADER=>"BOOTLOADERS",
		DSP_BL=>"BOOTLOADERS",
		NVRAM=>"BINREGION",
		PRO_INFO=>"PROTECTED",
		PROTECT_F=>"PROTECTED",
		PROTECT_S=>"PROTECTED",
		KB=>"PROTECTED",
		DKB=>"PROTECTED",
		OTP=>"RESERVED",
		PMT=>"RESERVED",
		BMTPOOL=>"RESERVED",
	);
	my %protect=(PRO_INFO=>"TRUE",NVRAM=>"TRUE",PROTECT_F=>"TRUE",PROTECT_S=>"TRUE",FAT=>"KEEPVISIBLE",FAT=>"INVISIBLE",BMTPOOL=>"INVISIBLE");
	my %Scatter_Info={};
	for ($iter=0; $iter<$total_rows; $iter++){
		$Scatter_Info{$PARTITION_FIELD[$iter]}={partition_index=>$iter,physical_start_addr=>sprintf("0x%x",$START_ADDR_PHY_Byte_DEC[$iter]),linear_start_addr=>$START_FIELD_Byte_HEX[$iter],partition_size=>sprintf("0x%x",${SIZE_FIELD_KB[$iter]}*1024)};

		if(exists $fileHash{$PARTITION_FIELD[$iter]}){
			$Scatter_Info{$PARTITION_FIELD[$iter]}{file_name}=$fileHash{$PARTITION_FIELD[$iter]};
		}else{
			$Scatter_Info{$PARTITION_FIELD[$iter]}{file_name}="NONE";
		}
		
		if($PARTITION_FIELD[$iter]=~/MBR/ || $PARTITION_FIELD[$iter]=~/EBR/){
			$Scatter_Info{$PARTITION_FIELD[$iter]}{file_name}=$PARTITION_FIELD[$iter];
		}
		
		if($DL_FIELD[$iter] == 0){
			$Scatter_Info{$PARTITION_FIELD[$iter]}{type}="NONE";
		}else{
			if($EMMC_SUPPORT eq "yes"){
			  if($TYPE_FIELD[$iter] eq "Raw data"){
			    $Scatter_Info{$PARTITION_FIELD[$iter]}{type}="NORMAL_ROM";
			  }else{			    
			    $Scatter_Info{$PARTITION_FIELD[$iter]}{type}="YAFFS_IMG";
			  }
			}else{			
			  if($TYPE_FIELD[$iter] eq "Raw data"){
  				$Scatter_Info{$PARTITION_FIELD[$iter]}{type}="NORMAL_ROM";
	  			}else{
		  			if($MTK_NAND_UBIFS_SUPPORT eq "yes"){
			  			$Scatter_Info{$PARTITION_FIELD[$iter]}{type}="UBI_IMG";
  					}else{
				 	  $Scatter_Info{$PARTITION_FIELD[$iter]}{type}="YAFFS_IMG";
				  }
			  }
			}
		}
		if($PARTITION_FIELD[$iter]=~/MBR/ || $PARTITION_FIELD[$iter]=~/EBR/){
			#$Scatter_Info{$PARTITION_FIELD[$iter]}{type}="MBR_BIN";
		}
		if($PARTITION_FIELD[$iter]=~/PRELOADER/ || $PARTITION_FIELD[$iter]=~/DSP_BL/)
		{
			$Scatter_Info{$PARTITION_FIELD[$iter]}{type}="SV5_BL_BIN";
		}
		if(exists $sepcial_operation_type{$PARTITION_FIELD[$iter]}){
			$Scatter_Info{$PARTITION_FIELD[$iter]}{operation_type}=$sepcial_operation_type{$PARTITION_FIELD[$iter]};
		}else{
			if($DL_FIELD[$iter] == 0){
				$Scatter_Info{$PARTITION_FIELD[$iter]}{operation_type}="INVISIBLE";
			}else{
				$Scatter_Info{$PARTITION_FIELD[$iter]}{operation_type}="UPDATE";
			}	
		}
		if($EMMC_SUPPORT eq "yes"){
			$Scatter_Info{$PARTITION_FIELD[$iter]}{region}="EMMC_$REGION_FIELD[$iter]";
		}else{
			$Scatter_Info{$PARTITION_FIELD[$iter]}{region}="NONE";
		}
		
		if($EMMC_SUPPORT eq "yes"){
			$Scatter_Info{$PARTITION_FIELD[$iter]}{storage}="HW_STORAGE_EMMC";
		}else{
			$Scatter_Info{$PARTITION_FIELD[$iter]}{storage}="HW_STORAGE_NAND";
		}
		
		if($PARTITION_FIELD[$iter]=~/BMTPOOL/ || $PARTITION_FIELD[$iter]=~/OTP/){
			$Scatter_Info{$PARTITION_FIELD[$iter]}{boundary_check}="false";
		}else{
			$Scatter_Info{$PARTITION_FIELD[$iter]}{boundary_check}="true";
		}
		
		if ($DL_FIELD[$iter] == 0){
			$Scatter_Info{$PARTITION_FIELD[$iter]}{is_download}="false";
		}else{
			$Scatter_Info{$PARTITION_FIELD[$iter]}{is_download}="true";
		}
	
		if($PARTITION_FIELD[$iter]=~/BMTPOOL/ || $PARTITION_FIELD[$iter]=~/OTP/){
			$Scatter_Info{$PARTITION_FIELD[$iter]}{is_reserved}="true";
		}else{
			$Scatter_Info{$PARTITION_FIELD[$iter]}{is_reserved}="false";
		}

		if($MTK_SHARED_SDCARD eq "yes" && $PARTITION_FIELD[$iter] =~ /USRDATA/){
			$PMT_END_NAME = $PARTITION_FIELD[$iter];
		}elsif($PARTITION_FIELD[$iter] =~ /FAT/){
			$PMT_END_NAME = $PARTITION_FIELD[$iter];	
		}
	}
my $Head1 = <<"__TEMPLATE";
############################################################################################################
#
#  General Setting 
#    
############################################################################################################
__TEMPLATE

my $Head2=<<"__TEMPLATE";
############################################################################################################
#
#  Layout Setting
#
############################################################################################################
__TEMPLATE

	my ${FirstDashes}="- ";
	my ${FirstSpaceSymbol}="  ";
	my ${SecondSpaceSymbol}="      ";
	my ${SecondDashes}="    - ";
	my ${colon}=": ";
	print SCAT_NAME $Head1;
	print SCAT_NAME "${FirstDashes}general${colon}MTK_PLATFORM_CFG\n";
	print SCAT_NAME "${FirstSpaceSymbol}info${colon}\n";
	print SCAT_NAME "${SecondDashes}config_version${colon}V1.1.1\n";
	print SCAT_NAME "${SecondSpaceSymbol}platform${colon}${PLATFORM}\n";
	print SCAT_NAME "${SecondSpaceSymbol}project${colon}${FULL_PROJECT}\n";
	if($EMMC_SUPPORT eq "yes"){
		print SCAT_NAME "${SecondSpaceSymbol}storage${colon}EMMC\n";
		print SCAT_NAME "${SecondSpaceSymbol}boot_channel${colon}MSDC_0\n";
		printf SCAT_NAME ("${SecondSpaceSymbol}block_size${colon}0x%x\n",2*64*1024);
	}else{
		print SCAT_NAME "${SecondSpaceSymbol}storage${colon}NAND\n";
		print SCAT_NAME "${SecondSpaceSymbol}boot_channel${colon}NONE\n";
		printf SCAT_NAME ("${SecondSpaceSymbol}block_size${colon}0x%x\n",${Page_Size}*64*1024);
	}
	print SCAT_NAME $Head2;
	for ($iter=0; $iter<$total_rows; $iter++){
		print SCAT_NAME "${FirstDashes}partition_index${colon}SYS$Scatter_Info{$PARTITION_FIELD[$iter]}{partition_index}\n";
		print SCAT_NAME "${FirstSpaceSymbol}partition_name${colon}${PARTITION_FIELD[$iter]}\n";
		print SCAT_NAME "${FirstSpaceSymbol}file_name${colon}$Scatter_Info{$PARTITION_FIELD[$iter]}{file_name}\n";
		print SCAT_NAME "${FirstSpaceSymbol}is_download${colon}$Scatter_Info{$PARTITION_FIELD[$iter]}{is_download}\n";
		print SCAT_NAME "${FirstSpaceSymbol}type${colon}$Scatter_Info{$PARTITION_FIELD[$iter]}{type}\n";
		print SCAT_NAME "${FirstSpaceSymbol}linear_start_addr${colon}$Scatter_Info{$PARTITION_FIELD[$iter]}{linear_start_addr}\n";
		print SCAT_NAME "${FirstSpaceSymbol}physical_start_addr${colon}$Scatter_Info{$PARTITION_FIELD[$iter]}{physical_start_addr}\n";
		print SCAT_NAME "${FirstSpaceSymbol}partition_size${colon}$Scatter_Info{$PARTITION_FIELD[$iter]}{partition_size}\n";
		print SCAT_NAME "${FirstSpaceSymbol}region${colon}$Scatter_Info{$PARTITION_FIELD[$iter]}{region}\n";
		print SCAT_NAME "${FirstSpaceSymbol}storage${colon}$Scatter_Info{$PARTITION_FIELD[$iter]}{storage}\n";
		print SCAT_NAME "${FirstSpaceSymbol}boundary_check${colon}$Scatter_Info{$PARTITION_FIELD[$iter]}{boundary_check}\n";
		print SCAT_NAME "${FirstSpaceSymbol}is_reserved${colon}$Scatter_Info{$PARTITION_FIELD[$iter]}{is_reserved}\n";
		print SCAT_NAME "${FirstSpaceSymbol}operation_type${colon}$Scatter_Info{$PARTITION_FIELD[$iter]}{operation_type}\n";
		print SCAT_NAME "${FirstSpaceSymbol}reserve${colon}0x00\n\n";
	}
	close SCAT_NAME;	
}
#****************************************************************************************
# subroutine:  GenMBRFile 
# return:
#****************************************************************************************

sub GenMBRFile {
	#my $eMMC_size_block = $User_Region_Size_KB*1024/512;
	my $iter = 0;
	my $iter_p = 0;
# MBR & EBR table init
#	
#	MBR
#			P1: extend partition, include SECRO & SYS
#			P2:	CACHE
#			P3: DATA
#			P4: VFAT
#	EBR1
#			P5: SECRO
#	EBR2
#			P6: SYS
#
	my $mbr_start;
	my @start_block;
	my @size_block;
	my @ebr_start_block;
	my $ebr_count = 0;
	my $br_folder = $PRODUCT_OUT;
my @BR = (
	["$PRODUCT_OUT/MBR", [	[0x00,0x0,0x0],
					[0x00,0x0,0x0],
					[0x00,0x0,0x0],
					[0x00,0x0,0x0]]],
	
);
	$ebr_start_block[0] = 0;
    #$sheet = get_sheet($SHEET_NAME,$PartitonBook) ;
# Fill MBR & EBR table -----------------------------------------------------
	for ($iter=0; $iter<@PARTITION_FIELD; $iter++) {   
		if($PARTITION_FIELD[$iter] eq "MBR"){
			$mbr_start = $START_FIELD_Byte[$iter];
			next;
		}
		if($PARTITION_FIELD[$iter] =~ /EBR(\d)/){
			$ebr_start_block[$1] =  ($START_FIELD_Byte[$iter]-$mbr_start)/512;
			$BR[$1][0] = "$PRODUCT_OUT/"."$PARTITION_FIELD[$iter]";
			$BR[$1][1] = [[0,0,0],[0,0,0],[0,0,0],[0,0,0]];
			printf ("%s %d %x\n",$BR[$1][0], $1 ,$ebr_start_block[$1]);
			$ebr_count ++;
			next;
		}
		if($PARTITION_IDX_FIELD[$iter]>0){
			#if($PARTITION_FIELD[$iter] eq "CACHE"||$PARTITION_FIELD[$iter] eq "USRDATA"||$PARTITION_FIELD[$iter] eq "SEC_RO"||$PARTITION_FIELD[$iter] eq "ANDROID"){
			#	$start_block[$PARTITION_IDX_FIELD[$iter]] = ($START_FIELD_Byte[$iter]-$mbr_start)/512;
			#	$size_block[$PARTITION_IDX_FIELD[$iter]] =  ($SIZE_FIELD_KB[$iter]-1024)*1024/512; 
			#}else{
			        $start_block[$PARTITION_IDX_FIELD[$iter]] = ($START_FIELD_Byte[$iter]-$mbr_start)/512;
				$size_block[$PARTITION_IDX_FIELD[$iter]] =  $SIZE_FIELD_KB[$iter]*1024/512; 
			#}
			
		}
		
	}
	my $item_s = 0;
	
	for($iter =0;$iter <=$ebr_count;$iter++){
		for($iter_p=1;$iter_p<@BR_INDEX;$iter_p++){
			if($iter ==0 &&$iter_p == 1){
					$BR[$iter][1][0][0] = 0x5;
					$BR[$iter][1][0][1] = $ebr_start_block[$iter+1]-$ebr_start_block[$iter];
					$BR[$iter][1][0][2] = 0xffffffff;
					$item_s ++;
					next;
			}
			
			if($iter == $BR_INDEX[$iter_p]){
				print "mbr_$iter p_$iter_p $BR_INDEX[$iter_p] index_$item_s\n";
				$BR[$iter][1][$item_s][0] = 0x83;
				$BR[$iter][1][$item_s][1] = $start_block[$iter_p] - $ebr_start_block[$iter];
				$BR[$iter][1][$item_s][2] = $size_block[$iter_p];
				if($iter_p == (@BR_INDEX-1)){
					if($ebr_count>0){
						$BR[$iter][1][$item_s][2] = 0xffffffff-($start_block[$iter_p]-$ebr_start_block[1]);}
					else{
						$BR[$iter][1][$item_s][2] = 0xffffffff;
					}
					last;
				}
				$item_s ++;
				if($item_s == 3){
					if($iter != 0){
						$BR[$iter][1][$item_s][0] = 0x5;
						$BR[$iter][1][$item_s][1] = $ebr_start_block[$iter+1]-$ebr_start_block[1];
						$BR[$iter][1][$item_s][2] = 0xffffffff;
					}else{
						next;	
					}
				}
			}
		}
				$item_s=0;
	}
	for($iter_p=1;$iter_p<@BR_INDEX;$iter_p++){
		if($iter_p == 1){
				next;
			}
		printf ("p%d start_block %x size_block %x\n",$iter_p,$start_block[$iter_p],$size_block[$iter_p]);
	}
	for($iter =0;$iter <= $ebr_count;$iter++){
		print "\n$BR[$iter][0] ";
		for($iter_p=0;$iter_p<4;$iter_p++){
			printf("%x ",$BR[$iter][1][$iter_p][0]);
			printf("%x ",$BR[$iter][1][$iter_p][1]);
			printf("%x ",$BR[$iter][1][$iter_p][2]);
		}
	}
	print "\n";
# Generate MBR&EBR binary file -----------------------------------------------------
foreach my $sBR (@BR){
	print("Generate $sBR->[0] bin file\n");
	
	#create file
	open(FH,">$sBR->[0]")|| die "create $sBR->[0] file failed\n";
	print FH pack("C512",0x0);

	#seek to tabel
	seek(FH,446,0);

	foreach (@{$sBR->[1]}){
		#type
		seek(FH,4,1);
		print FH pack("C1",$_->[0]);
		#offset and length
		seek(FH,3,1);
		print FH pack("I1",$_->[1]);
		print FH pack("I1",$_->[2]);
	}
	
	#end label
	seek(FH,510,0);
	print FH pack("C2",0x55,0xAA);

	close(FH);
}

}
#****************************************************************************************

# subroutine:  GenPartSizeFile;
# return:      
#****************************************************************************************

sub GenPartSizeFile
{
my $part_size_fh = open_for_rw($EMMC_PART_SIZE_LOCATION);	
	print $part_size_fh "\#!/usr/local/bin/perl\n";
	print $part_size_fh &copyright_file_header_for_shell();
	print $part_size_fh "\nifeq (\$(MTK_EMMC_SUPPORT),yes)\n";
	
	my $temp;
	my $index=0;
	my %PSalias=(
		SEC_RO=>SECRO,
		ANDROID=>SYSTEM,
		USRDATA=>USERDATA,
	);


	for($index=0;$index < $total_rows;$index++){
		if($TYPE_FIELD[$index] eq "EXT4" || $TYPE_FIELD[$index] eq "FAT"){
			$temp = $SIZE_FIELD_KB[$index]*1024;
			if($PARTITION_FIELD[$index] eq "USRDATA"){
				$temp -=1024*1024;	
			}
				#print "$PSalias{$PARTITION_FIELD[$index]} $PARTITION_FIELD[$index]\n";
			if(exists($PSalias{$PARTITION_FIELD[$index]})){
				print $part_size_fh "BOARD_$PSalias{$PARTITION_FIELD[$index]}IMAGE_PARTITION_SIZE:=$temp\n" ;
			}else{
				print $part_size_fh "BOARD_$PARTITION_FIELD[$index]IMAGE_PARTITION_SIZE:=$temp\n" ;
			}
		}
	}

 	print $part_size_fh "endif \n" ;
    close $part_size_fh ;
}

#****************************************************************************
# subroutine:  error_handler
# input:       $error_msg:     error message
#****************************************************************************
sub error_handler()
{
	   my ($error_msg, $file, $line_no) = @_;
	   my $final_error_msg = "Ptgen ERROR: $error_msg at $file line $line_no\n";
	   print $final_error_msg;
	   die $final_error_msg;
}

#****************************************************************************
# subroutine:  copyright_file_header_for_c
# return:      file header -- copyright
#****************************************************************************
sub copyright_file_header_for_c()
{
    my $template = <<"__TEMPLATE";
__TEMPLATE

   return $template;
}
#****************************************************************************
# subroutine:  copyright_file_header_for_shell
# return:      file header -- copyright
#****************************************************************************
sub copyright_file_header_for_shell()
{
    my $template = <<"__TEMPLATE";
__TEMPLATE

   return $template;
}

#****************************************************************************************
# subroutine:  GenPerloaderCust_partC
# return:		Gen Cust_Part.C in Preloader
# input:       no input
#****************************************************************************************
sub GenPerloaderCust_partC{
 	my $iter = 0 ;
 	my $temp;
 	my $SOURCE=open_for_rw($PreloaderC);
	print $SOURCE &copyright_file_header_for_c();
	print $SOURCE "\n#include \"typedefs.h\"\n";
	print $SOURCE "#include \"platform.h\"\n";
	print $SOURCE "#include \"blkdev.h\"\n";
	print $SOURCE "#include \"cust_part.h\"\n";
#static part_t platform_parts[PART_MAX_COUNT];
	print $SOURCE "\nstatic part_t platform_parts[PART_MAX_COUNT] = {\n";

	for ($iter=0; $iter< $total_rows; $iter++){
		last if($PARTITION_FIELD[$iter] eq "BMTPOOL" ||$PARTITION_FIELD[$iter] eq "OTP" );
		if($preloader_alias{$PARTITION_FIELD[$iter]}){
			$temp = "\t{PART_$preloader_alias{$PARTITION_FIELD[$iter]}, 0, PART_SIZE_$PARTITION_FIELD[$iter], 0,PART_FLAG_NONE},\n";
			print $SOURCE $temp;		
		}else{
			$temp = "\t{PART_$PARTITION_FIELD[$iter], 0, PART_SIZE_$PARTITION_FIELD[$iter], 0,PART_FLAG_NONE},\n";
			print $SOURCE $temp;
		}
	}
	$temp = "\t{NULL,0,0,0,PART_FLAG_END},\n};\n\n";
	print $SOURCE $temp;

#fuction

#   print $SOURCE  "void cust_part_init(void){}\n\n";

 #  print $SOURCE  "part_t *cust_part_tbl(void)\n";
#   print $SOURCE "{\n";
 #  print $SOURCE "\t return &platform_parts[0];\n";
  # print $SOURCE "}\n";
	 my $template = <<"__TEMPLATE";
void cust_part_init(void){}

part_t *cust_part_tbl(void)
{
	 return &platform_parts[0];
}

__TEMPLATE
	print $SOURCE $template;
	close $SOURCE;
}
#****************************************************************************************
sub GenKernel_PartitionC(){
	my $iter = 0;
 	my $temp;
 	my $SOURCE=open_for_rw($KernelH);
	
	print $SOURCE &copyright_file_header_for_c();
	my $template = <<"__TEMPLATE";

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include "partition_define.h"


/*=======================================================================*/
/* NAND PARTITION Mapping                                                  */
/*=======================================================================*/
static struct mtd_partition g_pasStatic_Partition[] = {

__TEMPLATE
	print $SOURCE $template;
	for ($iter=0; $iter< $total_rows; $iter++){
		
		$temp = lc($PARTITION_FIELD[$iter]);
		last if($PARTITION_FIELD[$iter] eq "BMTPOOL" || $PARTITION_FIELD[$iter] eq "OTP");
		print $SOURCE "\t{\n";
		if($kernel_alias{$PARTITION_FIELD[$iter]}){
			print $SOURCE "\t\t.name = \"$kernel_alias{$PARTITION_FIELD[$iter]}\",\n";
		}
		else{
			print $SOURCE "\t\t.name = \"$temp\",\n";
		}
		if($iter == 0){
			print $SOURCE "\t\t.offset = 0x0,\n";
		}else{
			print $SOURCE "\t\t.offset = MTDPART_OFS_APPEND,\n";
		}
		if($PARTITION_FIELD[$iter] ne "USRDATA"){
			print $SOURCE "\t\t.size = PART_SIZE_$PARTITION_FIELD[$iter],\n";
		}else{
			print $SOURCE "\t\t.size = MTDPART_SIZ_FULL,\n";
		}
		if($PARTITION_FIELD[$iter] eq "PRELOADER" ||$PARTITION_FIELD[$iter] eq "DSP_BL" ||$PARTITION_FIELD[$iter] eq "UBOOT" || $PARTITION_FIELD[$iter] eq "SEC_RO"){
			print $SOURCE "\t\t.mask_flags  = MTD_WRITEABLE,\n";
		}
		print $SOURCE "\t},\n";
	}
	print $SOURCE "};\n";

	$template = <<"__TEMPLATE";
#define NUM_PARTITIONS ARRAY_SIZE(g_pasStatic_Partition)
extern int part_num;	// = NUM_PARTITIONS;
__TEMPLATE
	print $SOURCE $template;
	close $SOURCE;


}
#****************************************************************************************
# subroutine:  GenPmt_H
# return:      
#****************************************************************************************
sub GenPmt_H(){
	my $pmt_h_fd = open_for_rw($PMT_H_NAME);
	print $pmt_h_fd &copyright_file_header_for_c();

    my $template = <<"__TEMPLATE";

#ifndef _PMT_H
#define _PMT_H

#include "partition_define.h"

//mt6516_partition.h has defination
//mt6516_download.h define again, both is 20

#define MAX_PARTITION_NAME_LEN 64
#ifdef MTK_EMMC_SUPPORT
/*64bit*/
typedef struct
{
    unsigned char name[MAX_PARTITION_NAME_LEN];     /* partition name */
    unsigned long long size;     						/* partition size */	
    unsigned long long offset;       					/* partition start */
    unsigned long long mask_flags;       				/* partition flags */

} pt_resident;
/*32bit*/
typedef struct 
{
    unsigned char name[MAX_PARTITION_NAME_LEN];     /* partition name */
    unsigned long  size;     						/* partition size */	
    unsigned long  offset;       					/* partition start */
    unsigned long mask_flags;       				/* partition flags */

} pt_resident32;
#else

typedef struct
{
    unsigned char name[MAX_PARTITION_NAME_LEN];     /* partition name */
    unsigned long size;     						/* partition size */	
    unsigned long offset;       					/* partition start */
    unsigned long mask_flags;       				/* partition flags */

} pt_resident;
#endif


#define DM_ERR_OK 0
#define DM_ERR_NO_VALID_TABLE 9
#define DM_ERR_NO_SPACE_FOUND 10
#define ERR_NO_EXIST  1

//Sequnce number


//#define PT_LOCATION          4090      // (4096-80)
//#define MPT_LOCATION        4091            // (4096-81)
#define PT_SIG      0x50547631            //"PTv1"
#define MPT_SIG    0x4D505431           //"MPT1"
#define PT_SIG_SIZE 4
#define is_valid_mpt(buf) ((*(u32 *)(buf))==MPT_SIG)
#define is_valid_pt(buf) ((*(u32 *)(buf))==PT_SIG)
#define RETRY_TIMES 5


typedef struct _DM_PARTITION_INFO
{
    char part_name[MAX_PARTITION_NAME_LEN];             /* the name of partition */
    unsigned int start_addr;                                  /* the start address of partition */
    unsigned int part_len;                                    /* the length of partition */
    unsigned char part_visibility;                              /* part_visibility is 0: this partition is hidden and CANNOT download */
                                                        /* part_visibility is 1: this partition is visible and can download */                                            
    unsigned char dl_selected;                                  /* dl_selected is 0: this partition is NOT selected to download */
                                                        /* dl_selected is 1: this partition is selected to download */
} DM_PARTITION_INFO;

typedef struct {
    unsigned int pattern;
    unsigned int part_num;                              /* The actual number of partitions */
    DM_PARTITION_INFO part_info[PART_MAX_COUNT];
} DM_PARTITION_INFO_PACKET;

typedef struct {
	int sequencenumber:8;
	int tool_or_sd_update:8;
	int mirror_pt_dl:4;   //mirror download OK
	int mirror_pt_has_space:4;
	int pt_changed:4;
	int pt_has_space:4;
} pt_info;

#endif
    
__TEMPLATE
	print $pmt_h_fd $template;
	close $pmt_h_fd;
}

#****************************************************************************************
# subroutine:  GenLK_PartitionC
# return:      
#****************************************************************************************
sub GenLK_PartitionC(){
	my $iter = 0;
 	my $temp;
 	my $SOURCE=open_for_rw($LK_PartitionC);
	print $SOURCE &copyright_file_header_for_c();

	print $SOURCE "#include \"mt_partition.h\"\n";

	if($PLATFORM eq "MT6575"){
		$temp = lc($PLATFORM);
		print $SOURCE "\npart_t $temp"."_parts[] = {\n";
	}else{
		print $SOURCE "\npart_t partition_layout[] = {\n";
	}
	for ($iter=0; $iter< $total_rows; $iter++){
		last if($PARTITION_FIELD[$iter] eq "BMTPOOL" ||$PARTITION_FIELD[$iter] eq "OTP");
		if($lk_xmodule_alias{$PARTITION_FIELD[$iter]}){
			$temp = "\t{PART_$lk_xmodule_alias{$PARTITION_FIELD[$iter]}, PART_BLKS_$lk_xmodule_alias{$PARTITION_FIELD[$iter]}, 0, PART_FLAG_NONE},\n";
			print $SOURCE $temp;		
		}else{
			$temp = "\t{PART_$PARTITION_FIELD[$iter], PART_BLKS_$PARTITION_FIELD[$iter], PART_FLAG_NONE,0},\n";
			print $SOURCE $temp;
		}
	}
	$temp = "\t{NULL, 0, PART_FLAG_END, 0},\n};";
	print $SOURCE $temp;
	print $SOURCE "\n\nstruct part_name_map g_part_name_map[PART_MAX_COUNT] = {\n";
	for ($iter=0; $iter< $total_rows; $iter++){
		last if($PARTITION_FIELD[$iter] eq "BMTPOOL" || $PARTITION_FIELD[$iter] eq "OTP");
		if($TYPE_FIELD[$iter] eq "UBIFS/YAFFS2"){
			if($MTK_NAND_UBIFS_SUPPORT eq "yes"){
				$temp_t = "ubifs";
			}else{
				$temp_t = "yaffs2";			
			}
		}else{
			$temp_t = lc($TYPE_FIELD[$iter]);
		}
		if($lk_alias{$PARTITION_FIELD[$iter]}){
			if($lk_xmodule_alias{$PARTITION_FIELD[$iter]}){
				$temp = "\t{\"$lk_alias{$PARTITION_FIELD[$iter]}\",\tPART_$lk_xmodule_alias{$PARTITION_FIELD[$iter]},\t\"$temp_t\",\t$iter,\t$FB_ERASE_FIELD[$iter],\t$FB_DL_FIELD[$iter]},\n";
			}else{	
				$temp = "\t{\"$lk_alias{$PARTITION_FIELD[$iter]}\",\tPART_$PARTITION_FIELD[$iter],\t\"$temp_t\",\t$iter,\t$FB_ERASE_FIELD[$iter],\t$FB_DL_FIELD[$iter]},\n";
			}
			print $SOURCE $temp;		
		}else{
			$temp = lc($PARTITION_FIELD[$iter]);
			if($lk_xmodule_alias{$PARTITION_FIELD[$iter]}){
				$temp = "\t{\"$temp\",\tPART_$lk_xmodule_alias{$PARTITION_FIELD[$iter]},\t\"$temp_t\",\t$iter,\t$FB_ERASE_FIELD[$iter],\t$FB_DL_FIELD[$iter]},\n";
			}else{	
				$temp = "\t{\"$temp\",\tPART_$PARTITION_FIELD[$iter],\t\"$temp_t\",\t$iter,\t$FB_ERASE_FIELD[$iter],\t$FB_DL_FIELD[$iter]},\n";
			}
			
			print $SOURCE $temp;
		}
	}
	print $SOURCE "};\n";
	close $SOURCE;
}
#****************************************************************************************
# subroutine:  GenLK_MT_ParitionH
# return:      
#****************************************************************************************
sub GenLK_MT_PartitionH(){
	my $iter = 0;
	my $SOURCE=open_for_rw($LK_MT_PartitionH);
	print $SOURCE &copyright_file_header_for_c();

	my $template = <<"__TEMPLATE";

#ifndef __MT_PARTITION_H__
#define __MT_PARTITION_H__


#include <platform/part.h>
#include "partition_define.h"
#include <platform/mt_typedefs.h>

#define NAND_WRITE_SIZE	 2048

#define BIMG_HEADER_SZ				(0x800)
#define MKIMG_HEADER_SZ				(0x200)

#define BLK_BITS         (9)
#define BLK_SIZE         (1 << BLK_BITS)
#ifdef MTK_EMMC_SUPPORT
#define BLK_NUM(size)    ((unsigned long long)(size) / BLK_SIZE)
#else
#define BLK_NUM(size)    ((unsigned long)(size) / BLK_SIZE)
#endif
#define PART_KERNEL     "KERNEL"
#define PART_ROOTFS     "ROOTFS"

__TEMPLATE
	print $SOURCE $template;
	for ($iter=0; $iter< $total_rows; $iter++){
		last if($PARTITION_FIELD[$iter] eq "BMTPOOL" || $PARTITION_FIELD[$iter] eq "OTP");
		if($lk_xmodule_alias{$PARTITION_FIELD[$iter]}){
			$temp = "#define PART_BLKS_$lk_xmodule_alias{$PARTITION_FIELD[$iter]}   BLK_NUM(PART_SIZE_$PARTITION_FIELD[$iter])\n";
			print $SOURCE $temp;		
		}else{
			$temp = "#define PART_BLKS_$PARTITION_FIELD[$iter]   BLK_NUM(PART_SIZE_$PARTITION_FIELD[$iter])\n";
			print $SOURCE $temp;
		}
	}
	print $SOURCE "\n\n#define PMT_END_NAME \"$PMT_END_NAME\"";
	print $SOURCE "\n\nstruct NAND"."_CMD\{\n";
	
	$template = <<"__TEMPLATE";
	u32	u4ColAddr;
	u32 u4RowAddr;
	u32 u4OOBRowAddr;
	u8	au1OOB[64];
	u8*	pDataBuf;
};

typedef union {
    struct {    
        unsigned int magic;        /* partition magic */
        unsigned int dsize;        /* partition data size */
        char         name[32];     /* partition name */
	unsigned int maddr;        /* partition memory address */
    } info;
    unsigned char data[BLK_SIZE];
} part_hdr_t;

typedef struct {
    char *name;        /* partition name */
    unsigned long  blknum;      /* partition blks */
    unsigned long  flags;       /* partition flags */
    unsigned long  startblk;    /* partition start blk */
} part_t;

struct part_name_map{
	char fb_name[32]; 	/*partition name used by fastboot*/	
	char r_name[32];  	/*real partition name*/
	char *partition_type;	/*partition_type*/
	int partition_idx;	/*partition index*/
	int is_support_erase;	/*partition support erase in fastboot*/
	int is_support_dl;	/*partition support download in fastboot*/
};

typedef struct part_dev part_dev_t;

struct part_dev {
    int init;
    int id;
    block_dev_desc_t *blkdev;
    int (*init_dev) (int id);
#ifdef MTK_EMMC_SUPPORT
	int (*read)  (part_dev_t *dev, u64 src, uchar *dst, int size);
    int (*write) (part_dev_t *dev, uchar *src, u64 dst, int size);
#else
    int (*read)  (part_dev_t *dev, ulong src, uchar *dst, int size);
    int (*write) (part_dev_t *dev, uchar *src, ulong dst, int size);
#endif
};
enum{
	RAW_DATA_IMG,
	YFFS2_IMG,
	UBIFS_IMG,
	EXT4_IMG,	
	FAT_IMG,
	UNKOWN_IMG,
};
extern struct part_name_map g_part_name_map[];
extern int mt_part_register_device(part_dev_t *dev);
extern part_t* mt_part_get_partition(char *name);
extern part_dev_t* mt_part_get_device(void);
extern void mt_part_init(unsigned long totalblks);
extern void mt_part_dump(void);
extern int partition_get_index(const char * name);
extern u64 partition_get_offset(int index);
extern u64 partition_get_size(int index);
extern int partition_get_type(int index, char **p_type);
extern int partition_get_name(int index, char **p_name);
extern int is_support_erase(int index);
extern int is_support_flash(int index);
extern u64 emmc_write(u64 offset, void *data, u64 size);
extern u64 emmc_read(u64 offset, void *data, u64 size);
extern int emmc_erase(u64 offset, u64 size);
extern unsigned long partition_reserve_size(void);
#endif /* __MT_PARTITION_H__ */

__TEMPLATE
	print $SOURCE $template;
	close $SOURCE;
}
#****************************************************************************************
# subroutine:  get_sheet
# return:      Excel worksheet no matter it's in merge area or not, and in windows or not
# input:       Specified Excel Sheetname
#****************************************************************************************
sub get_sheet {
  my ($sheetName,$Book) = @_;
  return $Book->Worksheet($sheetName);
}


#****************************************************************************************
# subroutine:  xls_cell_value
# return:      Excel cell value no matter it's in merge area or not, and in windows or not
# input:       $Sheet:  Specified Excel Sheet
# input:       $row:    Specified row number
# input:       $col:    Specified column number
#****************************************************************************************
sub xls_cell_value {
	my ($Sheet, $row, $col,$SheetName) = @_;
	my $cell = $Sheet->get_cell($row, $col);
	if(defined $cell){
		return  $cell->Value();
  	}else{
		my $error_msg="ERROR in ptgen.pl: (row=$row,col=$col) undefine in $SheetName!\n";
		print $error_msg;
		die $error_msg;
	}
}
sub InitGlobalValue{
    if (exists $ENV{OUT_DIR})
    {
        $PRODUCT_OUT = "$ENV{OUT_DIR}/target/product/$ENV{PROJECT}";
    }
    else
    {
        $PRODUCT_OUT = "out/target/product/$ENV{PROJECT}";
    }
    $custom_out_prefix  = "$ENV{MTK_ROOT_OUT}/PTGEN";#"mediatek/custom/$ArgList{PROJECT}";
    $configs_out_prefix = "$ENV{MTK_ROOT_OUT}/PTGEN";#"mediatek/config/$ArgList{PROJECT}";
    $preloader_out_prefix = "$ENV{MTK_ROOT_OUT}/PRELOADER_OBJ";
}

sub open_for_rw
{
    my $filepath = shift @_;
    if (-e $filepath)
    {
        chmod(0777, $filepath) or &error_handler_2("chmod 0777 $filepath fail", __FILE__, __LINE__);
        if (!unlink $filepath)
        {
            &error_handler("remove $filepath fail ", __FILE__, __LINE__);
        }
    }
    else
    {
        my $dirpath = substr($filepath, 0, rindex($filepath, "/"));
        eval { mkpath($dirpath) };
        if ($@)
        {
            &error_handler_2("Can not make dir $dirpath", __FILE__, __LINE__, $@);
        }
    }
    open my $filehander, "> $filepath" or &error_handler(" Can not open $filepath for read and write", __FILE__, __LINE__);
    return $filehander;
}
sub open_for_read
{
    my $filepath = shift @_;
    if (-e $filepath)
    {
        chmod(0777, $filepath) or &error_handler_2("chmod 777 $filepath fail", __FILE__, __LINE__);
    }
    else
    {
        print "No such file : $filepath\n";
        return undef;
    }
    open my $filehander, "< $filepath" or &error_handler_2(" Can not open $filepath for read", __FILE__, __LINE__);
    return $filehander;
}
sub error_handler_2
{
    my ($error_msg, $file, $line_no, $sys_msg) = @_;
    if (!$sys_msg)
    {
        $sys_msg = $!;
    }
    print "Fatal error: $error_msg <file: $file,line: $line_no> : $sys_msg";
    die;
}
#delete some obsolete file
sub clear_files
{
	my @ObsoleteFile;
    opendir (DIR,"mediatek/custom/$ENV{PROJECT}/common");
	push @ObsoleteFile,readdir(DIR);
    close DIR;
    push @ObsoleteFile,"mediatek/config/$ENV{PROJECT}/configs/EMMC_partition_size.mk";
	foreach my $filepath (@ObsoleteFile){
   		if (-e $filepath && !-d $filepath){
   	    	if (!unlink $filepath)
        	{
            	&error_handler("remove $filepath fail ", __FILE__, __LINE__);
        	}else{
   				print "clean $filepath: clean done \n"
   			}
  		}
	}
}
