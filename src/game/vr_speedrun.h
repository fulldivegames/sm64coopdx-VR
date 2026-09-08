#ifndef VR_SPEEDRUN_H
#define VR_SPEEDRUN_H
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
double vr_speedrun_now(void);
void vr_speedrun_initialize(const char* setupFile, unsigned int segments);
double vr_speedrun_best(void);
double vr_speedrun_elapsed(double now);
void vr_speedrun_reset(void);
void vr_speedrun_split(double now);
void vr_speedrun_pause(double now);
void vr_speedrun_basic(unsigned int segments);
void vr_speedrun_configure(unsigned int segments);
bool vr_speedrun_set_name(unsigned int index, const char* name);
const char* vr_speedrun_segment_name(unsigned int index);
double vr_speedrun_segment_time(unsigned int index);
unsigned int vr_speedrun_count(void);
unsigned int vr_speedrun_total(void);
const char* vr_speedrun_name(void);
bool vr_speedrun_running(void);
bool vr_speedrun_load(const char* filename);
bool vr_speedrun_save(const char* filename);
bool vr_speedrun_save_local(void);
bool vr_speedrun_import_lss(const char* filename);
const char* vr_speedrun_status(void);
#ifdef __cplusplus
}
#endif
#endif
