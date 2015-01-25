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



#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include <iostream>
#include <sstream>


#include "meta-info.hpp"
#include "http/http-request.hpp"
#include "http/url-encoding.hpp"
#include "client.hpp"
#include <string>
#include <cstring>
  using namespace std;
  using namespace sbt;



#include "client.hpp"
//#include <iostream>

void makeGetRequest(Client client){
  MetaInfo* metainfo = client.m_info;
  HttpRequest req;
    req.setHost(client.m_url);
    req.setPort(client.m_trackPort);
    req.setMethod(HttpRequest::GET);
    string left = to_string(metainfo->getLength());
    string path = "/announce.php?info_hash=" + url::encode((const uint8_t *)(metainfo->getHash()->get()), 20) + "&peer_id=" + url::encode(client.m_peerid, 20) +
      "&port=" + client.getPort() + "&uploaded=0&downloaded=0&left=" + left + "&event=started";
    req.setPath(path);
    req.setVersion("1.0");
    //req.addHeader("Accept-Language", "en-US");
    size_t reqLen = req.getTotalLength();
    char *buf = new char [reqLen];
    req.formatRequest(buf);
    string formatted = buf;

    cerr << "request is:" << buf;

 
    //return buf;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(client.m_trackPort);     // short, network byte order
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));

  // connect to the server
  if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
    perror("connect");
    //return 2;
  }

  struct sockaddr_in clientAddr;
  socklen_t clientAddrLen = sizeof(clientAddr);
  if (getsockname(sockfd, (struct sockaddr *)&clientAddr, &clientAddrLen) == -1) {
    perror("getsockname");
    //return 3;
  }

  char ipstr[INET_ADDRSTRLEN] = {'\0'};
  inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));
  std::cout << "Set up a connection from: " << ipstr << ":" <<
    ntohs(clientAddr.sin_port) << std::endl;

    bool isEnd = false;
  //std::string input;
  char rbuf[20] = {0};
  std::stringstream ss;
 fprintf(stderr,"before while");
  while (!isEnd) {
   fprintf(stderr,"inside while");

    if (send(sockfd, formatted.c_str(), formatted.size(), 0) == -1) {
      fprintf(stderr, "SEND FAILED");
      perror("send");
      //return 4;
      
    }

    fprintf(stderr, "SENT");

    if (recv(sockfd, rbuf, sizeof(rbuf), 0) == -1) {
      //cerr << "RECEIVE FAILED";
      perror("recv");
      
      //return 5;
    }
   // cerr << endl << "RECIEVED";
    ss << buf << std::endl;

    if (ss.str() == "close\n")
      break;

    ss.str("");
  }

  cerr << "out of while";
  close(sockfd);
}

int
main(int argc, char** argv)
{
  try
  {
    // Check command line arguments.
    if (argc != 3)
    {
      std::cerr << "Usage: simple-bt <port> <torrent_file>\n";
      return 1;
    }

    // Initialise the client.
    sbt::Client client(argv[1], argv[2]);
    // Josh: tests to see if decoded properly
    //std::cout << "Name:  " << client.m_info->getName() << std::endl;
    //std::cout << "Torrent file length: " << client.m_info->getLength() << std::endl;
    //std::cout << "Announce: " << client.m_info->getAnnounce() << std::endl;
    makeGetRequest(client);
  }
  catch (std::exception& e)
  {
    std::cerr << "exception: " << e.what() << "\n";
  }

  return 0;
}
