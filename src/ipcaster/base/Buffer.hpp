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

#include <memory>

namespace ipcaster {

/**
 * Encapsules a memory buffer implemented with smart pointers so delete
 * is auto handled when no strong references are holded.
 * 
 * @param Allocator User defined allocator, defaults to std::allocator
 */
template<class Allocator = std::allocator<uint8_t>>
class BufferBase : public std::enable_shared_from_this<BufferBase<Allocator>>
{
public:

    /** Constructor
     * 
     * Alloc the buffer with a user allocator
     * 
     * @param capacity bytes allocated
     * 
     * @param allocator shared pointer to the user allocator
     */
    BufferBase(size_t capacity, std::shared_ptr<Allocator> allocator) :
        size_(0),
        capacity_(capacity),
        allocator_(allocator)
    {
        data_ = allocator_->allocate(capacity);
        if(!data_) 
            throw std::bad_alloc();
    }

    /** Constructor
     * 
     * Alloc the buffer with standard allocator
     * 
     * @param capacity bytes allocated
     */
    BufferBase(size_t capacity) :
        size_(0),
        capacity_(capacity),
        allocator_(nullptr)
    {
        Allocator allocator;
        data_ = allocator.allocate(capacity);
        if(!data_) 
            throw std::bad_alloc();
    }

    /** Constructor
     * 
     * Creates a sub-buffer pointing to a fragment of a parent buffer already allocated
     * A strong reference to the parent buffer is holded.
     * 
     * @param data Pointer to the sub-buffer (must belong to the parent buffer space)
     * 
     * @param capacity "Allocated" size of the sub-buffer
     * 
     * @param size Size of the payload (current valid data in the buffer)
     * 
     * @param parent Shared pointer to the parent
     */
    BufferBase(void* data, size_t capacity, size_t size, std::shared_ptr<BufferBase<Allocator>> parent) :
        data_(data),
        capacity_(capacity),
        size_(size),
        parent_(parent),
        allocator_(parent->allocator_)
    {
        // TODO: assert data belongs to parent
    }

    /** Destructor
     */
    virtual ~BufferBase()
    {
        if(!parent_) {
            if(allocator_) {
                allocator_->deallocate(static_cast<uint8_t*>(data_), capacity_);
            }
            else {
                Allocator allocator;
                allocator.deallocate(static_cast<uint8_t*>(data_), capacity_);
            }
        }
    }

    /** 
     * Creates a sub-buffer pointing to a fragment of a parent this buffer
     * 
     * @param data Pointer to the sub-buffer (must be memory inside this buffer space)
     * 
     * @param capacity "Allocated" size of the sub-buffer
     * 
     * @param size Size of the payload (valid data in the buffer)
     * 
     * @returns A shared pointer to the new sub-buffer
     */
    std::shared_ptr<BufferBase<Allocator>> makeChild(void* data, size_t capacity, size_t size)
    {
        auto a = std::make_shared<BufferBase<Allocator>>(data, capacity, size, this->shared_from_this());
        return a;
    }

    /** @returns A 32-bit id associated to de payload type */
    inline uint32_t payloadId() const {return payload_id_;}

    /** @returns The pointer to the payload */
    inline void* data() const {return data_;}

    /** @returns The size of the payload (valid data in the buffer) */
    inline size_t size() const { return size_;}

    /** Sets the size of the payload */
    inline void setSize(size_t size) { size_ = size;}

    /** @returns the size of the buffer's allocated space */
    inline size_t capacity() const {return capacity_;}

private:

    // A 32-bit id associated to de payload type
    uint32_t payload_id_;

    // Pointer to the buffer
    void* data_;

    // Size to the payload (valid data in the buffer)
    size_t size_;

    // Size of the allocated buffer
    size_t capacity_;

    // Pointer to the parent buffer (nullptr if not parent)
    std::shared_ptr<BufferBase<Allocator>> parent_;

    // Pointer to the allocator used to create the buffer
    std::shared_ptr<Allocator> allocator_;
};

// Buffer with std::allocator 
typedef BufferBase<> Buffer;

}