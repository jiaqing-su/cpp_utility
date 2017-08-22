#ifndef _LOG_H_
#define _LOG_H_

#define LOG_OPT_LEVEL     0x01
#define LOG_OPT_DATE      0x02
#define LOG_OPT_TIME      0x04
#define LOG_OPT_FILELINE  0x08
#define LOG_OPT_FUNCTION  0x10
#define LOG_OPT_PID_TID   0x20

#define LOG_OUT_CONSOLE   0x01
#define LOG_OUT_FILE      0x02
#define LOG_OUT_VS_DEBUG  0x04

#define LOG_LEVEL_FATAL   0
#define LOG_LEVEL_ERROR   1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_INFO    3
#define LOG_LEVEL_DEBUG   4
#define LOG_LEVEL_TRACE   5

#ifdef __cplusplus
extern "C"{
#endif /*__cplusplus*/

void set_log_opt_mask(unsigned int mask);/* LOG_OPT_* */
void set_log_level(int lv);/* LOG_LEVEL_* */
void set_log_out_mask(unsigned int mask);/* LOG_OUT_* */
void set_log_file_path(const char* path);
void set_log_file_name(const char* name);

void log_print(int level, const char* file, int line, const char* func, const char* format, ...);

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#define log_out_(level, format, ...)\
	log_print(level, __FILE__, __LINE__, __FUNCTION__, format,  ##__VA_ARGS__)

#define log_fatal(format,...)\
	log_out_(LOG_LEVEL_FATAL, format, ##__VA_ARGS__);

#define log_error(format,...)\
	log_out_(LOG_LEVEL_ERROR, format, ##__VA_ARGS__);

#define log_warning(format,...)\
	log_out_(LOG_LEVEL_WARNING, format, ##__VA_ARGS__);

#define log_info(format,...)\
	log_out_(LOG_LEVEL_INFO, format, ##__VA_ARGS__);

#define log_debug(format,...)\
	log_out_(LOG_LEVEL_DEBUG, format, ##__VA_ARGS__);

#define log_trace(format,...)\
	log_out_(LOG_LEVEL_TRACE, format, ##__VA_ARGS__);

#if defined(DEBUG)||defined(_DEBUG)
#define LOG_FATAL log_fatal
#define LOG_ERROR log_error
#define LOG_WARNING log_warning
#define LOG_INFO log_info
#define LOG_DEBUG log_debug
#define LOG_TRACE log_trace
#else
#define LOG_FATAL
#define LOG_ERROR
#define LOG_WARNING
#define LOG_INFO
#define LOG_DEBUG
#define LOG_TRACE
#endif

#endif/*_LOG_H_*/
