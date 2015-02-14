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
namespace sbt {

std::string getHostNameFromAnnounce(std::string announce, int &start);
std::string getPortFromAnnounce(std::string announce, int &start);
std::string getPathFromAnnounce(std::string announce, int &start);

class Peer
{
public:
  Peer(PeerInfo pi, int numPieces)
  {
    m_peerId = pi.peerId;
    m_ip = pi.ip;
    m_port = pi.port;
    m_pieceIndex = new bool[numPieces](); // zero-initialized (false)
    m_numPieces = numPieces;
    m_peerChoked = true;
    m_peerInterested = false; 
    m_amChoked = true;
    m_amInterested = false;
  }
  Peer(const Peer& other)
  {
    memcpy(m_pieceIndex, other.m_pieceIndex, sizeof(bool) * m_numPieces);
    m_peerId = other.m_peerId;
    m_ip = other.m_ip;
    m_port = other.m_port;
    m_numPieces = other.m_numPieces;
    m_amChoked = other.m_amChoked;
    m_amInterested = other.m_amInterested;
    m_peerChoked = other.m_peerChoked;
    m_peerInterested = other.m_peerInterested;  
  }
  ~Peer()
  {
    delete[] m_pieceIndex;
  }
  std::string m_peerId;
  std::string m_ip;
  uint16_t m_port;
  bool* m_pieceIndex;  // keep track of pieces this peer has
  int m_numPieces;
  bool m_amChoked;     // I am choked by this peer
  bool m_amInterested; // I am interested in this peer
  bool m_peerChoked;   // I am choking this peer
  bool m_peerInterested;//This peer is interested in me

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
    m_strId = reinterpret_cast<const char*>(m_peerid);
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
