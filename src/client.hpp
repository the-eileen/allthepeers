/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California
 *
 * This file is part of Simple BT.
 * See AUTHORS.md for complete list of Simple BT authors and contributors.
 *
 * NSL is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NSL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NSL, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 *
 * \author Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef SBT_CLIENT_HPP
#define SBT_CLIENT_HPP

#include <fstream>
#include <string>
#include "common.hpp"
#include "meta-info.hpp"
#include "http/http-request.hpp"
#include "http/http-response.hpp"
#include "tracker-response.hpp"
#include "msg/msg-base.hpp"

extern bool* PIECESOBTAINED; // declared in main.cpp

namespace sbt {

std::string getHostNameFromAnnounce(std::string announce, int &start);
std::string getPortFromAnnounce(std::string announce, int &start);
std::string getPathFromAnnounce(std::string announce, int &start);

class Peer
{
public:
  Peer(const PeerInfo& pi, int numPieces)
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
    m_buffSize = -1;
  }
  Peer(const Peer& other)
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
  ~Peer()
  {
    delete[] m_pieceIndex;
  }
  void setInterest(int whichPiece)
  {
     m_pieceIndex[whichPiece] = true; //mark that this peer has this piece
     for (int i = 0; i < m_numPieces; i++)
     {
       if (PIECESOBTAINED[i] == false && m_pieceIndex[i] == true)
       {
          m_amInterested = true;
          m_desiredPiece = static_cast<uint32_t>(i); // set first piece they have that we don't
          break;
       }
     }
  }
  ssize_t sendMsg(msg::MsgBase& msg)
  {
    char* buf = (char*)(msg.encode()->buf());
    int msg_len = msg.getPayload()->size() + 5;
    return send(m_sockfd, buf, msg_len, 0);
  }
  ssize_t recvMsg()
  {
    m_buffSize = recv(m_sockfd, m_buff, sizeof(m_buff), 0 );
    return m_buffSize;
  }
  std::string m_peerId;
  std::string m_ip;
  uint16_t m_port;
  bool* m_pieceIndex;  // keep track of pieces this peer has
  int m_numPieces;
  // Josh: not sure how many of these we need
  bool m_amChoking;     // I am choking this peer
  bool m_amInterested;  // I am interested in this peer
  bool m_peerChoking;   // This peer is choking me
  bool m_peerInterested;// This peer is interested in me
  char m_buff[100] = {}; // store messages
  int m_buffSize;  // how filled up it is
  int m_sockfd;
  uint32_t m_desiredPiece; //TODO: only holds single value, probably gonna need a peer connection manager implemented later
};


class Client
{
public:
  Client(const std::string& port, const std::string& torrent)
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

  MetaInfo* m_info;
  uint8_t* m_peerid;
  std::string m_hostName;
  unsigned short m_trackPort;
  unsigned short m_Port;
  std::string m_strCport;
  std::string m_announce;
  std::string m_path;
  int m_announcePos;
  std::string m_strId;
  std::string getPort() const
  {  
    return m_currPort;
  }
private:
  std::string m_currPort;
};

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

#endif // SBT_CLIENT_HPP
