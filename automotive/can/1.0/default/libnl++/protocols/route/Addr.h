/*
 * Copyright (C) 2022 The Android Open Source Project
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

#pragma once

#include "../MessageDefinition.h"

#include <linux/rtnetlink.h>

namespace android::nl::protocols::route {

class Addr : public MessageDefinition<ifaddrmsg> {
  public:
    Addr();
    void toStream(std::stringstream& ss, const ifaddrmsg& data) const override;
};

}  // namespace android::nl::protocols::route
