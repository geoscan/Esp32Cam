#ifndef COMPONENTS_HTTP_HTTP_H
#define COMPONENTS_HTTP_HTTP_H

#ifdef __cplusplus
extern "C" {
#endif

void httpStart(void);
void httpTest(void);
extern const char *kHttpDebugTag;

inline const char *httpDebugTag()
{
	return kHttpDebugTag;
}

#ifdef __cplusplus
}
#endif


#endif // COMPONENTS_HTTP_HTTP_H
