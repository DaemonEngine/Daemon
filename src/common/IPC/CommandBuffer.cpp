/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2016, Daemon Developers
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Daemon developers nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL DAEMON DEVELOPERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
===========================================================================
*/

#include "CommandBuffer.h"

namespace IPC {

    void CommandBuffer::Init(void* memory, size_t size) {
        if (size < DATA_OFFSET) {
            Sys::Drop("Buffer is too small, size is %i", size);
        }

        size_ = size - DATA_OFFSET;
        base_ = reinterpret_cast<char*>(memory);
        writerOffset_ = SAFETY_OFFSET;
        readerOffset_ = 0;
    }

    void CommandBuffer::Reset() {
        // The writer starts at the safety offset to have the invariant at init.
        reinterpret_cast<SharedWriterData*>(base_ + WRITER_OFFSET)->offset.store(SAFETY_OFFSET);
        reinterpret_cast<SharedReaderData*>(base_ + READER_OFFSET)->offset.store(0);
    }

    void CommandBuffer::LoadWriterData() {
        writerOffset_ = reinterpret_cast<SharedWriterData*>(base_ + WRITER_OFFSET)->offset.load(std::memory_order_acquire);
        if (writerOffset_ >= size_) {
            Sys::Drop("Invalid writerOffset %i for size %i", writerOffset_, size_);
        }
    }

    void CommandBuffer::LoadReaderData() {
        readerOffset_ = reinterpret_cast<SharedReaderData*>(base_ + READER_OFFSET)->offset.load(std::memory_order_acquire);
        if (readerOffset_ >= size_) {
            Sys::Drop("Invalid readerOffset %i for size %i", readerOffset_, size_);
        }
    }

    size_t CommandBuffer::GetMaxReadLength() const {
        return FromRead(writerOffset_) - SAFETY_OFFSET;
    }

    size_t CommandBuffer::GetMaxWriteLength() const {
        return FromWrite(readerOffset_) - SAFETY_OFFSET;
    }

    bool CommandBuffer::CanRead(size_t len) const {
        return FromRead(writerOffset_) - SAFETY_OFFSET >= len;
    }

    bool CommandBuffer::CanWrite(size_t len) const {
        return FromWrite(readerOffset_) - SAFETY_OFFSET >= len;
    }

    void CommandBuffer::Read(char* out, size_t len, size_t offset) {
        // Add an additional SAFETY_OFFSET to catch what the write pointer has written.
        InternalRead(readerOffset_ + offset + SAFETY_OFFSET, out, len);
    }

    void CommandBuffer::Write(const char* in, size_t len, size_t offset) {
        InternalWrite(writerOffset_ + offset, in, len);
    }

    void CommandBuffer::AdvanceReadPointer(size_t offset) {
        // TODO assert that offset is < size
        // Realign the offset to be a multiple of 4
        offset = (offset + 3) & ~3;
        readerOffset_ = Normalize(readerOffset_ + offset);
        reinterpret_cast<SharedReaderData*>(base_ + READER_OFFSET)->offset.store(readerOffset_, std::memory_order_release);
    }

    void CommandBuffer::AdvanceWritePointer(size_t offset) {
        // TODO assert that offset is < size
        // Realign the offset to be a multiple of 4
        offset = (offset + 3) & ~3;
        writerOffset_ = Normalize(writerOffset_ + offset);
        reinterpret_cast<SharedWriterData*>(base_ + WRITER_OFFSET)->offset.store(writerOffset_, std::memory_order_release);
    }

    size_t CommandBuffer::GetSize() const {
        return size_;
    }

    size_t CommandBuffer::Normalize(size_t offset) const {
        if (offset >= size_) {
            offset -= size_;
        }
        if (offset >= size_) {
            offset -= size_;
        }
        //TODO assert < size
        return offset;
    }

    size_t CommandBuffer::FromRead(size_t offset) const {
        if (offset < readerOffset_) {
            offset += size_;
        }
        return offset - readerOffset_;
    }

    size_t CommandBuffer::FromWrite(size_t offset) const {
        if (offset < writerOffset_) {
            offset += size_;
        }
        return offset - writerOffset_;
    }

    void CommandBuffer::InternalRead(size_t offset, char* out, size_t len) {
        ASSERT(out); // must not be null when using memcpy
        char* data = base_ + DATA_OFFSET;
        offset = Normalize(offset);
        size_t canRead = size_ - offset;
        if (len >= canRead) {
            memcpy(out, data + offset, canRead);
            len -= canRead;
            out += canRead;
            memcpy(out, data, len);
        } else {
            memcpy(out, data + offset, len);
        }
    }

    void CommandBuffer::InternalWrite(size_t offset, const char* in, size_t len) {
        ASSERT(in); // must not be null when using memcpy
        char* data = base_ + DATA_OFFSET;
        offset = Normalize(offset);
        size_t canWrite = size_ - offset;
        if (len >= canWrite) {
            memcpy(data + offset, in, canWrite);
            len -= canWrite;
            in += canWrite;
            memcpy(data, in, len);
        } else {
            memcpy(data + offset, in, len);
        }
    }

} // namespace IPC
