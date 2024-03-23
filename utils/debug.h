#ifndef PDW_DEBUG_H
# define PDW_DEBUG_H


#if defined _DEBUG || defined DBG
# define OUTPUTDEBUGMSG(dprintf_exp)	((void)(DebugPrintf dprintf_exp))
# define ASSERTDEBUGMSG(cond, dprintf_exp) ((void)((cond) ? 0:(DebugPrintf dprintf_exp)))
# define DEBUGASSERT(dprintf_exp)	((void)(Debugv dprintf_exp));
#else
# define OUTPUTDEBUGMSG(dprintf_exp)
# define ASSERTDEBUGMSG(cond, dprintf_exp)
# define DEBUGASSERT(dprintf_exp)
#endif // DEBUG


#ifdef __cplusplus
extern "C" {
#endif

extern int	nDebugOutput;
extern int	nDebugNum;

extern int DebugPrintf(char *fmt,...) ;

#ifdef __cplusplus
}
#endif


#endif
