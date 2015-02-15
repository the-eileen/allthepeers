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
#include "msg/msg-base.hpp"

  using namespace std;
  using namespace sbt;
  using namespace msg;

bool* PIECESOBTAINED;  // Josh: global; size declared once numOfPieces obtained
int numOfPieces;

void updatePiecesArray(int whichPiece) // use to update PIECES array
{
  PIECESOBTAINED[whichPiece] = true;
}

bool areAllPiecesObtained()
{
  int numTrue = 0;
  for (int i = 0; i < numOfPieces; i++)
    if (PIECESOBTAINED[i] == true)
      numTrue++;
  if (numTrue == numOfPieces)
    return true;
  return false; 
}
 
int shakeHands(Peer pr, Client client){ //takes peer and client, returns socket created for this peer
  MetaInfo* metainfo = client.m_info;
  int sockfd = socket(AF_INET, SOCK_STREAM, 0); //create socket
  
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(pr.m_port);     
  const char* peerip = pr.m_ip.c_str();
  addr.sin_addr.s_addr = inet_addr(peerip);
  memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));
  if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    perror("connect");
    //return 2;
    }

  char rshake[68] = {0}; //find a way to save this somewhere
  HandShake hshake = HandShake(metainfo->getHash(), client.m_strId);
  char* buf = (char*)hshake.encode()->buf();
  if(send(sockfd, buf, 68, 0) == -1)
    perror("send");
  if (recv(sockfd, rshake, sizeof(rshake), 0) == -1) 
    perror("recv");

  cerr << "rshake is" << rshake;
  fprintf(stderr, "PRINT");
  ConstBufferPtr peerShake = std::make_shared<Buffer>(rshake, 68);
  return sockfd;
}
void bitFieldProt(Peer peer, int peersock){
    //takes in a peer and it's socket, sends bitfield, populates 
    //      the peer's boolean array
    //assumes that it's already connected to the peer

    //this should technically work, since arrays are just sequential bits...right?
    int toSendSize = (numOfPieces + 7) /8;
    char* moddedBitField;
    moddedBitField = new char[toSendSize];
    char tmp = {0};
    for(int i = 0; i < toSendSize; i++)
    {
        if(i == toSendSize - 1)
        {
            int bitsNeeded = numOfPieces % 8;
            bitsNeeded = (bitsNeeded == 0) ? 8 : bitsNeeded;
            tmp = {0};
            for(int j = 0; j < 8; j++)
            {
                tmp = tmp << 1;

                if(j < bitsNeeded && PIECESOBTAINED[(i*8)+j] == 1)
                {
                    //gotta add one bruh
                    tmp = tmp | 0x01;
                } //look shit's pretty fucked here but tmp should be a thing
            }
            memset(moddedBitField + i, tmp, sizeof(char));
        }
        else
        {
            for(int j = 0; j < 8; j++)
            {
                if(PIECESOBTAINED[(i*8) + j] == 1)
                {
                    tmp = tmp << 1;
                    tmp = tmp | 0x01;
                }
                else
                {
                    tmp = tmp << 1;
                }
            }
            memset(moddedBitField + i, tmp, sizeof(char));
        }
    }

    ConstBufferPtr tempPtr = make_shared<Buffer>(moddedBitField, toSendSize);
    Bitfield* bitField = new Bitfield(tempPtr);

    //rumor has it that PIECESOBTAINED needs to be a multiple of 8? Maybe?

    //bitField->encodePayload();
    Bitfield* pBitField = new Bitfield();

    ConstBufferPtr enc_mes = bitField->encode();
    int msg_len = bitField->getPayload()->size() + 5;
    const char* b_msg = reinterpret_cast<const char*>(enc_mes->buf());
    cerr << "sending" << endl;
    if(send(peersock, b_msg, msg_len, 0) == -1)
    {
        perror("send");
    }
    cerr <<"receining"<< endl;

    uint8_t hs_buf[1000] = {'\0'};
    ssize_t n_buf = 0;
    if((n_buf = recv(peersock, hs_buf, sizeof(hs_buf), 0)) == -1)
    //if(recv(peersock, pBitField, toSendSize+5, 0) == -1)
    {
      cerr <<"uh oh spahghethgti o" << endl;
        perror("receive");
    }

    ConstBufferPtr hs_res = make_shared<Buffer>(hs_buf, n_buf);

    cerr <<"theers a thing here somethinwere :" << sizeof(hs_res->buf()) << endl;
    //pBitField->decodePayload();
    ///ConstBufferPtr newBF = pBitField->getPayload();

    //peer.m_pieceIndex = (bool*)(newBF->buf());
    //cerr << "Peer's bitfield 1 is " << peer.m_pieceIndex[0] << endl;
}
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
        cout << "printed";
 
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

    bool isFirst = true;
  //std::string input;
    char rbuf[10000] = {0};
    std::stringstream ss;
