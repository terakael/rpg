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

using boost::asio::ip::tcp;

const std::string WS_KEY = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";// magic string for WebSockets

std::string makeDaytimeString()
{
    std::time_t now = std::time(0);
    return std::ctime(&now);
}

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
            buf = buf.substr(0, buf.find_first_of("\r\n\r\n") + 2);
        }

        mFullBuffer += buf;

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
            const std::string keyTag = "Sec-WebSocket-Key: ";

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
    enum {MAX_LENGTH = 16};
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