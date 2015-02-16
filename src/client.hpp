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


namespace sbt {

class Peer
{
public:
  Peer(const PeerInfo& pi, int numPieces);
  Peer(const Peer& other);
  ~Peer();
  void updateInterest(int whichPiece);
  void setInterest();
  ssize_t sendMsg(msg::MsgBase& msg);
  ssize_t recvMsg();
  
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
  Client(const std::string& port, const std::string& torrent);

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
  std::string getPort() const;
private:
  std::string m_currPort;
};

} // namespace sbt

#endif // SBT_CLIENT_HPP
