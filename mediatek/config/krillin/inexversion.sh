#!/system/bin/sh

# Create CKT specified 'internal' and 'external' version strings from
# the available ubuntu data, and seed them in specified android
# properties
#
# String format:
#
# external version: project_oem_Llc_svvv_yymmdd
#
# where:
#  project = KRILIN01A-S15A
#  oem = BQ
#  l = number of languages in the image
#  c = default language on boot
#  s = 'software stream' 1 - test versions 2 - production versions
#  vvv = channel build number
#  yymmdd = date of build
#
# the internal version appends a HHMMSS build time to the external
# version

external_version_prop="ro.build.display.id"
internal_version_prop="internal.version"

production_channel="ubuntu-touch/stable/bq-aquaris.en"

external_version=""
internal_version=""

projectandoem="KRILIN01A-S15A_BQ"
current_langs="100"

# fake a build time for the internal version. first build of the day
# is at 000100, second is one minute later, etc
convert_buildno_to_time() {
    # Ubuntu build numbers are undecorated dates, or append .1, .2 etc
    # for second and subsequent builds that day. Make sure there is
    # always a build number to slice off.
    ubuntu_build=$(echo $(getprop persist.ubuntu.version.rootfs).0 | cut -d. -f2)
    printf "00%02d00\n" $((ubuntu_build + 1))
}

update_version() {
    channels=$(getprop persist.ubuntu.version.channel)
    build=$(getprop persist.ubuntu.version)
    stream="1"
    if [[ $channels == *$production_channel* ]];then
        stream="2"
    fi
    version=$(printf "%d%03d\n" $stream $build)

    short_date=$(getprop persist.ubuntu.version.rootfs | cut -c3-8)
    fake_build_time=$(convert_buildno_to_time)

    default_language=$(getprop persist.ubuntu.default_language)
    # if there is no default set in the custom tarball, assume EN
    if [[ $default_language == "unknown" ]];then
        default_language="EN"
    fi

    external_version=$projectandoem"_L"$current_langs$default_language"_"$version"_"$short_date
    internal_version=$external_version$fake_build_time

    setprop $external_version_prop $external_version
    setprop $internal_version_prop $internal_version
}

for i in 1 2 3 4 5; do
    sleep 1
    update_version
    ext_ver=$(getprop $external_version_prop)
    int_ver=$(getprop $internal_version_prop)

    if [ "$ext_ver" == $external_version ] && [ "$int_ver" == $internal_version ];then
        exit
    fi
done
