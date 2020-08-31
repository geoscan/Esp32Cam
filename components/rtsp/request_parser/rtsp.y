%{
	#include "Types.hpp"
	#include "lex.yy.h"
	#include "parser_debug.hpp"

	extern int yylex();
	extern int yyparse();
	void yyerror(const char *);

	static Rtsp::Request *req; // Not null, guaranteed

	#if PARSER_DEBUG != 1
	#include "asio/detail/mutex.hpp"
	static asio::detail::mutex mutex;
	#endif // PARSER_DEBUG == 1

	#include <string>
	#include <map>
%}

%union {
	int   uival;
	float fval;
	char  *sval;
}

%token <uival> UINT
%token <fval> FLOAT
%token <sval> RTSP

%token <sval> RTSP_HOST
%token <sval> RTSP_RESOURCE 
%token <uival> RTSP_PORT

%token CSEQ
%token CLIENT_PORT
%token SESSION
%token UDP

%token DESCRIBE    
%token SETUP       
%token TEARDOWN    
%token PLAY        
%token PAUSE
%token OPTION

%%


head:
	request_type {debug("req. type");} text
;


request_type:
	DESCRIBE {req->requestType = Rtsp::RequestType::Describe;  }
|	SETUP    {req->requestType = Rtsp::RequestType::Setup;     }
|	TEARDOWN {req->requestType = Rtsp::RequestType::Teardown;  }
|	PLAY     {req->requestType = Rtsp::RequestType::Play;      }
|	PAUSE    {req->requestType = Rtsp::RequestType::Pause;     }
|	OPTION   {req->requestType = Rtsp::RequestType::Option;    }
|	         {req->requestType = Rtsp::RequestType::NotStated; }
;


text:
	word
|	text word
;


word:
	token_word
|	rtsp_url
|	cseq
|	transport
|	session
;


token_word:
	UINT
|	FLOAT
;


rtsp_url:
	RTSP_HOST rtsp_port RTSP_RESOURCE {
		debug("RTSP <host> [port] <resource>");
		debug($1);
		debug($3);

		req->hostaddr = std::string($1);
		req->hostResource = std::string($3);
		
		free($1);
		free($3);
	}
;
rtsp_port:
|	RTSP_PORT
	{
		debug("RTSP port");
		debug($1);
		req->hostport = $1;
	}
;


cseq: 
	CSEQ UINT
	{
		debug("GOT: CSeq");
		debug($2);
		req->cseq = $2;
	}
;


transport:
	CLIENT_PORT UINT
	{
		debug("GOT: client_port");
		debug($2);
		req->clientPort = $2;
	}
|	UDP
	{
		debug("GOT: Transport: ... UDP");
		req->udp = true;
	}
;


session:
	SESSION UINT
	{
		debug("GOT: Session");
		debug($2);
		req->session = $2;
	}


%%




void parse(Rtsp::Request &request, const void *data, const size_t len)
{
	#if PARSER_DEBUG != 1
	mutex.lock();
	#endif // PARSER_DEBUG != 1

	debug("Parse started");
	req = &request;
	yyin = nullptr;
	//auto bufState = yy_scan_bytes(reinterpret_cast<const char *>(data.data), data.len);
	auto bufState = yy_scan_bytes(reinterpret_cast<const char *>(data), len);
	yyparse();
	yy_delete_buffer(bufState);

	#if PARSER_DEBUG != 1
	mutex.unlock();
	#endif // PARSER_DEBUG != 1
}

#if DEBUG_FLEX != 1

void yyerror(const char *s)
{
	debug("ERROR:");
	debug(s);
}

#endif