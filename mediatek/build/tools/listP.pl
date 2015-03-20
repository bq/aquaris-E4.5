#!/usr/bin/perl
$pmPath = "mediatek/config";
chdir($pmPath);
@files = <*>;
$i = 0;
foreach $f (@files) {
    next if (! -e $f."/ProjectConfig.mk");
    next if ($f =~ /^common|^mt\d+$/);
    $f =~ /(.*)/;
    next if($1 eq "ckt82_we_kk");
    print $1."\n";
#    $i++;
#    if ($i%3 == 0) {
#        print "\n";
#    } else {
#        print " " x (26-length $f);
#    }
}
#if ($i%3 != 0) {
#    print "\n";
#}

exit 0;
