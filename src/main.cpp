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
#include "util/hash.hpp"

  using namespace std;
  using namespace sbt;
  using namespace msg;
  using namespace util;

bool* PIECESOBTAINED;  // Josh: global; size declared once numOfPieces obtained
int numOfPieces;
int nextStartReq = 0;

void getNextReq(Peer &peer);

void updatePiecesArray(int whichPiece) // use to update PIECES array
{
  PIECESOBTAINED[whichPiece] = true;
}

/* Josh: already implemented in client.cpp
void checkAndUpdateInterested(Peer &peer)
{
    peer.m_amInterested = false;
    for(int i = 0; i < peer.m_numPieces; i++)
    {
        if(PIECESOBTAINED[i] == false && peer.m_pieceIndex[i] == true)
        {
            peer.m_amInterested = true;
        }
    }
}*/

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
 
int shakeHands(Peer &pr, Client &client){ //takes peer and client, returns socket created for this peer
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
;
  ConstBufferPtr peerShake = std::make_shared<Buffer>(rshake, 68);
  return sockfd;
}
void bitFieldProt(Peer &peer, int peersock){
    //cout << peer.m_numPieces << endl;
    //    for(int i = 0; i < peer.m_numPieces; i++)
    //    cout << peer.m_pieceIndex[i] ;
    //takes in a peer and it's socket, sends bitfield, populates 
    //      the peer's boolean array
    //assumes that it's already connected to the peer

    //this should technically work, since arrays are just sequential bits...right?
    int toSendSize = (numOfPieces + 7) /8;
    char* moddedBitField;
    moddedBitField = new char[toSendSize];
    char tmp = {0};

    int bitsNeeded = numOfPieces % 8;
    bitsNeeded = (bitsNeeded == 0) ? 8 : bitsNeeded;
    for(int i = 0; i < toSendSize; i++)
    {
        if(i == toSendSize - 1)
        {
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
    //cerr << (int)moddedBitField[0] << endl;
    ConstBufferPtr tempPtr = make_shared<Buffer>(moddedBitField, toSendSize);
    Bitfield* bitField = new Bitfield(tempPtr);

    //rumor has it that PIECESOBTAINED needs to be a multiple of 8? Maybe?

    //bitField->encodePayload();

    ConstBufferPtr enc_mes = bitField->encode();
    int msg_len = bitField->getPayload()->size() + 5;
    const char* b_msg = reinterpret_cast<const char*>(enc_mes->buf());
   // cerr << "sending" << endl;
    if(send(peersock, b_msg, msg_len, 0) == -1)
    {
        perror("send");
    }
   // cerr <<"receining"<< endl;

    uint8_t hs_buf[1000] = {'\0'};
    ssize_t n_buf = 0;
    if((n_buf = recv(peersock, hs_buf, sizeof(hs_buf), 0)) == -1)
    //if(recv(peersock, pBitField, toSendSize+5, 0) == -1)
    {
      cerr <<"uh oh spahghethgti o" << endl;
        perror("receive");
    }
    //cerr << " hs_buf is " << (int)hs_buf[7] << " " << (int)hs_buf[4] << endl;

    //the good shit is in hs_buf[5:]
    for(int i = 5; i < n_buf; i++)
    {
        uint8_t cmpX;
        uint8_t toCmp;
        if(i == n_buf -1)
        {
             toCmp = hs_buf[i]; //the byte we're interested in
             for(int j = 0; j < bitsNeeded; j++)
             {
                 cmpX = toCmp & 0x80;
                 if(cmpX == 0x80)
                 {
                     peer.m_pieceIndex[(i-5)*8 + j] = true;
                 }
                 else
                 {
                     peer.m_pieceIndex[(i-5)*8 + j] = false;
                 }
                 toCmp = toCmp << 1;
             }
        }
        else
         {
             toCmp = hs_buf[i]; //the byte we're interested in
             for(int j = 0; j < 8; j++)
             {
                 cmpX = toCmp & 0x80;
                 if(cmpX == 0x80)
                 {
                     peer.m_pieceIndex[(i-5)*8 + j] = true;
                 }
                 else
                 {
                     peer.m_pieceIndex[(i-5)*8 + j] = false;
                 }
                 toCmp = toCmp << 1;
             }
        }
    }
    if(peer.updateInterest())
      getNextReq(peer);
    //cerr << "interested in peer? :" <<peer.m_amInterested<<endl;
    //ConstBufferPtr hs_res = make_shared<Buffer>(hs_buf, n_buf);

    //cout << peer.m_numPieces << endl;
    //for(int i = 0; i < peer.m_numPieces; i++)
    //cout << peer.m_pieceIndex[i] << endl;
}

void getNextReq(Peer &peer)
{
    //only call if you're sure it's interested
    //returns -1 if none found
    for(int i = 0; i < numOfPieces; i++)
    {
        nextStartReq = nextStartReq % numOfPieces;
        if(peer.m_pieceIndex[nextStartReq] == 1 && PIECESOBTAINED[nextStartReq] == 0)
        {
            int retVal = nextStartReq;
            nextStartReq++;
            nextStartReq = nextStartReq % numOfPieces;
            peer.m_desiredPiece = retVal;
            return;
        }
        else
        {
            nextStartReq++;
            nextStartReq = nextStartReq % numOfPieces;
        }
    }
    peer.m_desiredPiece = -1;
}


bool verifyPiece(const Piece& piece, vector<uint8_t> hash){
  //cerr << "enter verify" << endl;
  int offset = piece.getIndex() * 20;

 cerr << "offset is " << offset << endl;
 cerr << "vector size" << hash.size() << endl;

  //char* b_msg = reinterpret_cast<const char*>(enc_mes->buf());
 //vector<uint8_t> tempRec;
  cerr << "hash is ";
  for(int j = 0; j < 20; j++){
    cerr << int(hash[j+offset]) << " ";
  }
 cerr << endl;
 // vector<uint8_t> recHash = sha1(tempRec);
  ConstBufferPtr tempRec = sha1(piece.getBlock());
  const uint8_t* recHash = tempRec->buf();
  
  cerr << "rechashCBP is ";
  for (int i = 0; i < 20; i++) {
  cerr << int(recHash[i])<< " ";
  }
  cerr << endl;

  cerr << "get block is " << piece.getBlock()->get() << endl;


 for(int k = 0; k < 20; k++){
   if(hash[offset + k] != recHash[k])
    return false;
  }
  cerr << "after for" << endl;
  
  
return true;

}


void doAllTheThings(Client client){
  
    MetaInfo* metainfo = client.m_info;
    HttpRequest req;
    req.setHost(client.m_hostName);
    req.setPort(client.m_trackPort);
    req.setMethod(HttpRequest::GET);
    string left = to_string(metainfo->getLength());
    string path = client.m_path + "?info_hash=" + url::encode((const uint8_t *)(metainfo->getHash()->get()), 20) + "&peer_id=" + url::encode(client.m_peerid, 20) +
      "&port=" + client.getPort() + "&uploaded=0&downloaded=0&left=" + left + "&event=started";
    //cerr << path << std::endl;
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
    //cerr << path << std::endl; 
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
   
   cerr << std::endl << "Pieces Obtained: ";
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
    }
    // end of isFirst
   
    // Josh start 
    for (std::vector<Peer*>::iterator it = peerList.begin(); it != peerList.end(); it++)
    {
       cerr << "Peer: " << (*it)->m_peerId << std::endl;
       cerr << "m_amInterested = " << (*it)->m_amInterested << std::endl;
       if ((*it)->m_amInterested == true)
       {
         if ((*it)->m_buffSize == 0) // empty buffer
         {
           // send interested message
           Interested* in = new Interested();
           if ((*it)->sendMsg(in) == -1)
             perror("Error sending interest");
           cerr << "Interested Message sent" << std::endl;
           if ((*it)->recvMsg() == -1)
             perror("Error recv");
           cerr << "Received something!" << std::endl;
         }
         else // something in buffer
         {
           int msgType = (*it)->m_buff[4]; // 5th byte is id
           switch (msgType) 
           {
             case 1: // peer is unchoking us *phew*
             {
               cerr << "I've been unchoked!" << std::endl;
               // request the piece
               if ((int)((*it)->m_desiredPiece) <  (numOfPieces-1)) // not the last piece
               {
               cerr << "Trying to request piece index: " << (*it)->m_desiredPiece;
                 Request* rqst = new Request((*it)->m_desiredPiece, 0, 20 );
                 if ((*it)->sendMsgWPayload(rqst) == -1)
                   perror("Error sending request");
                 cerr << "Request sent!" << std::endl;
               }
               (*it)->resetBuff(); // reset buffer to use for next recv
               break;
             }
             case 2: // peer interested in us *yeah*
             {
                 Unchoke* unchoke = new Unchoke();
                 if ((*it)->sendMsg(unchoke) == -1)
                   perror("Error sending unchoke");
                 (*it)->resetBuff();
                 break;
             }
             case 4: // received have
             {
               ConstBufferPtr temp = make_shared<Buffer>((*it)->m_buff, (*it)->m_buffSize);
               Have hv;
               hv.decode(temp);
               if ((*it)->setInterest((int)hv.getIndex()))
                 getNextReq(**it);
               (*it)->resetBuff(); // reset buffer to use for next recv
               break;
             }
             case 6: // received a request
             {
               ConstBufferPtr temp = make_shared<Buffer>((*it)->m_buff, (*it)->m_buffSize);
               Request req;
               req.decode(temp); 
               Piece* pie = new Piece(req.getIndex(), req.getBegin(), req.getPayload());
               if ((*it)->sendMsgWPayload(pie) == -1)
                 perror("Error sending piece");
               cerr << "Piece sent!" << std::endl;
               (*it)->resetBuff();
               break;
             }
             case 7: // piece
               {
                 cerr << "Got a piece!" << std::endl;
                 // verify piece with hash
                 ConstBufferPtr temp = make_shared<Buffer>((*it)->m_buff, (*it)->m_buffSize);
                 Piece pie;
                 pie.decode(temp);
                 cerr << "payload size" << pie.getPayload()->size() << endl;
                 cerr << "Is this perhaps the right piece?" << std::endl;
                 if (verifyPiece(pie,client.m_info->getPieces()))
                 {
                    cerr << "Now I've got a GOLDEN TICKET!!!" << std::endl;
                    Have* hv = new Have(pie.getIndex());
                    updatePiecesArray((int)(hv->getIndex()));      
                    if ((*it)->updateInterest())
                      getNextReq(**it);
                    // send have to all the peers
                    for (std::vector<Peer*>::iterator it_ptr = peerList.begin(); it_ptr != peerList.end(); it_ptr++)
                    {
                      if ((*it_ptr)->sendMsgWPayload(hv) == -1)
                        perror("Error sending have");
                      cerr << "Tell ALL the peers!" << std::endl;
                    }
                 }
                 else  // resend request
                 {

                   cerr << "Trying to request piece index: " << (*it)->m_desiredPiece;
                   Request* rqst = new Request((*it)->m_desiredPiece, 0, 20);

                   //cerr << "Sry no ticket for you" << endl;
                   //Request* rqst = new Request((*it)->m_desiredPiece, 0, static_cast<uint32_t>(client.m_info->getLength()));

                   if ((*it)->sendMsgWPayload(rqst) == -1)
                     perror("Error sending request");
                 }
                 (*it)->resetBuff(); 
                 break;
               }
              default:
                cerr << "Received something else..." << std::endl;
           } // end switch
         } //end something in buffer
       }
       // not interested
       else
       {
         if ((*it)->m_buffSize == -1) // empty buffer
         {
           if ((*it)->recvMsg() == -1) 
             perror("recv");
         }
         else // something in buffer
         {
           int msgType = (*it)->m_buff[4];
           switch (msgType)
           {
             case 2: // peer interested
               {
                 Unchoke* unchoke = new Unchoke();
                 if ((*it)->sendMsg(unchoke) == -1)
                   perror("Error sending unchoke");
                 (*it)->resetBuff();
               break;
               }
             case 4: // received have
             {
               ConstBufferPtr temp = make_shared<Buffer>((*it)->m_buff, (*it)->m_buffSize);
               Have hv;
               hv.decode(temp);  
               if ((*it)->setInterest((int)hv.getIndex()))
                 getNextReq(**it);
               (*it)->resetBuff(); // reset buffer to use for next recv
               break;
             }
             case 6: // request
             {
               ConstBufferPtr temp = make_shared<Buffer>((*it)->m_buff, (*it)->m_buffSize);
               Request req;
               req.decode(temp);
               Piece* pie = new Piece(req.getIndex(), req.getBegin(), req.getPayload());
               if ((*it)->sendMsgWPayload(pie) == -1)
                 perror("Error sending piece");
               cerr << "Piece sent!" << std::endl;
               (*it)->resetBuff();
               break;
             } 
             default:
               cerr << "Received something else..." << std::endl;
           } // end switch
         } // end something in buffer
       } // end uninterest
    } // end loop
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
    cerr << "ready, okay! " << std::endl;

    doAllTheThings(client);

  }
  catch (std::exception& e)
  {
    std::cerr << "exception: " << e.what() << "\n";
  }

  return 0;
}
