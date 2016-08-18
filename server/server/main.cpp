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
#include <set>

#define htonll(x) ((((uint64_t)htonl(x)) << 32) + htonl((x) >> 32))

using boost::asio::ip::tcp;

const std::string WS_KEY = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";// magic string for WebSockets

#if 0

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

#else

class chat_message
{
public:
    enum { header_length = 4 };
    enum { max_body_length = 512 };
    enum { chunk_length = 32 };

    chat_message()
        : body_length_(0)
    {
    }

    const char* data() const
    {
        return data_;
    }

    char* data()
    {
        return data_;
    }

    std::size_t length() const
    {
        return header_length + body_length_;
    }

    const char* body() const
    {
        return data_ + header_length;
    }

    char* body()
    {
        return data_ + header_length;
    }

    std::size_t body_length() const
    {
        return body_length_;
    }

    void body_length(std::size_t new_length)
    {
        body_length_ = new_length;
        if (body_length_ > max_body_length)
            body_length_ = max_body_length;
    }

    bool decode_header()
    {
        char header[header_length + 1] = "";
        std::strncat(header, data_, header_length);
        body_length_ = std::atoi(header);
        if (body_length_ > max_body_length)
        {
            body_length_ = 0;
            return false;
        }
        return true;
    }

    void encode_header()
    {
        char header[header_length + 1] = "";
        std::sprintf(header, "%4d", body_length_);
        std::memcpy(data_, header, header_length);
    }

private:
    char data_[header_length + max_body_length];
    std::size_t body_length_;
};

typedef std::deque<chat_message> chat_message_queue;

//----------------------------------------------------------------------

class chat_participant
{
public:
    virtual ~chat_participant() {}
    virtual void deliver(const chat_message& msg) = 0;
};

typedef std::shared_ptr<chat_participant> chat_participant_ptr;

//----------------------------------------------------------------------

class chat_room
{
public:
    void join(chat_participant_ptr participant)
    {
        participants_.insert(participant);
        for (auto msg : recent_msgs_)
            participant->deliver(msg);
    }

    void leave(chat_participant_ptr participant)
    {
        participants_.erase(participant);
    }

    void deliver(const chat_message& msg)
    {
        recent_msgs_.push_back(msg);
        while (recent_msgs_.size() > max_recent_msgs)
            recent_msgs_.pop_front();

        for (auto participant : participants_)
            participant->deliver(msg);
    }

private:
    std::set<chat_participant_ptr> participants_;
    enum { max_recent_msgs = 100 };
    chat_message_queue recent_msgs_;
};

//----------------------------------------------------------------------

class chat_session
    : public chat_participant,
    public std::enable_shared_from_this<chat_session>
{
public:
    chat_session(tcp::socket socket, chat_room& room)
        : socket_(std::move(socket)),
        room_(room)
    {
    }

    void start()
    {
        room_.join(shared_from_this());

        do_read_handshake();
    }

    void deliver(const chat_message& msg)
    {
        bool write_in_progress = !write_msgs_.empty();
        write_msgs_.push_back(msg);
        if (!write_in_progress)
        {
            do_write();
        }
    }

private:
    void do_read_header()
    {
        auto self(shared_from_this());
        boost::asio::async_read(socket_,
            boost::asio::buffer(read_msg_.data(), chat_message::header_length),
            [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
            if (!ec && read_msg_.decode_header())
            {
                do_read_body();
            }
            else
            {
                room_.leave(shared_from_this());
            }
        });
    }

    void do_read_body()
    {
        auto self(shared_from_this());
        boost::asio::async_read(socket_,
            boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
            [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
            if (!ec)
            {
                room_.deliver(read_msg_);
                do_read_header();
            }
            else
            {
                room_.leave(shared_from_this());
            }
        });
    }

    void do_read_handshake()
    {
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(read_msg_.data(), read_msg_.chunk_length),
            [this, self](boost::system::error_code ec, std::size_t length)
        {
            if (!ec)
            {
                buf.append(read_msg_.data(), length);
                if (buf.find("\r\n\r\n") == -1)
                {
                    do_read_handshake();
                }
                else
                {
                    static const std::string keyTag = "Sec-WebSocket-Key: ";
                    const size_t startElement = buf.find(keyTag) + keyTag.size();
                    const size_t endElement = buf.find("\r\n", startElement);
                    std::string key = buf.substr(startElement, endElement - startElement);

                    std::string str = ProcessWebSocketKey(key + WS_KEY);

                    std::string response;
                    response += "HTTP/1.1 101 Switching Protocols\r\n";
                    response += "Upgrade: websocket\r\n";
                    response += "Connection: Upgrade\r\n";
                    response += "Sec-WebSocket-Accept: ";
                    response += str;
                    response += "\r\n\r\n";

                    buf = response;
                    do_write_handshake();
                }
            }
            else
            {
                room_.leave(shared_from_this());
            }
        });
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

    void do_write_handshake()
    {
        auto self(shared_from_this());
        boost::asio::async_write(socket_,
            boost::asio::buffer(buf, buf.size()),
            [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
            if (!ec)
            {
                buf.clear();
                do_read_header();
            }
            else
            {
                room_.leave(shared_from_this());
            }
        });
    }

    void do_write()
    {
        auto self(shared_from_this());
        boost::asio::async_write(socket_,
            boost::asio::buffer(write_msgs_.front().data(),
                write_msgs_.front().length()),
            [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
            if (!ec)
            {
                write_msgs_.pop_front();
                if (!write_msgs_.empty())
                {
                    do_write();
                }
            }
            else
            {
                room_.leave(shared_from_this());
            }
        });
    }

    tcp::socket socket_;
    chat_room& room_;
    chat_message read_msg_;
    chat_message_queue write_msgs_;
    std::string buf;
};

//----------------------------------------------------------------------

class chat_server
{
public:
    chat_server(boost::asio::io_service& io_service,
        const tcp::endpoint& endpoint)
        : acceptor_(io_service, endpoint),
        socket_(io_service)
    {
        do_accept();
    }

private:
    void do_accept()
    {
        acceptor_.async_accept(socket_,
            [this](boost::system::error_code ec)
        {
            if (!ec)
            {
                std::make_shared<chat_session>(std::move(socket_), room_)->start();
            }

            do_accept();
        });
    }

    tcp::acceptor acceptor_;
    tcp::socket socket_;
    chat_room room_;
};

//----------------------------------------------------------------------

int main(int argc, char* argv[])
{
    try
    {
        if (argc < 2)
        {
            std::cerr << "Usage: chat_server <port> [<port> ...]\n";
            return 1;
        }

        boost::asio::io_service io_service;

        std::list<chat_server> servers;
        for (int i = 1; i < argc; ++i)
        {
            tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[i]));
            servers.emplace_back(io_service, endpoint);
        }

        io_service.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}

#endif

