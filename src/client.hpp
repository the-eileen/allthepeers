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
#include "common.hpp"
#include "meta-info.hpp"

namespace sbt {

class Client
{
public:
  Client(const std::string& port, const std::string& torrent)
  { 
    m_currPort = port;
    m_info = new MetaInfo;
    std::ifstream torrentStream(torrent, std::ifstream::in); //Jchu: convert to istream
    m_info->wireDecode(torrentStream);  // Josh: decode bencoded torrent file
   
  }
  MetaInfo* m_info;
  uint8_t* m_peerid;
  std::string getPort() const
  {  
    return m_currPort;
  }
private:
  std::string m_currPort;
};

} // namespace sbt

#endif // SBT_CLIENT_HPP
