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
#include <math.h>

#include <list>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>

#include "meta-info.hpp"
#include "http/http-request.hpp"
#include "http/url-encoding.hpp"
#include "client.hpp"
#include "tracker-response.hpp"
#include "msg/handshake.hpp"
#include <string>
#include <cstring>
  using namespace std;
  using namespace sbt;
  using namespace msg;

void makeGetRequest(Client client){
  MetaInfo* metainfo = client.m_info;
  HttpRequest req;
    req.setHost(client.m_hostName);
    req.setPort(client.m_trackPort);
    req.setMethod(HttpRequest::GET);
    string left = to_string(metainfo->getLength());
    string path = client.m_path + "?info_hash=" + url::encode((const uint8_t *)(metainfo->getHash()->get()), 20) + "&peer_id=" + url::encode(client.m_peerid, 20) +
      "&port=" + client.getPort() + "&uploaded=0&downloaded=0&left=" + left + "&event=started";
    req.setPath(path);
    req.setVersion("1.0");
    //req.addHeader("Accept-Language", "en-US");
    size_t reqLen = req.getTotalLength();
    char *buf = new char [reqLen];
    req.formatRequest(buf);
    string formatted = buf;

    //cerr << "request is:" << buf;

 
    //return buf;
    /*int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(client.m_trackPort);     // short, network byte order
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));
*/
  // connect to the server
  /*if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
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
    ntohs(clientAddr.sin_port) << std::endl;*/

    bool isEnd = false;
    bool isFirst = true;
  //std::string input;
  char rbuf[10000] = {0};
  std::stringstream ss;
// fprintf(stderr,"before while");

  std::vector<Peer> peerList;
  int numOfPieces = ceil(client.m_info->getLength() / client.m_info->getPieceLength());
  while (!isEnd) {
    // fprintf(stderr,"inside while");

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(client.m_trackPort);     // short, network byte order
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));


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
    //  std::cout << "Set up a connection from: " << ipstr << ":" <<
    //    ntohs(clientAddr.sin_port) << std::endl;

    if (send(sockfd, formatted.c_str(), formatted.size(), 0) == -1) {
      //fprintf(stderr, "SEND FAILED");
      perror("send");
      //return 4;
      
    }

    path = client.m_path + "?info_hash=" + url::encode((const uint8_t *)(metainfo->getHash()->get()), 20) + "&peer_id=" + url::encode(client.m_peerid, 20) +
      "&port=" + client.getPort() + "&uploaded=0&downloaded=0&left=" + left;
    req.setPath(path);
    size_t reqLeng = req.getTotalLength();
    char *buf2 = new char [reqLeng];
    
    req.formatRequest(buf2);
    formatted = buf2;

    //cerr << "formatted is" << formatted;


    //fprintf(stderr, "SENT");

    if (recv(sockfd, rbuf, sizeof(rbuf), 0) == -1) {
      //cerr << "RECEIVE FAILED";
      perror("recv");
      
      //return 5;
    }
   // cerr << endl << "RECIEVED";
    int rbuf_size = 0;
    while(rbuf[rbuf_size] != '\0')
    {
        rbuf_size++;
    } 
    if(rbuf_size == 0){
      continue; 
    }
    const char* body;
    bencoding::Dictionary dict;
    HttpResponse httpResp;
    body = httpResp.parseResponse(rbuf, rbuf_size);

    std::istringstream req_stream(body);
    dict.wireDecode(req_stream);
    TrackerResponse* trackerResponse = new TrackerResponse();
    trackerResponse->decode(dict); 

    if(isFirst)
    {
    isFirst = false;
    // Josh: first create file that we'll be writing to
    fstream targetFile;
    targetFile.open("text.txt"); 
    for(std::vector<PeerInfo>::const_iterator it = (trackerResponse->getPeers()).begin(); it != (trackerResponse->getPeers()).end(); it++)
    {
      std::stringstream ss;
      ss << it->port;
      string port = ss.str();
      if ((it->ip != "127.0.0.1") && (port != client.getPort())) // check that it's not client
        peerList.push_back(Peer(*it, numOfPieces));
    }
      char rshake[68] = {0};
    
    //Eileen start
   vector<int> socketList;

for(std::vector<Peer>::iterator it = peerList.begin(); it != peerList.end() ; it++){
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  socketList.push_back(sockfd);
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(it->m_port);     // short, network byte order
  const char* peerip = it->m_ip.c_str();
  addr.sin_addr.s_addr = inet_addr(peerip);
  memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));
  if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
    perror("connect");
    //return 2;
    }



  HandShake hshake = HandShake(metainfo->getHash(), client.m_strId);
  char* buf = (char*)hshake.encode()->buf();
  if(send(sockfd, buf, 68, 0) == -1)
    perror("send");
  if (recv(sockfd, rshake, sizeof(rshake), 0) == -1) 
    perror("recv");
  ConstBufferPtr peerShake = std::make_shared<Buffer>(rshake, 68);
}
//Eileen end
    }
    int waitTime = trackerResponse->getInterval();

    sleep(waitTime);

    ss << buf << std::endl;

    if (ss.str() == "close\n")
      break;

    ss.str("");
    close(sockfd);
  }



 // close(sockfd);
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
    //std::cout << "Announce: " << client.m_url << std::endl;
    //std::cout << "TrackerPort: " << client.m_trackPort << std::endl;
    makeGetRequest(client);
  }
  catch (std::exception& e)
  {
    std::cerr << "exception: " << e.what() << "\n";
  }

  return 0;
}
