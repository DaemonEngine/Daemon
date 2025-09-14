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
#include "common/Common.h"
#include "engine/qcommon/qcommon.h"
#include "engine/qcommon/sys.h"
#include "Network.h"

#ifdef _WIN32
#       include <winsock2.h>
#else
#       include <sys/socket.h>
#       include <netinet/in.h>
#endif

extern Log::Logger netLog;

// Helper functions defined in net_ip.cpp
void Sys_SockaddrToString( char *dest, int destlen, struct sockaddr *input );
void NetadrToSockadr( const netadr_t *a, struct sockaddr *s );

namespace Net {

void OutOfBandData( netsrc_t sock, const netadr_t& adr, byte *data, std::size_t len )
{
	if ( len == 0 )
	{
		return;
	}

	byte buf[MAX_MSGLEN]; // TODO should this be shorter, like MAX_PACKET?
	size_t size = OOBHeader().size() + len;
	if (size > sizeof(buf)) {
		Log::Warn("OutOfBandData: not sending excessively large (%d) message", len);
		return;
	}
	std::copy_n(OOBHeader().begin(), OOBHeader().size(), buf);
	std::copy_n(data, len, buf + OOBHeader().size());

	msg_t mbuf{};
	mbuf.data = buf;
	mbuf.cursize = size;
	Huff_Compress( &mbuf, 16 );
	// send the datagram
	NET_SendPacket( sock, mbuf.cursize, mbuf.data, adr );
}

std::string AddressToString( const netadr_t& address, bool with_port )
{

    if ( address.type == netadrtype_t::NA_LOOPBACK )
    {
        return "loopback";
    }
    else if ( address.type == netadrtype_t::NA_BOT )
    {
        return "bot";
    }
    else if ( address.type == netadrtype_t::NA_IP ||
              address.type == netadrtype_t::NA_IP6 ||
              address.type == netadrtype_t::NA_IP_DUAL )
    {
        char s[ NET_ADDR_STR_MAX_LEN ];
        sockaddr_storage sadr = {};
        NetadrToSockadr( &address, reinterpret_cast<sockaddr*>(&sadr) );
        Sys_SockaddrToString( s, sizeof(s), reinterpret_cast<sockaddr*>(&sadr) );

        std::string result = s;
        if ( with_port )
        {
            if ( NET_IS_IPv6( address.type ) )
            {
                result = '[' + result + ']';
            }
                result += ':' + std::to_string(ntohs(GetPort(address)));
        }
        return result;
    }

    return "";
}

unsigned short GetPort(const netadr_t& address)
{
    if ( address.type == netadrtype_t::NA_IP_DUAL )
    {
        if ( NET_IS_IPv4( address.type ) )
        {
            return address.port4;
        }
        else if ( NET_IS_IPv6( address.type ) )
        {
            return address.port6;
        }
    }
    return address.port;
}

// Resend the query when it gets this old
constexpr int DNS_STALE_TIME = 3 * 60000;

// Stop using the query result at all when it's this old
constexpr int DNS_EXPIRED_TIME = 60 * 60000;

namespace {
struct DNSQuery
{
    std::string hostname;
    DNSResult result;
    int protocolMask;
    int timestamp;
};

netadr_t Resolve(Str::StringRef hostname, netadrtype_t protocol)
{
    netadr_t ip;
    switch (NET_StringToAdr(hostname.c_str(), &ip, protocol)) {
    case 0:
        break; // failure indicated to caller by NA_BAD
    case 1: // fully specified address
        break;
    case 2: // Unknown port. The GetAddresses caller should set a default on its own
        ip.port = 0;
        break;
    }
    return ip;
}

// All functions in this class besides ResolverMain are intended to be called
// by the engine main thread only.
class DNSResolver
{
private:
    std::vector<DNSQuery> queries_;
    std::thread resolverThread_;
    std::condition_variable alarm_;
    std::mutex mutex_; // Guards queries_ and halt_
    bool halt_ = false;

