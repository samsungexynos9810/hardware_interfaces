/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "GeneratedTestHarness.h"

#include <android-base/logging.h>
#include <android/hardware/neuralnetworks/1.0/IPreparedModel.h>
#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware/neuralnetworks/1.1/IDevice.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>

#include <iostream>

#include "1.0/Callbacks.h"
#include "1.0/Utils.h"
#include "MemoryUtils.h"
#include "TestHarness.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace generated_tests {

using ::android::hardware::neuralnetworks::V1_0::ErrorStatus;
using ::android::hardware::neuralnetworks::V1_0::IPreparedModel;
using ::android::hardware::neuralnetworks::V1_0::Request;
using ::android::hardware::neuralnetworks::V1_0::RequestArgument;
using ::android::hardware::neuralnetworks::V1_0::implementation::ExecutionCallback;
using ::android::hardware::neuralnetworks::V1_0::implementation::PreparedModelCallback;
using ::android::hardware::neuralnetworks::V1_1::ExecutionPreference;
using ::android::hardware::neuralnetworks::V1_1::IDevice;
using ::android::hardware::neuralnetworks::V1_1::Model;
using ::android::hidl::memory::V1_0::IMemory;
using ::test_helper::compare;
using ::test_helper::filter;
using ::test_helper::for_all;
using ::test_helper::MixedTyped;
using ::test_helper::MixedTypedExample;
using ::test_helper::resize_accordingly;

// Top level driver for models and examples generated by test_generator.py
// Test driver for those generated from ml/nn/runtime/test/spec
void EvaluatePreparedModel(sp<IPreparedModel>& preparedModel, std::function<bool(int)> is_ignored,
                           const std::vector<MixedTypedExample>& examples,
                           bool hasRelaxedFloat32Model, float fpAtol, float fpRtol) {
    const uint32_t INPUT = 0;
    const uint32_t OUTPUT = 1;

    int example_no = 1;
    for (auto& example : examples) {
        SCOPED_TRACE(example_no++);
        const MixedTyped& inputs = example.operands.first;
        const MixedTyped& golden = example.operands.second;

        const bool hasFloat16Inputs = !inputs.float16Operands.empty();
        if (hasRelaxedFloat32Model || hasFloat16Inputs) {
            // TODO: Adjust the error limit based on testing.
            // If in relaxed mode, set the absolute tolerance to be 5ULP of FP16.
            fpAtol = 5.0f * 0.0009765625f;
            // Set the relative tolerance to be 5ULP of the corresponding FP precision.
            fpRtol = 5.0f * 0.0009765625f;
        }

        std::vector<RequestArgument> inputs_info, outputs_info;
        uint32_t inputSize = 0, outputSize = 0;
        // This function only partially specifies the metadata (vector of RequestArguments).
        // The contents are copied over below.
        for_all(inputs, [&inputs_info, &inputSize](int index, auto, auto s) {
            if (inputs_info.size() <= static_cast<size_t>(index)) inputs_info.resize(index + 1);
            RequestArgument arg = {
                    .location = {.poolIndex = INPUT,
                                 .offset = 0,
                                 .length = static_cast<uint32_t>(s)},
                    .dimensions = {},
            };
            RequestArgument arg_empty = {
                    .hasNoValue = true,
            };
            inputs_info[index] = s ? arg : arg_empty;
            inputSize += s;
        });
        // Compute offset for inputs 1 and so on
        {
            size_t offset = 0;
            for (auto& i : inputs_info) {
                if (!i.hasNoValue) i.location.offset = offset;
                offset += i.location.length;
            }
        }

        MixedTyped test;  // holding test results

        // Go through all outputs, initialize RequestArgument descriptors
        resize_accordingly(golden, test);
        for_all(golden, [&outputs_info, &outputSize](int index, auto, auto s) {
            if (outputs_info.size() <= static_cast<size_t>(index)) outputs_info.resize(index + 1);
            RequestArgument arg = {
                    .location = {.poolIndex = OUTPUT,
                                 .offset = 0,
                                 .length = static_cast<uint32_t>(s)},
                    .dimensions = {},
            };
            outputs_info[index] = arg;
            outputSize += s;
        });
        // Compute offset for outputs 1 and so on
        {
            size_t offset = 0;
            for (auto& i : outputs_info) {
                i.location.offset = offset;
                offset += i.location.length;
            }
        }
        std::vector<hidl_memory> pools = {nn::allocateSharedMemory(inputSize),
                                          nn::allocateSharedMemory(outputSize)};
        ASSERT_NE(0ull, pools[INPUT].size());
        ASSERT_NE(0ull, pools[OUTPUT].size());

        // load data
        sp<IMemory> inputMemory = mapMemory(pools[INPUT]);
        sp<IMemory> outputMemory = mapMemory(pools[OUTPUT]);
        ASSERT_NE(nullptr, inputMemory.get());
        ASSERT_NE(nullptr, outputMemory.get());
        char* inputPtr = reinterpret_cast<char*>(static_cast<void*>(inputMemory->getPointer()));
        char* outputPtr = reinterpret_cast<char*>(static_cast<void*>(outputMemory->getPointer()));
        ASSERT_NE(nullptr, inputPtr);
        ASSERT_NE(nullptr, outputPtr);
        inputMemory->update();
        outputMemory->update();

        // Go through all inputs, copy the values
        for_all(inputs, [&inputs_info, inputPtr](int index, auto p, auto s) {
            char* begin = (char*)p;
            char* end = begin + s;
            // TODO: handle more than one input
            std::copy(begin, end, inputPtr + inputs_info[index].location.offset);
        });

        inputMemory->commit();
        outputMemory->commit();

        const Request request = {.inputs = inputs_info, .outputs = outputs_info, .pools = pools};

        // launch execution
        sp<ExecutionCallback> executionCallback = new ExecutionCallback();
        ASSERT_NE(nullptr, executionCallback.get());
        Return<ErrorStatus> executionLaunchStatus =
                preparedModel->execute(request, executionCallback);
        ASSERT_TRUE(executionLaunchStatus.isOk());
        EXPECT_EQ(ErrorStatus::NONE, static_cast<ErrorStatus>(executionLaunchStatus));

        // retrieve execution status
        executionCallback->wait();
        ASSERT_EQ(ErrorStatus::NONE, executionCallback->getStatus());

        // validate results
        outputMemory->read();
        copy_back(&test, outputs_info, outputPtr);
        outputMemory->commit();
        // Filter out don't cares
        MixedTyped filtered_golden = filter(golden, is_ignored);
        MixedTyped filtered_test = filter(test, is_ignored);

        // We want "close-enough" results for float
        compare(filtered_golden, filtered_test, fpAtol, fpRtol);
    }
}

