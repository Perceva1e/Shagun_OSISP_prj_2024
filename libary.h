#ifndef LIBARY_H_
#define LIBARY_H_

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <unistd.h>
#include <dirent.h>
#include <signal.h> 

#include <stdlib.h>
#include <stdio.h>
#include <cstring>
#include <X11/Xutil.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <dirent.h>

#include <limits.h>

#include <getopt.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <linux/stat.h>
#include <sys/sysmacros.h>
#include <locale.h>
#include <chrono>
#include <thread>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

using namespace std;
#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 512

#define PROC_DIRECTORY "/proc"
#define MAX_LINE_LENGTH 256

bool windwow(Display *display, Window root_window,const char *texts);
char* execute_command(const char* command);
void find_camera_process_pid_and_write_to_file(const char* file_path);
int read_pid_from_file(const char *file_path);
void writePIDToFile(int pid);
bool listMicrophoneProcesses();
bool compareFiles(); 
void findAndKillMicrophoneProcesses();
void deleteExtraFiles(const char *file1, const char *file2);
int locale_strcmp(const struct dirent **a, const struct dirent **b);
void compareAndUpdateFiles(const char *sourceFilePath, const char *targetFilePath);
void dirwalkFile(char *path, bool l_opt, bool d_opt, bool f_opt, bool s_opt, FILE *outputFile);

#endif // LIBRARY_H_