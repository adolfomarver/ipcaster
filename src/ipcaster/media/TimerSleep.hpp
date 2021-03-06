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

#pragma once

#include <chrono>

namespace ipcaster
{

	/**
	 * Implements a waitable Timer with a fix period
	 * This implementation rely on std::this_thread::sleep_for()
	 * On Windows x64 the jitter is a bit higher about 10(ms).
	 * The accuracy of this timer is important because it will be 
	 * reflected as jitter in the TS PCR clock, it will also
	 * have impact in size of the output burst.
	 * Best methods, relative to using Windows high precission timers
	 * at Kernel level could be implemented to achive jitter near 1(ms)
	 */
	class TimerSleep
	{
	public:

		/** Constructor
		 * Launches the timer thread
		 *
		 * @param period Period of the timer in nanoseconds
		 *
		 * @throws std::exception
		 */
		TimerSleep(const std::chrono::nanoseconds& period)
			: period_(period)
		{
		}

		/**
		* Waits until timer is triggered
		*
		* @returns The current std::high_resolution_clock time.
		*
		* @note Concurrency is not supported so there should only be one thread
		* calling wait at a time.
		*/
		std::chrono::high_resolution_clock::time_point wait()
		{
			std::this_thread::sleep_for(period_);

			return std::chrono::high_resolution_clock::now();
		}

		/** @returns The period of the timer */
		inline std::chrono::nanoseconds period() { return period_; }

		/** @returns The current std::high_resolution_clock time. */
		inline std::chrono::high_resolution_clock::time_point now()
		{
			return std::chrono::high_resolution_clock::now();
		}

	private:

		std::chrono::nanoseconds period_;
	};

}