#ifdef WIN32
#define SII_INLINE _inline
#else
#ifndef SII_INLINE
#  if defined(__GNUC__)
#    define SII_INLINE __inline__
#  else
#    define SII_INLINE inline
#  endif 
#endif 
#endif 
