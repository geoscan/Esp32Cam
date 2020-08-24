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
%}

%union {
	int   uival;
	float fval;
}

%token <uival> UINT
%token <fval> FLOAT

%token CSEQ
%token CLIENT_PORT
%token SESSION
%token UDP

%token DESCRIBE    
%token SETUP       
%token TEARDOWN    
%token PLAY        
%token PAUSE       


%%


head:
	request_type text
;


request_type:
	DESCRIBE {req->requestType = Rtsp::RequestType::Describe;  }
|	SETUP    {req->requestType = Rtsp::RequestType::Setup;     }
|	TEARDOWN {req->requestType = Rtsp::RequestType::Teardown;  }
|	PLAY     {req->requestType = Rtsp::RequestType::Play;      }
|	PAUSE    {req->requestType = Rtsp::RequestType::Pause;     }
|	         {req->requestType = Rtsp::RequestType::NotStated; }
;


text:
	word
|	text word	
|	text cseq
|	text transport
|	text session
;

 
word:
	UINT
|	FLOAT
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



// --------------------------- Public --------------------------- //



void parse(Rtsp::Request &request, Rtsp::Data data)
{
	#if PARSER_DEBUG != 1
	mutex.lock();
	#endif // PARSER_DEBUG != 1

	debug("Parse started");
	req = &request;
	yyin = nullptr;
	auto bufState = yy_scan_bytes(reinterpret_cast<char *>(data.data), data.len);
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