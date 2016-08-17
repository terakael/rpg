#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>
#include <boost/algorithm/string.hpp>
#include <sstream>
#include <openssl/sha.h>
#include <ctime>

#define htonll(x) ((((uint64_t)htonl(x)) << 32) + htonl((x) >> 32))

using boost::asio::ip::tcp;

const std::string WS_KEY = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";// magic string for WebSockets

class TcpConnection : public boost::enable_shared_from_this<TcpConnection>
{
public:
    typedef boost::shared_ptr<TcpConnection> pointer;
    static pointer create(boost::asio::io_service& io)
    {
        std::cout << "creating TcpConnection pointer" << std::endl;
        return pointer(new TcpConnection(io));
    }

    tcp::socket& socket()
    {
        return mSocket;
    }

    void Start()
    {
        std::cout << "Starting" << std::endl;

        mSocket.async_read_some(boost::asio::buffer(mMessage, MAX_LENGTH), boost::bind(&TcpConnection::HandleRead, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

private:
    TcpConnection(boost::asio::io_service& io)
        : mSocket(io)
    {

    }

    void HandleWrite(const boost::system::error_code& error)
    {
        if (!error)
        {
            std::cout << "Message: " << mFullBuffer << std::endl;
            mFullBuffer = "";
            memset(mMessage, 0, MAX_LENGTH);
            mSocket.async_read_some(boost::asio::buffer(mMessage, MAX_LENGTH), boost::bind(&TcpConnection::HandleRead, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        }
    }

    void HandleRead(const boost::system::error_code& error, size_t bytesTransferred)
    {
        std::string buf(mMessage, MAX_LENGTH);

        bool isEof = false;
        if (buf.find("\r\n\r\n") != std::string::npos)
        {
            isEof = true;
            buf = buf.substr(0, buf.find("\r\n\r\n") + 2);
        }

        mFullBuffer += buf;


        static const std::string keyTag = "Sec-WebSocket-Key: ";
        if (mFullBuffer.find(keyTag) == -1)
        {
            // not a handshake
            int maskOffset = 2;
            int len = decodePayloadLength(mMessage, maskOffset);
            std::string data = unmaskData(mMessage, len, maskOffset);
        }

        // given the handshake request looks like this:
        /*
        GET /chat HTTP/1.1
            Host: server.example.com
            Upgrade: WebSocket
            Connection: Upgrade
            Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
            Origin: http://example.com
            Sec-WebSocket-Protocol: chat, superchat
            Sec-WebSocket-Version: 13
       */

        // The response should be output like this:
        /*
        HTTP/1.1 101 Switching Protocols
            Upgrade: WebSocket
            Connection: Upgrade
            Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo= 
            Sec-WebSocket-Protocol: chat
        */

        if (isEof)
        {
            const size_t startElement = mFullBuffer.find(keyTag) + keyTag.size();
            const size_t endElement = mFullBuffer.find("\r\n", startElement);
            std::string key = mFullBuffer.substr(startElement, endElement - startElement);
            
            std::string str = ProcessWebSocketKey(key + WS_KEY);

            std::string response;
            response += "HTTP/1.1 101 Switching Protocols\r\n";
            response += "Upgrade: websocket\r\n";
            response += "Connection: Upgrade\r\n";
            response += "Sec-WebSocket-Accept: ";
            response += str;
            response += "\r\n\r\n";
            mFullBuffer = response;
        }
        if (!error)
        {
            if (isEof)
            {
                // send response
                boost::asio::async_write(mSocket, boost::asio::buffer(mFullBuffer.c_str(), mFullBuffer.size()), boost::bind(&TcpConnection::HandleWrite, shared_from_this(), boost::asio::placeholders::error));
            }
            else
            {
                mSocket.async_read_some(boost::asio::buffer(mMessage, MAX_LENGTH), boost::bind(&TcpConnection::HandleRead, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
            }
        }
        else
        {
            std::cout << &error;
        }
     }


    const int decodePayloadLength(const char* bytes, int& maskOffset)
    {
        // payloadLen is in the second byte (byte[1]), between bits 1-7 (excl. bit 0)
        static const int msb = 7;
        static const int lsb = 1;
        unsigned int len = 0;

        // exclude the first bit of bytes[1] i.e. AND with 0111 1111 (dec 127)
        
        unsigned int result = bytes[1] & 127;
        switch (result)
        {
        case 127:
            // read the next 64 bits and interpret those as an unsigned integer (the most significant bit MUST be 0).
        {
            // use htonll here
            len = (unsigned int)htonll(*(uint64_t*)(bytes + 2));
            maskOffset = 10;// masking key starts at the 10th byte
            break;
        }
        case 126:
        {
            // read the next 16 bits and interpret those as an unsigned integer.
            len = (unsigned int)htons(*(uint16_t*)(bytes + 2));
            maskOffset = 4;// masking key starts at the 4th byte
            break;
        }
        default:
            len = result;
            maskOffset = 2;// masking key starts at the 2nd byte
            break;
        }

        return len;
    }

    const std::string unmaskData(const char* bytes, const unsigned int len, const unsigned int maskOffset)
    {
        // check if the mask bit was set first
        // we want  _
        // this bit 1000 0000
        std::string unmaskedData;

        int masked = (bytes[1] & 128) >> 7;
        if (masked)
        {
            const char* mask = bytes + maskOffset;// masking key starts at the 10th byte.  It's four bytes long.
            const char* payloadData = bytes + maskOffset + 4;// payload data starts at the 14th byte; length is passed into function

            for (int i = 0; i < len; ++i)
                unmaskedData += char(payloadData[i] ^ mask[i % 4]);
        }
            
        int messageLength = unmaskedData.size();
        return unmaskedData;
    }

    const std::string ProcessWebSocketKey(const std::string& key)
    {
        unsigned char hash[SHA_DIGEST_LENGTH];
        SHA1(reinterpret_cast<const unsigned char*>(key.c_str()), key.size(), hash);
        using namespace boost::archive::iterators;
        std::string hashAsc(reinterpret_cast<char*>(hash), SHA_DIGEST_LENGTH);
        using It = base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>;
        auto tmp = std::string(It(std::begin(hashAsc)), It(std::end(hashAsc)));
        return tmp.append((3 - hashAsc.size() % 3) % 3, '=');
    }

    tcp::socket mSocket;
    //std::string mMessage;
    enum {MAX_LENGTH = 1024};
    char mMessage[MAX_LENGTH];
    std::string mFullBuffer;
};

class TcpServer
{
public:
    TcpServer(boost::asio::io_service& io)
        : mAcceptor(io, tcp::endpoint(tcp::v4(), 45555))
    {
        StartAccept();
    }

private:
    void StartAccept()
    {
        std::cout << "Starting accept" << std::endl;
        TcpConnection::pointer newConnection = TcpConnection::create(mAcceptor.get_io_service());
        mAcceptor.async_accept(newConnection->socket(), boost::bind(&TcpServer::HandleAccept, this, newConnection, boost::asio::placeholders::error));
    }

    void HandleAccept(TcpConnection::pointer newConnection, const boost::system::error_code& error)
    {
        std::cout << "Handling accept" << std::endl;
        if (!error)
            newConnection->Start();

        StartAccept();
    }

    tcp::acceptor mAcceptor;
};

int main()
{
    try
    {
        boost::asio::io_service io;
        TcpServer server(io);
        std::cout << "Running" << std::endl;
        io.run();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}