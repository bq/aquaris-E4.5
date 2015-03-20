#
# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# This is a build configuration for the product aspects that
# are specific to the emulator.

PRODUCT_PROPERTY_OVERRIDES := \
    ro.ril.hsxpa=1 \
    ro.ril.gprsclass=10 \
    ro.adb.qemud=1

PRODUCT_COPY_FILES := \
    device/generic/goldfish/data/etc/apns-conf.xml:system/etc/apns-conf.xml \
    device/generic/goldfish/data/etc/vold.conf:system/etc/vold.conf \
    $(call add-to-product-copy-files-if-exists,development/tools/emulator/system/camera/media_profiles.xml:system/etc/media_profiles.xml) \
    $(call add-to-product-copy-files-if-exists,development/tools/emulator/system/camera/media_codecs.xml:system/etc/media_codecs.xml) \
    hardware/libhardware_legacy/audio/audio_policy.conf:system/etc/audio_policy.conf

PRODUCT_PACKAGES := \
    audio.primary.goldfish \
    power.goldfish
