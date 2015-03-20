#ifndef MALLOC_280_H
#define MALLOC_280_H

#ifdef __cplusplus
extern "C" {
#endif
void* mtk_ext_malloc(size_t bytes);
void mtk_ext_free(void* mem);
#ifdef __cplusplus
};  /* end of extern "C" */
#endif

#endif
