//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
// NOTE(TBB MIGRATION): root_task usage and task scheduling was reworked in accordance to official migration
//  guide: https://oneapi-src.github.io/oneTBB/main/tbb_userguide/Migration_Guide/Task_Scheduler_Init.html

#include "pxr/pxr.h"
#include "pxr/base/work/dispatcher.h"

PXR_NAMESPACE_OPEN_SCOPE

WorkDispatcher::WorkDispatcher()
    : _context(
        tbb::task_group_context::isolated,
        tbb::task_group_context::concurrent_wait |
        tbb::task_group_context::default_traits),
      _taskGroup(_context)
{
    _waitCleanupFlag.clear();
}

WorkDispatcher::~WorkDispatcher()
{
    Wait();
}

void
WorkDispatcher::Wait()
{
    // Wait for tasks to complete.
    // NOTE(TBB MIGRATION): context is being reset inside task_group::wait()
    _taskGroup.wait();

    // If we take the flag from false -> true, we do the cleanup.
    if (!_waitCleanupFlag.test_and_set()) {
        // Post all diagnostics to this thread's list.
        for (auto &et: _errors) {
            et.Post();
        }
        _errors.clear();
        _waitCleanupFlag.clear();
    }
}

void
WorkDispatcher::Cancel()
{
    _context.cancel_group_execution();
}

/* static */
void
WorkDispatcher::_TransportErrors(const TfErrorMark &mark,
                                 _ErrorTransports *errors)
{
    TfErrorTransport transport = mark.Transport();
    errors->grow_by(1)->swap(transport);
}

PXR_NAMESPACE_CLOSE_SCOPE
