//
// Copyright (C) 2019 Adofo Martinez <adolfo at ipcaster dot net>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <iostream>
#include <thread>

//#include <gtest/gtest.h>

//#include "FIFOTest.hpp"
#include "SendReceiveTest.hpp"

#ifdef _MSC_VER // Windows

#define SOURCE_TS "..\\tsfiles\\test.ts"
#define IPCASTER_EXEC "release\\ipcaster -v 3 -s " SOURCE_TS " 127.0.0.1 50000"
#define DELETEOUTPUT "del out.ts"

#else // Linux / Unix

#define SOURCE_TS "../tsfiles/test.ts"
#define IPCASTER_EXEC "./ipcaster -v 3 -s " SOURCE_TS " 127.0.0.1 50000"
#define DELETEOUTPUT "rm out.ts"

#endif

int main(int argc, char* argv[])
{
    //testing::InitGoogleTest(&argc, argv);
    //return RUN_ALL_TESTS();

    try {
        ipcaster::SendReceiveTest send_receive_test(50000, SOURCE_TS, "out.ts");

        auto future_ipcaster = std::async(std::launch::async, [&] () { 
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    return system(IPCASTER_EXEC);
                });

        send_receive_test.run();

        auto rs = system(DELETEOUTPUT);

        return future_ipcaster.get();
    }
    catch(std::exception& e) {
        std::cout << e.what() << std::endl;
        std::cout << "Tests failed !!!" << std::endl;
        return -1;
    }

    return 0;
}