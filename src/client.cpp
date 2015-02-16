#include "client.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <iostream>
    using namespace std;

extern bool* PIECESOBTAINED; // declared in main.cpp

namespace sbt {

std::string getHostNameFromAnnounce(std::string announce, int &start);
std::string getPortFromAnnounce(std::string announce, int &start);
std::string getPathFromAnnounce(std::string announce, int &start);


Peer::Peer(const PeerInfo& pi, int numPieces)
{
    m_peerId = pi.peerId;
    m_ip = pi.ip;
    m_port = pi.port;
    m_pieceIndex = new bool[numPieces](); // zero-initialized (false)
    m_numPieces = numPieces;
    m_peerChoking = true;
    m_peerInterested = false; 
    m_amChoking = true;
    m_amInterested = false;
    m_sockfd = -1;
    m_desiredPiece = -1;
    m_buffSize = 0;
}
Peer::Peer(const Peer& other)
{
    memcpy(m_pieceIndex, other.m_pieceIndex, sizeof(bool) * m_numPieces);
    m_peerId = other.m_peerId;
    m_ip = other.m_ip;
    m_port = other.m_port;
    m_numPieces = other.m_numPieces;
    m_amChoking = other.m_amChoking;
    m_amInterested = other.m_amInterested;
    m_peerChoking = other.m_peerChoking;
    m_peerInterested = other.m_peerInterested;  
    memcpy(m_buff, other.m_buff, sizeof(char) * 50);
    m_sockfd = other.m_sockfd;
    m_buffSize = other.m_buffSize;
    m_desiredPiece = other.m_desiredPiece;
}
Peer::~Peer()
{
    delete[] m_pieceIndex;
}
bool Peer::updateInterest()
{
    for (int i = 0; i < m_numPieces; i++)
    {
     if (PIECESOBTAINED[i] == false && m_pieceIndex[i] == true)
     {
        m_amInterested = true;
        return m_amInterested;
     }
    }
    m_amInterested = false; // no pieces of interest found
    m_desiredPiece = -1;
    return m_amInterested;
}
bool Peer::setInterest(int whichPiece) // call after receiving a have
{
   m_pieceIndex[whichPiece] = true; //mark that this peer has this piece
   return updateInterest();
}
ssize_t Peer::sendMsg(msg::MsgBase* msg)
{
  ConstBufferPtr enc = msg->encode();
  const char* buf = reinterpret_cast<const char*>(enc->buf());
  int msg_len = 5;
  return send(m_sockfd, buf, msg_len, 0);
}
ssize_t Peer::sendMsgWPayload(msg::MsgBase* msg)
{
  ConstBufferPtr enc = msg->encode();
  const char* buf = reinterpret_cast<const char*>(enc->buf());
  int msg_len = msg->getPayload()->size() + 5;
  return send(m_sockfd, buf, msg_len, 0);
}
ssize_t Peer::recvMsg()
{
  m_buffSize = recv(m_sockfd, m_buff, sizeof(m_buff), 0 );
  cerr << "bufsize is: " << m_buffSize << endl;
  return m_buffSize;
}
void Peer::resetBuff()
{
  m_buffSize = 0;
  memset(m_buff, '\0', sizeof(m_buff));
}

Client::Client(const std::string& port, const std::string& torrent)
{ 
    m_currPort = port;
    m_info = new MetaInfo;
    std::ifstream torrentStream(torrent, std::ifstream::in); //Jchu: convert to istream
    m_info->wireDecode(torrentStream);  // Josh: decode bencoded torrent file

    // Josh: DO NOT change the order of the three functions below; dependent on each other
    m_announce = (m_info->getAnnounce()).c_str();
    m_announcePos = 0;
    m_hostName = getHostNameFromAnnounce(m_announce, m_announcePos); 
    m_trackPort = atoi(getPortFromAnnounce(m_announce, m_announcePos).c_str());
    //m_strId = reinterpret_cast<const char*>(m_peerid);
    m_strId = "SIMPLEBT.TEST.800813";
    m_path = getPathFromAnnounce(m_announce, m_announcePos);
    //m_Port = atoi(port.c_str());
    //m_strCport = getPortFromAnnounce(m_url);
}
std::string Client::getPort() const
{  
  return m_currPort;
}


std::string getHostNameFromAnnounce(std::string announce, int &start)
{
    if (announce == "")
    {
        std::cerr << "No Announce File\n";
        return "";
    }
    int i = start;
    int strlen = 0;
    int startPos = start;
    while (announce[i] != '\0') {
        if (announce[i] == '/' && announce[i+1] == '/' )
        {
            startPos += 2;
            strlen++;
            i += 2;
            while (announce[i+1] != ':') {
                strlen++;
                i++;
            }
            break;
        }
      i++;
      startPos++;
    }
    start = i;
    return announce.substr(startPos, strlen);
}

std::string getPortFromAnnounce(std::string announce, int &start)
{
    if (announce == "")
    {
        std::cerr << "No Announce File\n";
        return "";
    }
    int i = start;
    int strlen = 0;
    int startPos = start;
    while (announce[i] != '\0') {
        if (announce[i] == ':' && isdigit(announce[i+1]) )
        {
            startPos++;
            strlen++;
            i++;
            while (isdigit(announce[i+1])) {
                strlen++;
                i++;
            }
            break;
        }
        i++;
        startPos++;
    }
    start = i;
    return announce.substr(startPos, strlen);
}

std::string getPathFromAnnounce(std::string announce, int &start)
{
    if (announce == "")
    {
        std::cerr << "No Announce File\n";
        return "";
    }
    int i = start;
    int strlen = 0;
    int startPos = start;
    while (announce[i] != '\0') {
        if (announce[i] == '/' )
        {
            strlen++;
            while (announce[i+1] != '\0') {
                strlen++;
                i++;
            }
            break;
        }
        i++;
        startPos++;
    }
    return announce.substr(startPos, strlen);
}


} // namespace sbt
