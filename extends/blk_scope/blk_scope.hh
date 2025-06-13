#ifndef _BLK_SCOPE_H_
#define _BLK_SCOPE_H_
#ifdef __cplusplus
extern "C" {
#endif
int rtapi_app_main();
void rtapi_app_exit();
void catch_cycle(void * arg, long period);
#ifdef __cplusplus
}
#endif
#endif