void Execute(const sp<IDevice>& device, std::function<Model(void)> create_model,
             std::function<bool(int)> is_ignored, const std::vector<MixedTypedExample>& examples) {
    Model model = create_model();

    // see if service can handle model
    bool fullySupportsModel = false;
    Return<void> supportedCall = device->getSupportedOperations_1_1(
            model, [&fullySupportsModel](ErrorStatus status, const hidl_vec<bool>& supported) {
                ASSERT_EQ(ErrorStatus::NONE, status);
                ASSERT_NE(0ul, supported.size());
                fullySupportsModel = std::all_of(supported.begin(), supported.end(),
                                                 [](bool valid) { return valid; });
            });
    ASSERT_TRUE(supportedCall.isOk());

    // launch prepare model
    sp<PreparedModelCallback> preparedModelCallback = new PreparedModelCallback();
    ASSERT_NE(nullptr, preparedModelCallback.get());
    Return<ErrorStatus> prepareLaunchStatus = device->prepareModel_1_1(
            model, ExecutionPreference::FAST_SINGLE_ANSWER, preparedModelCallback);
    ASSERT_TRUE(prepareLaunchStatus.isOk());
    ASSERT_EQ(ErrorStatus::NONE, static_cast<ErrorStatus>(prepareLaunchStatus));

    // retrieve prepared model
    preparedModelCallback->wait();
    ErrorStatus prepareReturnStatus = preparedModelCallback->getStatus();
    sp<IPreparedModel> preparedModel = preparedModelCallback->getPreparedModel();

    // early termination if vendor service cannot fully prepare model
    if (!fullySupportsModel && prepareReturnStatus != ErrorStatus::NONE) {
        ASSERT_EQ(nullptr, preparedModel.get());
        LOG(INFO) << "NN VTS: Early termination of test because vendor service cannot "
                     "prepare model that it does not support.";
        std::cout << "[          ]   Early termination of test because vendor service cannot "
                     "prepare model that it does not support."
                  << std::endl;
        GTEST_SKIP();
    }
    EXPECT_EQ(ErrorStatus::NONE, prepareReturnStatus);
    ASSERT_NE(nullptr, preparedModel.get());

    EvaluatePreparedModel(preparedModel, is_ignored, examples,
                          model.relaxComputationFloat32toFloat16, 1e-5f, 1e-5f);
}

}  // namespace generated_tests
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
