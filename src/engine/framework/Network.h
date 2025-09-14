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
#ifndef COMMON_NETWORK_H
#define COMMON_NETWORK_H

namespace Net {

inline const std::string& OOBHeader()
{
	static const std::string header ="\xff\xff\xff\xff";
	return header;
}

template<class... Args>
void OutOfBandPrint( netsrc_t net_socket, const netadr_t& adr, Str::StringRef format, Args&&... args )
{
	std::string message = Str::Format( format, std::forward<Args>( args )... );

	// This will be read by MSG_ReadString, which expects the string length
	message.resize( message.size() + 4 );
	std::move( message.data(), message.data() + message.size() - 4, message.data() + 4 );

	uint32_t* sizeEncode = ( uint32_t* ) message.data();
	*sizeEncode = message.size() - 4;

	message = OOBHeader() + message;

	NET_SendPacket( net_socket, message.size(), message.c_str(), adr );
}

/*
 * Sends huffman-compressed data
 */
void OutOfBandData( netsrc_t sock, const netadr_t& adr, byte* data, std::size_t len );

/*
 * Converts an address to its string representation
 */
std::string AddressToString( const netadr_t& address, bool with_port = false );

/*
 * Returns port, port4 or port6 depending on the address type
 */
unsigned short GetPort(const netadr_t& address);

// --- ASYNC DNS RESOLUTION ---
// This API wraps the synchronous DNS resolution (NET_StringToAdr),
// running it in a separate thread.

// 0 can be used as an invalid value
using DNSQueryHandle = size_t;

struct DNSResult
{
	netadr_t ipv4;
	netadr_t ipv6;

	DNSResult()
	{
		ipv4.type = netadrtype_t::NA_BAD;
		ipv6.type = netadrtype_t::NA_BAD;
	}
};

DNSQueryHandle AllocDNSQuery();

// The resolver thread will start working on the query and periodically
// refresh it in the background.
// protocolMask considers NET_ENABLEV4 and NET_ENABLEV6
void SetDNSQuery(DNSQueryHandle query, std::string hostname, int protocolMask);

// type == NA_BAD if the query is still running or failed, or for a disabled protocol
DNSResult GetAddresses(DNSQueryHandle query);

// Stop the resolver thread
// You should clear any queries when you do this, so you won't get stale results
void ShutDownDNS();

} // namespace Net

#endif // COMMON_NETWORK_H
