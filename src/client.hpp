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
namespace sbt {

std::string getHostNameFromAnnounce(std::string announce, int &start);
std::string getPortFromAnnounce(std::string announce, int &start);
std::string getPathFromAnnounce(std::string announce, int &start);

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
