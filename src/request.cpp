/* might end up putting these functions 
somewhere else but I'll put my part 
here for now */

#include "meta-info.hpp"
#include "http-request.hpp"
#include <string>
#include <cstring>
	using namespace std;

string makeGetRequest(Client client){
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



}