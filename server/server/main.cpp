#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <ctime>

using boost::asio::ip::tcp;

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
            std::cout << "Message: " << mMessage << std::endl;
            mSocket.async_read_some(boost::asio::buffer(mMessage, MAX_LENGTH), boost::bind(&TcpConnection::HandleRead, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        }
    }

    void HandleRead(const boost::system::error_code& error, size_t bytesTransferred)
    {
        if (!error)
            boost::asio::async_write(mSocket, boost::asio::buffer(mMessage, bytesTransferred), boost::bind(&TcpConnection::HandleWrite, shared_from_this(), boost::asio::placeholders::error));
    }

    tcp::socket mSocket;
    //std::string mMessage;
    enum {MAX_LENGTH = 1024};
    char mMessage[MAX_LENGTH];
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