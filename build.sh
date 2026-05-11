git submodule update --init --recursive

rm -rf Builds

cmake -B Builds -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DKFR_ENABLE_MULTIARCH=OFF \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang \
    -DZL_JUCE_FORMATS="VST3;Standalone" .
    # -DZL_JUCE_FORMATS="VST3;LV2" .

cmake --build Builds --config Release -j 24