#!/usr/bin/perl
($#ARGV != 0) && &Usage;
$prj = $ARGV[0];
$prjmk = "mediatek/config/${prj}/ProjectConfig.mk";
$prjmk = "mediatek/config/common/ProjectConfig.mk";

print "\n";
print "# begin mediatek build properties\n";

foreach $prjmk ("mediatek/config/${prj}/ProjectConfig.mk", "mediatek/config/common/ProjectConfig.mk", "mediatek/config/${prj}/ProjectConfig_ckt.mk") {
  if (!-e $prjmk) {
  	unless ($_ eq "mediatek/config/${prj}/ProjectConfig_ckt.mk")
  	{
    die "#### Can't find $prjmk\n";
    }
  } else {
    open (FILE_HANDLE, "<$prjmk") or die "cannot open $prjmk\n";
    while (<FILE_HANDLE>) {
      if (/^(\S+)\s*=\s*(\S+)/) {
        $$1 = $2;
      }
    }
    close FILE_HANDLE;
  }
}

if (
      (! -e "ckt/$ENV{CUST_NAME}/system/etc/getsystemtype.sh")
   || ( "$ENV{CKT_VERSION_AUTO_SWITCH}" ne "yes")
   )#如果不存在此文件才产生,否则自动产生,或者CKT_VERSION_AUTO_SWITCH不等于yes 苏 勇 2013年08月05日13:49:05 
{
	print "ro.mediatek.version.release=$ENV{CKT_BUILD_VERNO}\n";
}
print "ro.mediatek.platform=$MTK_PLATFORM\n";
print "ro.mediatek.chip_ver=$MTK_CHIP_VER\n";
print "ro.mediatek.version.branch=$MTK_BRANCH\n";
print "ro.mediatek.version.sdk=$PLATFORM_MTK_SDK_VERSION\n";
print "# end mediatek build properties\n";



#// 增加对modem的hash解析 苏 勇 2013年08月26日11:06:00 
my $modemhash=&GetModeHash();
print "ro.ckt.modem.hash=$modemhash\n";

exit 0;


sub GetModeHash()
{
	my @modemfile=<mediatek/custom/common/modem/$CUSTOM_MODEM/*.img>;
	unless (scalar(@modemfile) == 1)
	{
		print "#modem文件未找到,或者找到多个 @tempfile\n";
		$modemhash="";
	}
	
	my $modemhash=`grep -awoe 'HASH_[0-9,a-z]\\{7\\}' $modemfile[0]`;
	
	unless (length($modemhash) == 13)
	{
		print "#modem Hash未找到,或者找到多个modemhash=$modemhash  grep -awoe 'HASH_[0-9,a-z]\\{7\\}' $modemfile[0]\n";
		$modemhash="";
	}
	$modemhash=~s/HASH_//;
	return $modemhash;
}