    void ResolverMain()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (!halt_) {
            // Find most overdue query
            DNSQueryHandle next = 0;
            // Make dummy 0th query have maximum freshness
            queries_[0].timestamp = std::numeric_limits<int>::max() - DNS_STALE_TIME;
            for (size_t i = 1; i < queries_.size(); i++) {
                if (queries_[i].protocolMask && queries_[i].timestamp < queries_[next].timestamp) {
                    next = i;
                }
            }
            int wait = (queries_[next].timestamp + DNS_STALE_TIME) - Sys::Milliseconds();
            if (wait > 0) {
                // Atomically fall asleep and unlock
                alarm_.wait_for(lock, std::chrono::milliseconds(wait));
                // locked again when wait_for returns
            } else {
                std::string hostname = queries_[next].hostname;
                int mask = queries_[next].protocolMask;
                lock.unlock();
                DNSResult result;
                bool need4 = mask & NET_ENABLEV4;
                bool need6 = mask & NET_ENABLEV6;
                // HACK: if IPv4 and IPv6 are both enabled, we call the DNS resolution stuff twice,
                // but with both protocols requested the first time, in hopes that the second call
                // will just be gotten from the OS cache.
                // TODO: this is really stupid, change the API so I can request both at once,
                //       or at least not be affected by NET_PRIOV6
                if (need4 && need6) {
                    netadr_t ip = Resolve(hostname, netadrtype_t::NA_UNSPEC);
                    // If it's NA_BAD result applies to both
                    if (need6 && ip.type != netadrtype_t::NA_IP) {
                        result.ipv6 = ip;
                        need6 = false;
                    }
                    if (need4 && ip.type != netadrtype_t::NA_IP6) {
                        result.ipv4 = ip;
                        need4 = false;
                    }
                }
                if (need4) {
                    result.ipv4 = Resolve(hostname, netadrtype_t::NA_IP);
                }
                if (need6) {
                    result.ipv6 = Resolve(hostname, netadrtype_t::NA_IP6);
                }
                if (result.ipv4.type == netadrtype_t::NA_BAD && result.ipv6.type == netadrtype_t::NA_BAD) {
                    netLog.Notice("Failed to resolve hostname %s", hostname);
                } else {
                    netLog.DoVerboseCode([&hostname, &result] {
                        if (result.ipv4.type != netadrtype_t::NA_BAD) {
                            netLog.Verbose("Resolved %s (IPv4) to %s", hostname, AddressToString(result.ipv4));
                        }
                        if (result.ipv6.type != netadrtype_t::NA_BAD) {
                            netLog.Verbose("Resolved %s (IPv6) to %s", hostname, AddressToString(result.ipv6));
                        }
                    });
                }
                lock.lock();
                if (hostname == queries_[next].hostname && mask == queries_[next].protocolMask) {
                    queries_[next].result = result;
                    queries_[next].timestamp = Sys::Milliseconds();
                }
            }
        }
    }

public:
    DNSResolver()
    {
        queries_.emplace_back(); // Placeholder since 0 is an invalid handle
    }

    DNSQueryHandle AllocQuery()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queries_.emplace_back();
        return queries_.size() - 1;
    }

    void SetQuery(DNSQueryHandle handle, std::string hostname, int protocolMask)
    {
        protocolMask &= NET_ENABLEV4 | NET_ENABLEV6;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            ASSERT(handle > 0 && handle < queries_.size());
            queries_[handle].timestamp = -DNS_EXPIRED_TIME;
            queries_[handle].hostname = std::move(hostname);
            queries_[handle].protocolMask = protocolMask;
        }
        if (resolverThread_.joinable()) {
            alarm_.notify_one();
        } else {
            // Start thread on first use
            netLog.Notice("Starting DNS resolver thread");
            resolverThread_ = std::thread(&DNSResolver::ResolverMain, this);
        }
    }

    DNSResult GetAddress(DNSQueryHandle handle)
    {
        if (handle == 0) {
            return {};
        }
        std::lock_guard<std::mutex> lock(mutex_);
        ASSERT_LT(handle, queries_.size());
        if (Sys::Milliseconds() - DNS_EXPIRED_TIME >= queries_[handle].timestamp) {
            return {};
        }
        return queries_[handle].result;
    }

    void Stop()
    {
        if (!resolverThread_.joinable()) {
            return;
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            halt_ = true;
        }
        netLog.Notice("Stopping DNS resolver thread");
        alarm_.notify_one();
        resolverThread_.join();
        halt_ = false;
    }
};
} // namespace

static DNSResolver resolver;

DNSQueryHandle AllocDNSQuery()
{
    return resolver.AllocQuery();
}

void SetDNSQuery(DNSQueryHandle query, std::string hostname, int protocolMask)
{
    resolver.SetQuery(query, hostname, protocolMask);
}

DNSResult GetAddresses(DNSQueryHandle query)
{
    return resolver.GetAddress(query);
}

void ShutDownDNS()
{
    resolver.Stop();
}

} // namespace Net
