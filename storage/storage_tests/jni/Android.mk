# @todo(vbykoianko)
# Probably this file shell be generated with a script based on .pro.
# If you do this take into acount:
# - the order of libs is important. It solves some linking problems;
# - there are additional libs here (android_native_app_glue and zlib);

LOCAL_PATH := $(call my-dir)
ROOT_PATH := ../..
include ../../android/UnitTests/jni/AndroidBeginning.mk

LOCAL_MODULE := storage_tests

LOCAL_STATIC_LIBRARIES := succinct android_native_app_glue platform_tests_support storage indexer platform geometry coding base protobuf tomcrypt opening_hours minizip zlib jansson

include ../../android/UnitTests/jni/AndroidEnding.mk