// fprintf(stderr,"before while");

    std::vector<Peer*> peerList;
  //TODO: This won't work.  Ceil takes in a float.  The int value will be evaluated first
   // Josh: I tested this and by casting the first value to double, it evaluates properly
    numOfPieces = ceil(((double)client.m_info->getLength()) / client.m_info->getPieceLength()); 
    //made this Global
    /*
    cerr << "length of file: " << client.m_info->getLength() << std::endl;
    cerr << "PieceLength: " << client.m_info->getPieceLength() << std::endl;
    cerr << "numOfPieces: " << numOfPieces << std::endl;
    cerr << "client id: " << client.m_strId << std::endl;
    cerr << "client port: " << client.getPort() << std::endl;
    */
  PIECESOBTAINED = new bool[numOfPieces](); // Josh: all pieces false (not obtained yet)
  while (!areAllPiecesObtained()) {
    // fprintf(stderr,"inside while");

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    //cerr << client.m_trackPort << std::endl;
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
    // TRACKER REQUEST NEEDS TO BE UPDATED
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
      /*
      cerr << "PeerInfo Size: " << trackerResponse->getPeers().size() << std::endl;
      cerr << "PeerInfo ID: " << it->peerId << std::endl;
      cerr << "PeerInfo ip: " << it->ip << std::endl;
      cerr << "PeerInfo Port: " << it->port << std::endl;
      */
      std::stringstream ss;
      ss << it->port;
      string port = ss.str();
      
      //cerr << "PeerInfo port: " << port << std::endl;
      //cerr << "Client Port: " << client.getPort() << std::endl;
      if (!((it->ip == "127.0.0.1") && (port == client.getPort()))) // check that it's not client
      {
        //cerr << "Condition passed" << std::endl;
        Peer* temp = new Peer(*it, numOfPieces);
        peerList.push_back(temp);
      }
    }
     
    //cerr << "reached" << std::endl;
   /* 
   for(std::vector<Peer*>::iterator it = peerList.begin(); it != peerList.end() ; it++)
   {
      cerr << "Peer Size: " << peerList.size() << std::endl;
      cerr << "Peer ID: " << (*it)->m_peerId << std::endl;
      cerr << "Peer ip: " << (*it)->m_ip << std::endl;
      cerr << "Peer Port: " << (*it)->m_port << std::endl;
      cerr << "Peer has the following pieces: ";
      for (int i = 0; i < (*it)->m_numPieces; i++)
        cerr << (*it)->m_pieceIndex[i] << " ";
      
   }
   
   cerr << "Pieces Obtained: ";
   for (int i = 0; i < 23; i++)
        cerr << PIECESOBTAINED[i] << " ";
   */
   
        vector<int> socketList;
    
        for(std::vector<Peer*>::iterator it = peerList.begin(); it != peerList.end() ; it++){
           int curSock = shakeHands(**it,client);
           socketList.push_back(curSock);
           bitFieldProt(**it, curSock);
           (*it)->m_sockfd = curSock; 
       }

        
    }
    // end of isFirst
    
    // Josh start (WORK IN PROGRESS)
    /*
    for (std::vector<Peer*>::iterator it = peerList.begin(); it != peerList.end(); it++)
    {
       if ((*it)->m_amInterested == true)
       {
         // starting here, I have a lot of trouble with the formatting
         if ((*it)->m_buff[0] == '\0') // empty buffer
         {
             Choke choke = Choke();
             char* buf = (char*)choke.encode()->buf();
             if (send((*it)->m_sockfd, buf, 50, 0) == -1)
	       perror("send");
         }
         else // something in buffer
         {
           ConstBufferPtr msg = (MsgBase)((*it)->m_buff);
           int msgType = msg->getId();
           switch (msgType) 
           {
             case 1: // unchoke
               //send request
               break;
             case 7: // piece
               // verify piece with hash
               // if actually piece
               {
                 // send have
               }
               // else resend request
           }
           
         }
       
       }
       else // uninterested
       {
         if ((*it)->m_buff[0] == '\0') // empty buffer
         {
           if (recv((*it)->m_sockfd, (*it)->m_buff, sizeof((*it)->m_buff), 0) == -1) 
            perror("recv");
         }
         else // something in buffer
         {
           int msgType = (*it)->m_buff[5];
           switch (msgType)
           {
             case 2: // peer interested
               // send choke
               break;
             case 4: // have
               if ((*it)->m_buff[6] != '\0')
                 (*it)->setInterest((*it)->m_buff[6]);
               break;
           }
         }
       }
    }
    */
    // Josh end
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
    //cerr << "about to enter makeGetRequest: " << std::endl;

    makeGetRequest(client);

  }
  catch (std::exception& e)
  {
    std::cerr << "exception: " << e.what() << "\n";
  }

  return 0;
}
