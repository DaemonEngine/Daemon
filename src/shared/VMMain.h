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

#ifndef SHARED_VM_MAIN_H_
#define SHARED_VM_MAIN_H_

#include "common/IPC/Channel.h"

namespace VM {

	// Functions each specific gamelogic should implement
	void VMInit();
	void VMHandleSyscall(uint32_t id, Util::Reader reader);
	void GetNetcodeTables(NetcodeTable& playerStateTable, int& playerStateSize);
	extern int VM_API_VERSION;

    // TODO(WASM): Remove it once NaCl is removed.
	// Root channel used to communicate with the engine
	extern IPC::Channel rootChannel;

#if !defined(__wasm__)
	// Send a message to the engine
	template<typename Msg, typename... Args> void SendMsg(Args&&... args) {
		IPC::SendMsg<Msg>(rootChannel, VMHandleSyscall, std::forward<Args>(args)...);
	}

#else
    std::vector<char> SendRawMsg(const Util::Writer& message);

    namespace detail {
        // Implementations of SendMsg for Message and SyncMessage
        template<typename Id, typename... MsgArgs, typename... Args>
        void SendMsg(IPC::Message<Id, MsgArgs...>, Args&&... args) {
            using Message = IPC::Message<Id, MsgArgs...>;
            static_assert(sizeof...(Args) == std::tuple_size<typename Message::Inputs>::value, "Incorrect number of arguments for IPC::SendMsg");

            Util::Writer writer;
            writer.Write<uint32_t>(Message::id);
            writer.WriteArgs(Util::TypeListFromTuple<typename Message::Inputs>(), std::forward<Args>(args)...);
            auto reply = SendRawMsg(writer);
            ASSERT(reply.GetData().empty());
        }

        template<typename Msg, typename Reply, typename... Args>
        void SendMsg(IPC::SyncMessage<Msg, Reply>, Args&&... args) {
            using Message = IPC::SyncMessage<Msg, Reply>;
            static_assert(sizeof...(Args) == std::tuple_size<typename Message::Inputs>::value + std::tuple_size<typename Message::Outputs>::value, "Incorrect number of arguments for IPC::SendMsg");

            Util::Writer writer;
            writer.Write<uint32_t>(Message::id);
            writer.WriteArgs(Util::TypeListFromTuple<typename Message::Inputs>(), std::forward<Args>(args)...);

            SendRawMsg(writer);
            auto reply = SendRawMsg(writer);

            Util::Reader reader;
            std::swap(reader.GetData(), reply);
            auto out = std::forward_as_tuple(std::forward<Args>(args)...);
            reader.FillTuple<std::tuple_size<typename Message::Inputs>::value>(Util::TypeListFromTuple<typename Message::Outputs>(), out);
        }
    }

	template<typename Msg, typename... Args>
    void SendMsg(Args&&... args) {
        detail::SendMsg(Msg(), std::forward<Args>(args)...);
	}
#endif // !defined(__wasm__)

}

#endif // SHARED_VM_MAIN_H_
