#include "RtspParser.hpp"

bool RtspParser::parse(Rtsp::Data data, Rtsp::Request &request)
{
	char CmdName[RTSP_PARAM_STRING_MAX];
	static char CurRequest[RTSP_BUFFER_SIZE]; // Note: we assume single threaded, this large buf we keep off of the tiny stack
	unsigned CurRequestSize;

	Init();
	CurRequestSize = aRequestSize;
	memcpy(CurRequest,aRequest,aRequestSize);

	// check whether the request contains information about the RTP/RTCP UDP client ports (SETUP command)
	char * ClientPortPtr;
	char * TmpPtr;
	static char CP[1024];
	char * pCP;

	ClientPortPtr = strstr(CurRequest,"client_port");
	if (ClientPortPtr != nullptr)
	{
		TmpPtr = strstr(ClientPortPtr,"\r\n");
		if (TmpPtr != nullptr)
		{
			TmpPtr[0] = 0x00;
			strcpy(CP,ClientPortPtr);
			pCP = strstr(CP,"=");
			if (pCP != nullptr)
			{
				pCP++;
				strcpy(CP,pCP);
				pCP = strstr(CP,"-");
				if (pCP != nullptr)
				{
					pCP[0] = 0x00;
					m_ClientRTPPort  = atoi(CP);
					m_ClientRTCPPort = m_ClientRTPPort + 1;
				};
			};
		};
	};

	// Read everything up to the first space as the command name
	bool parseSucceeded = false;
	unsigned i;
	for (i = 0; i < sizeof(CmdName)-1 && i < CurRequestSize; ++i)
	{
		char c = CurRequest[i];
		if (c == ' ' || c == '\t')
		{
			parseSucceeded = true;
			break;
		}
		CmdName[i] = c;
	}
	CmdName[i] = '\0';
	if (!parseSucceeded) {
		printf("failed to parse RTSP\n");
		return false;
	}

	printf("RTSP received %s\n", CmdName);

	// find out the command type
	if (strstr(CmdName,"OPTIONS")   != nullptr) m_RtspCmdType = RTSP_OPTIONS; else
	if (strstr(CmdName,"DESCRIBE")  != nullptr) m_RtspCmdType = RTSP_DESCRIBE; else
	if (strstr(CmdName,"SETUP")     != nullptr) m_RtspCmdType = RTSP_SETUP; else
	if (strstr(CmdName,"PLAY")      != nullptr) m_RtspCmdType = RTSP_PLAY; else
	if (strstr(CmdName,"TEARDOWN")  != nullptr) m_RtspCmdType = RTSP_TEARDOWN;

	// check whether the request contains transport information (UDP or TCP)
	if (m_RtspCmdType == RTSP_SETUP)
	{
		TmpPtr = strstr(CurRequest,"RTP/AVP/TCP");
		if (TmpPtr != nullptr) m_TcpTransport = true; else m_TcpTransport = false;
	};

	// Skip over the prefix of any "rtsp://" or "rtsp:/" URL that follows:
	unsigned j = i+1;
	while (j < CurRequestSize && (CurRequest[j] == ' ' || CurRequest[j] == '\t')) ++j; // skip over any additional white space
	for (; (int)j < (int)(CurRequestSize-8); ++j)
	{
		if ((CurRequest[j]   == 'r' || CurRequest[j]   == 'R')   &&
			(CurRequest[j+1] == 't' || CurRequest[j+1] == 'T') &&
			(CurRequest[j+2] == 's' || CurRequest[j+2] == 'S') &&
			(CurRequest[j+3] == 'p' || CurRequest[j+3] == 'P') &&
			CurRequest[j+4] == ':' && CurRequest[j+5] == '/')
		{
			j += 6;
			if (CurRequest[j] == '/')
			{   // This is a "rtsp://" URL; skip over the host:port part that follows:
				++j;
				unsigned uidx = 0;
				while (j < CurRequestSize && CurRequest[j] != '/' && CurRequest[j] != ' ' && uidx < sizeof(m_URLHostPort) - 1)
				{   // extract the host:port part of the URL here
					m_URLHostPort[uidx] = CurRequest[j];
					uidx++;
					++j;
				};
			}
			else --j;
			i = j;
			break;
		}
	}

	// Look for the URL suffix (before the following "RTSP/"):
	parseSucceeded = false;
	for (unsigned k = i+1; (int)k < (int)(CurRequestSize-5); ++k)
	{
		if (CurRequest[k]   == 'R'   && CurRequest[k+1] == 'T'   &&
			CurRequest[k+2] == 'S'   && CurRequest[k+3] == 'P'   &&
			CurRequest[k+4] == '/')
		{
			while (--k >= i && CurRequest[k] == ' ') {}
			unsigned k1 = k;
			while (k1 > i && CurRequest[k1] != '/') --k1;
			if (k - k1 + 1 > sizeof(m_URLSuffix)) return false;
			unsigned n = 0, k2 = k1+1;

			while (k2 <= k) m_URLSuffix[n++] = CurRequest[k2++];
			m_URLSuffix[n] = '\0';

			if (k1 - i > sizeof(m_URLPreSuffix)) return false;
			n = 0; k2 = i + 1;
			while (k2 <= k1 - 1) m_URLPreSuffix[n++] = CurRequest[k2++];
			m_URLPreSuffix[n] = '\0';
			i = k + 7;
			parseSucceeded = true;
			break;
		}
	}
	if (!parseSucceeded) return false;

	// Look for "CSeq:", skip whitespace, then read everything up to the next \r or \n as 'CSeq':
	parseSucceeded = false;
	for (j = i; (int)j < (int)(CurRequestSize-5); ++j)
	{
		if (CurRequest[j]   == 'C' && CurRequest[j+1] == 'S' &&
			CurRequest[j+2] == 'e' && CurRequest[j+3] == 'q' &&
			CurRequest[j+4] == ':')
		{
			j += 5;
			while (j < CurRequestSize && (CurRequest[j] ==  ' ' || CurRequest[j] == '\t')) ++j;
			unsigned n;
			for (n = 0; n < sizeof(m_CSeq)-1 && j < CurRequestSize; ++n,++j)
			{
				char c = CurRequest[j];
				if (c == '\r' || c == '\n')
				{
					parseSucceeded = true;
					break;
				}
				m_CSeq[n] = c;
			}
			m_CSeq[n] = '\0';
			break;
		}
	}
	if (!parseSucceeded) return false;

	// Also: Look for "Content-Length:" (optional)
	for (j = i; (int)j < (int)(CurRequestSize-15); ++j)
	{
		if (CurRequest[j]    == 'C'  && CurRequest[j+1]  == 'o'  &&
			CurRequest[j+2]  == 'n'  && CurRequest[j+3]  == 't'  &&
			CurRequest[j+4]  == 'e'  && CurRequest[j+5]  == 'n'  &&
			CurRequest[j+6]  == 't'  && CurRequest[j+7]  == '-'  &&
			(CurRequest[j+8] == 'L' || CurRequest[j+8]   == 'l') &&
			CurRequest[j+9]  == 'e'  && CurRequest[j+10] == 'n' &&
			CurRequest[j+11] == 'g' && CurRequest[j+12]  == 't' &&
			CurRequest[j+13] == 'h' && CurRequest[j+14] == ':')
		{
			j += 15;
			while (j < CurRequestSize && (CurRequest[j] ==  ' ' || CurRequest[j] == '\t')) ++j;
			unsigned num;
			if (sscanf(&CurRequest[j], "%u", &num) == 1) m_ContentLength = num;
		}
	}
	return true;
}