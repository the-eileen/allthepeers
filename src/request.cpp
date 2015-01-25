/* might end up putting these functions 
somewhere else but I'll put my part 
here for now */
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

void makeGetRequest(sbt::Client client){
	MetaInfo metainfo = CLient.m_info;
	HttpRequest req;
    req.setHost(client.m_url);
    req.setPort(client.m_trackPort);
    req.setMethod(HttpRequest::GET);
    string left = to_string(metainfo.length());
    string path = "/announce?info_hash=" + url::encode(metainfo.getHash(), 20) + "&peer_id=" + url::encode(client.m_peerid, 20) +
    	"&port=" + client.getPort() + "&uploaded=0&downloaded=0&left=" + left + "&event=started";
    req.setPath(path);
    req.setVersion("1.1");
    //req.addHeader("Accept-Language", "en-US");
    size_t reqLen = req.getTotalLength();
    char *buf = new char [reqLen];
    req.formatRequest(buf);

    //return buf;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bool isEnd = false;
  //std::string input;
  char rbuf[20] = {0};
  std::stringstream ss;

  while (!isEnd) {
    //memset(buf, '\0', sizeof(buf));

    //std::cin >> input;
    if (send(sockfd, buf, sizeof(buf), 0) == -1) {
      perror("send");
      //return 4;
    }


    if (recv(sockfd, rbuf, sizeof(rbuf), 0) == -1) {
      perror("recv");
      //return 5;
    }
    ss << buf << std::endl;

    if (ss.str() == "close\n")
      break;

    ss.str("");
  }

  close(sockfd);
}
