//
// Copyright (c) 2019-2022 yanggaofeng
//

#ifndef INCLUDE_YANGUTIL_SYS_YANGTHREAD_H_
#define INCLUDE_YANGUTIL_SYS_YANGTHREAD_H_
#include <yangutil/yangtype.h>
#if Yang_Enable_Phtread
#include <pthread.h>
#define yang_thread_create pthread_create
#define yang_thread_t pthread_t
#define yang_thread_mutex_t pthread_mutex_t
#define yang_thread_cond_t pthread_cond_t
#define yang_thread_join pthread_join
#define yang_thread_exit pthread_exit
#define yang_thread_detach pthread_detach
#define yang_thread_equal pthread_equal
#define yang_thread_mutex_lock pthread_mutex_lock
#define yang_thread_mutex_unlock pthread_mutex_unlock
#define yang_thread_cond_signal pthread_cond_signal
#define yang_thread_cond_timedwait pthread_cond_timedwait
#define yang_thread_cond_wait pthread_cond_wait
#define yang_thread_mutex_init pthread_mutex_init
#define yang_thread_mutex_destroy pthread_mutex_destroy
#define yang_thread_cond_init pthread_cond_init
#define yang_thread_cond_destroy pthread_cond_destroy
#endif

#endif /* INCLUDE_YANGUTIL_SYS_YANGTHREAD_H_ */
