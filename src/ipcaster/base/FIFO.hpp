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

#include <condition_variable>
#include <mutex>

#include <boost/lockfree/spsc_queue.hpp>

namespace ipcaster {

/**
 * Waitable single producer / single consumer FIFO 
 * 
 * Allows the produder thread to wait when FIFO is full.
 * Allows the consumer thread to wait when FIFO is empty.
 * 
 * @todo Implement lock-free push / pop when the FIFO is neither full not empty
 */
template <typename T>
class FIFO
{
public:
    FIFO(size_t capacity) 
        :   queue_(capacity), 
            capacity_(capacity), 
            producer_is_waiting_(false), 
            consumer_is_waiting_(false),
            unblock_producer_(false),
            unblock_consumer_(false)
    {

    }

    /** 
     * Try to push one element into the FIFO
     *
     * @pre Only one thread (producer) is allowed to push data to the FIFO
     * @returns false is the FIFO is full, true otherwise.
     *
     * @note Thread-safe
     */
    bool tryPush(const T& element)
    {
        // If the queue is not full
        if(queue_.push(element)) {

            std::lock_guard<std::mutex> lock_empty(mutex_empty_);

            if(consumer_is_waiting_) {
                consumer_is_waiting_ = false;
                condition_empty_.notify_one(); // awake the consumer
            }

            return true;
        }

        return false;
    }

    /** 
     * Push one element into the FIFO 
     * 
     * blocks if the FIFO is full
     *
     * @pre Only one thread(producer) is allowed to push data to the FIFO
     * @param [in] element Reference to the element to be pushed.
     *
     * @note Thread-safe
     * The calling thread can be unblocked invoking unblockConsumer() from
     * another thread.
     */
    void push(const T& element)
    {
        // If the queue is full
        if(!queue_.push(element)) {

            std::unique_lock<std::mutex> lock_full(mutex_full_);

            while(!queue_.push(element) && !unblock_producer_) {
                producer_is_waiting_ = true;
                // Wait until pop is done by the consumer
                condition_full_.wait(lock_full);
            }
        }

        std::lock_guard<std::mutex> lock_empty(mutex_empty_);

        if(consumer_is_waiting_) {
            consumer_is_waiting_ = false;
            condition_empty_.notify_one(); // awake the consumer
        }
    }

    /** 
     * Get a reference to element in the front of the FIFO
     *
     * Availability of front element can be checked using readAvailable().
     *
     * @pre only a consumer thread is allowed to check front element
     * @pre readAvailable() > 0. If FIFO is empty, it's undefined behaviour to invoke this method.
     * @returns reference to the first element in the queue
     *
     * @note Thread-safe and wait-free
     */
    T& front()
    {
        return queue_.front();
    }

    /** 
     * Pops the front object from the FIFO
     *
     * @pre only one thread(consumer) is allowed to pop data from the FIFO
     *
     * @note Thread-safe
     */
    void pop()
    {
        queue_.pop();

        std::lock_guard<std::mutex> lock_full(mutex_full_);

        if(producer_is_waiting_) {
            producer_is_waiting_ = false;
            condition_full_.notify_one(); // awake the producer
        }
    }

    /** 
     * Get the write space to write elements
     *
     * @returns number of elements that can be pushed to the FIFO
     *
     * @note Thread-safe and wait-free, should only be called from the producer thread
     */
    size_t writeAvailable() const
    {
        return queue_.write_available();
    }

    /** 
     * Get the number of elements that are available for read
     *
     * @returns number of available elements that can be popped from the FIFO
     *
     * @note Thread-safe and wait-free, should only be called from the consumer thread
     */
    size_t readAvailable() const
    {
        return queue_.read_available();
    }

    /** 
     * Waits until there is an element ready to read.
     *
     * \return The number of elements that can be popped to the FIFO.
     *
     * \note Thread-safe, should only be called from the consumer thread.
     * The calling thread can be unblocked invoking unblockConsumer() from
     * another thread.
     */
    size_t waitReadAvailable()
    {
        size_t pop_available = queue_.read_available();
        // If the queue is empty
        if(!pop_available) { 

            std::unique_lock<std::mutex> lock_empty(mutex_empty_);

            while(!(pop_available = queue_.read_available()) && !unblock_consumer_) {
                consumer_is_waiting_ = true;
                // Wait until push is done by the producer
                condition_empty_.wait(lock_empty);
            }
        }

        return pop_available;
    }

    /** 
     * Get the total reserved capacity of the FIFO.
     *
     * @return The max total number of elements that can be stored in the FIFO.
     *
     * @note Thread-safe
     */
    size_t capacity() const
    {
        return capacity_;
    }

    /** 
     * Unblocks the producer thread if it's blocked in a push operation
     *
     * @param [in] unblock When true enable unblocking mecanism, when
     * false disable the unblocking mecanism.
     * 
     * @note Once the unblocking mecanishm is enabled the producer will never
     * blocked again until the unblocking macanism is disabled by calling this
     * function with unblock = false or by calling clear().
     *
     * @note Thread-safe
     */
    void unblockProducer(bool unblock = true) 
    {
        std::lock_guard<std::mutex> lock_full(mutex_full_);
        unblock_producer_ = unblock;
        condition_full_.notify_one(); // awake the producer
    }

    /** 
     * Unblocks the consumer thread if it's blocked in a waitReadAvailable operation
     *
     * @param [in] unblock When true enable unblocking mecanism, when
     * false disable the unblocking mecanism.
     * 
     * @note Once the unblocking mecanishm is enabled the consumer will never
     * blocked again until the unblocking macanism is disabled by calling this
     * function with unblock = false or by calling clear().
     *
     * @note Thread-safe
     */
    void unblockConsumer(bool unblock = true) 
    {
        std::unique_lock<std::mutex> lock_empty(mutex_empty_);
        unblock_consumer_ = unblock;
        condition_empty_.notify_one(); // awake the consumer
    }

    /** 
     * Empties the FIFO 
     *
     * @note Disables the unblocks mecanisms for producer and consumer
     *
     * @note Not Thread-safe
     */
    void clear()
    {
        unblock_consumer_ = false;
        unblock_producer_ = false;
        queue_.reset();
    }

private:

    // If true avoid the producer wait
    bool unblock_producer_;

    // If true avoid the consumer wait
    bool unblock_consumer_;

    // Flag
    bool producer_is_waiting_;

    // Flag
    bool consumer_is_waiting_;

    // Lockfree queue
    boost::lockfree::spsc_queue<T> queue_;

    // Mutex for condition full
    std::mutex mutex_full_;

    // To wait (consumer) if the queue is full
    std::condition_variable condition_full_;

    // Mutex for condition empty
    std::mutex mutex_empty_;

    // To wait (producer) if queue if empty 
    std::condition_variable condition_empty_;

    // Number of allocated elements
    size_t capacity_;
};

}