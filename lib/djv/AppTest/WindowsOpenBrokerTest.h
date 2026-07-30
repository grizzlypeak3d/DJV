// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

namespace djv
{
    namespace app_tests
    {
        //! Run the dependency-free Windows open-broker regression suite.
        //!
        //! Returns zero on success and a non-zero value on failure. Defining
        //! DJV_WINDOWS_OPEN_BROKER_TEST_MAIN builds this as a standalone test.
        int runWindowsOpenBrokerTests();
    }
}
