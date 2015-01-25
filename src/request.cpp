/* might end up putting these functions 
somewhere else but I'll put my part 
here for now */

#include "meta-info.hpp"
#include "http-request.hpp"
#include <string>
	using namespace std;

string makeGetRequest(Client client){
	MetaInfo metainfo = CLient.m_info;
	HttpRequest req;
    req.setHost();
    req.setPort(80);
    req.setMethod(HttpRequest::GET);
    req.setPath("/");
    req.setVersion("1.1");
    req.addHeader("Accept-Language", "en-US");
    size_t reqLen = req.getTotalLength();
    char *buf = new char [reqLen];
    req.formatRequest(buf);



}