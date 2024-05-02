#include "libary.h"

bool windwow(Display *display, Window root_window,const char *texts) {
    int screen = DefaultScreen(display);
    Window window = XCreateSimpleWindow(display, root_window, 0, 0, 300, 200, 1,
                                        BlackPixel(display, screen), WhitePixel(display, screen));
    XSelectInput(display, window, ExposureMask | ButtonPressMask);
    XStoreName(display, window, "Alarm");
    XFontStruct *font_info;
    font_info = XLoadQueryFont(display, "fixed");
    if (!font_info) {
    fprintf(stderr, "Failed to load font\n");
    exit(1);
    }
    GC gc = XCreateGC(display, window, 0, 0);
    XSetFont(display, gc, font_info->fid);  
    const char *text = texts;
    XDrawString(display, window, gc, 50, 50, text, strlen(text));
    const char *yes_label = "Yes";
    const char *no_label = "No";
    int button_width = 60;
    int button_height = 40;
    int button_x = 50;
    int button_y = 100;
    XDrawRectangle(display, window, gc, button_x, button_y, button_width, button_height);
    XDrawString(display, window, gc, button_x + 10, button_y + 25, yes_label, strlen(yes_label));
    XDrawRectangle(display, window, gc, button_x + 100, button_y, button_width, button_height);
    XDrawString(display, window, gc, button_x + 110, button_y + 25, no_label, strlen(no_label));
    XMapWindow(display, window);
    XEvent event;
    while (1) {
        XNextEvent(display, &event);
        switch (event.type) {
            case Expose:
                XClearWindow(display, window);
                XDrawString(display, window, gc, 50, 50, text, strlen(text));
                XDrawRectangle(display, window, gc, button_x, button_y, button_width, button_height);
                XDrawRectangle(display, window, gc, button_x + 100, button_y, button_width, button_height);
                XDrawString(display, window, gc, button_x + 10, button_y + 25, yes_label, strlen(yes_label));
                XDrawString(display, window, gc, button_x + 110, button_y + 25, no_label, strlen(no_label));
                break;
            case ButtonPress:
                if (event.xbutton.button == Button1) {
                    if (event.xbutton.x >= button_x && event.xbutton.x <= button_x + button_width &&
                        event.xbutton.y >= button_y && event.xbutton.y <= button_y + button_height) {
                        printf("Да\n");
                        XCloseDisplay(display);
                        return(true);
                    } else if (event.xbutton.x >= button_x + 100 && event.xbutton.x <= button_x + 100 + button_width &&
                               event.xbutton.y >= button_y && event.xbutton.y <= button_y + button_height) {
                        printf("Нет\n");
                        XCloseDisplay(display);
                        return(false);
                    }
                }
                break;
            default:
                break;
        }
    }
}
char* execute_command(const char* command) {
    FILE* fp = popen(command, "r");
    if (fp == NULL) {
        return NULL;
    }
    char buffer[1024];
    size_t result_size = 1024; 
    char* result = (char*)malloc(sizeof(char) * result_size);
    if (result == NULL) {
        fclose(fp);
        return NULL;
    }
    strcpy(result, "");
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        size_t current_length = strlen(result);
        size_t buffer_length = strlen(buffer);
        if (current_length + buffer_length < result_size) {
            strcat(result, buffer);
        } else {
            result_size *= 2;
            char* temp = (char*)realloc(result, sizeof(char) * result_size);
            if (temp == NULL) {
                fclose(fp);
                free(result);
                return NULL;
            }
            result = temp;
            strcat(result, buffer);
        }
    }
    pclose(fp);
    return result;
}
void find_camera_process_pid_and_write_to_file(const char* file_path) {
    const char* command = "lsof /dev/video0";
    char* output = execute_command(command);
    if (output == NULL) {
        printf("Error executing command\n");
        return;
    }
    char* pid_str = strtok(output, " ");
    while (pid_str != NULL) {
        int pid = atoi(pid_str);
        if (pid > 0) {
            FILE* file = fopen(file_path, "w");
            if (file != NULL) {
                if (fprintf(file, "%d", pid) < 0) {
                    printf("Error writing PID to file\n");
                } else {
                    printf("PID successfully written to file\n");
                }
                fclose(file);
                free(output);
                return;
            } else {
                printf("Failed to open file for writing\n");
                free(output);
                return;
            }
        }
        pid_str = strtok(NULL, " ");
    }
    free(output);
    printf("Failed to find a process using the camera\n");
}
int read_pid_from_file(const char *file_path) {
    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        printf("Ошибка открытия файла.\n");
        return -1;
    }
    int pid;
    if (fscanf(file, "%d", &pid) != 1) {
        printf("Ошибка чтения PID из файла.\n");
        fclose(file);
        return -1;
    }
    fclose(file);
    return pid;
}
void writePIDToFile(int pid) {
    FILE *file = fopen("microphone_processes.txt", "r+");
    if (file == NULL) {
        perror("Ошибка при открытии файла для записи PID");
        exit(1);
    }
    fprintf(file, "%d\n", pid);
    fclose(file);
}
bool listMicrophoneProcesses() {
    DIR *dir;
    struct dirent *entry;
    char path[MAX_LINE_LENGTH];
    char line[MAX_LINE_LENGTH];
    int hasMicrophoneProcesses = 0; 
    dir = opendir(PROC_DIRECTORY);
    if (dir == NULL) {
        perror("Ошибка при открытии каталога /proc");
        exit(1);
    }
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && atoi(entry->d_name) != 0) {
            snprintf(path, sizeof(path), "%s/%s/status", PROC_DIRECTORY, entry->d_name);
            FILE *fp = fopen(path, "r");
            if (fp != NULL) {
                while (fgets(line, sizeof(line), fp) != NULL) {
                    if (strncmp(line, "Name:", 5) == 0) {
                        if (strstr(line, "audio") != NULL || strstr(line, "sound") != NULL) {
                            printf("Процесс с PID %s использует микрофон.\n", entry->d_name);
                            hasMicrophoneProcesses = 1;
                            writePIDToFile(atoi(entry->d_name));
                        }
                        break;
                    }
                }
                fclose(fp);
            }
        }
    }
    closedir(dir);
    if (!hasMicrophoneProcesses) {
        printf("Не найдено процессов, использующих микрофон.\n");
        return false;
    }
    return true;
}
bool compareFiles() {
    FILE *file1 = fopen("microphone_processes.txt", "r");
    FILE *file2 = fopen("previous_microphone_processes.txt", "r");
    if (file1 == NULL || file2 == NULL) {
        perror("Ошибка при открытии файлов");
        exit(1);
    }
    char line1[MAX_LINE_LENGTH];
    char line2[MAX_LINE_LENGTH];
    bool mismatch = false;
    while (fgets(line1, sizeof(line1), file1) != NULL) {
        bool found = false;
        rewind(file2); 
        while (fgets(line2, sizeof(line2), file2) != NULL) {
            if (strcmp(line1, line2) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            printf("Найден лишний процесс с PID: %s\n", line1);
            mismatch = true;
        }
    }
    fclose(file1);
    fclose(file2);
    return mismatch;
}
void findAndKillMicrophoneProcesses() {
    Display *displayAudio = XOpenDisplay(":0");
    if (displayAudio == NULL) {
        fprintf(stderr, "Unable to open display.\n");
        return;
    }
    int screenAudio = DefaultScreen(displayAudio);
    Window root_window_audio = RootWindow(displayAudio, screenAudio);
    bool check = false;
    const char *texts = "Microfone is work.It is your activitity";
    if (listMicrophoneProcesses()) {
        check = compareFiles();
    }
    if(check)
    {   
       check = !(windwow(displayAudio,root_window_audio,texts));
    }
    if (check) {   
        FILE *file = fopen("microphone_processes.txt", "r");
        if (file == NULL) {
            perror("Ошибка при открытии файла с PID");
            exit(1);
        }
        char line[MAX_LINE_LENGTH];
        FILE *tempFile = fopen("temp_microphone_processes.txt", "w");
        if (tempFile == NULL) {
            perror("Ошибка при создании временного файла");
            exit(1);
        }
        while (fgets(line, sizeof(line), file) != NULL) {
            int pid = atoi(line);
            printf("Завершение процесса с PID: %d\n", pid);
            if (kill(pid, SIGTERM) == 0) {
                printf("Процесс успешно завершен.\n");
            } else {
                perror("Ошибка при завершении процесса");
            }
        }
        fclose(file);
        fclose(tempFile);
        if (rename("temp_microphone_processes.txt", "microphone_processes.txt") != 0) {
            perror("Ошибка при замене файла");
            exit(1);
        }
        else {
        printf("Нет лишних процессов для завершения.\n");
        }
    }
    system("cp microphone_processes.txt previous_microphone_processes.txt");
}
void deleteExtraFiles(const char *file1, const char *file2) {
    FILE *fp1, *fp2;
    char path1[1000], path2[1000];
    fp1 = fopen(file1, "r+");
    fp2 = fopen(file2, "r");
    if (fp1 == NULL || fp2 == NULL) {
        printf("Ошибка при открытии файлов.\n");
        return;
    }
    while (fgets(path1, sizeof(path1), fp1) != NULL) {
        path1[strcspn(path1, "\n")] = 0;
        int found = 0;
        rewind(fp2);
        while (fgets(path2, sizeof(path2), fp2) != NULL) {
            path2[strcspn(path2, "\n")] = 0;
            if (strcmp(path1, path2) == 0) {
                found = 1;
                break;
            }
        }
        if (!found) {
            if (remove(path1) == 0) {
                printf("Файл или директория %s удалены.\n", path1);
                rewind(fp1);
                char line[1000];
                FILE *temp = fopen("temp.txt", "w");
                while (fgets(line, sizeof(line), fp1) != NULL) {
                    line[strcspn(line, "\n")] = 0;
                    if (strcmp(line, path1) != 0) {
                        fprintf(temp, "%s\n", line);
                    }
                }
                fclose(temp);
                fclose(fp1);
                remove(file1);
                rename("temp.txt", file1);
                fp1 = fopen(file1, "r+");
            } else {
                printf("Ошибка при удалении файла или директории %s.\n", path1);
                if (access(path1, F_OK) == -1) {
                    printf("Проверка доступности файла или директории: файл или директория не существует.\n");
                }
                printf("Дополнительная информация об ошибке: %s\n", strerror(errno));
            }
        }
    }
    fclose(fp1);
    fclose(fp2);
}
int locale_strcmp(const struct dirent **a, const struct dirent **b) {
    return strcoll((*a)->d_name, (*b)->d_name);
}
void compareAndUpdateFiles(const char *sourceFilePath, const char *targetFilePath) {
    Display *displayFile = XOpenDisplay(":0");
    if (displayFile == NULL) {
        fprintf(stderr, "Unable to open display.\n");
        return;
    }
    int screenFile = DefaultScreen(displayFile);
    Window root_window_file = RootWindow(displayFile, screenFile);
    const char *texts = "Find suspicious files.It is your activitity";
    FILE *sourceFile = fopen(sourceFilePath, "r");
    FILE *targetFile = fopen(targetFilePath, "r+");
    if (sourceFile == NULL || targetFile == NULL) {
        perror("Error opening files");
        return;
    }
    fseek(sourceFile, 0L, SEEK_END);
    long sourceFileSize = ftell(sourceFile);
    rewind(sourceFile);
    fseek(targetFile, 0L, SEEK_END);
    long targetFileSize = ftell(targetFile);
    rewind(targetFile);
    if (sourceFileSize != targetFileSize) {
        char *buffer = (char *)malloc(sourceFileSize + 1);
        fread(buffer, 1, sourceFileSize, sourceFile);
        buffer[sourceFileSize] = '\0';
        fseek(targetFile, 0, SEEK_SET);
        if(windwow(displayFile,root_window_file,texts)==false)
        {
            deleteExtraFiles(sourceFilePath,targetFilePath);
            printf("Suspicious files are deleted\n");
            return;
        }
        fwrite(buffer, 1, sourceFileSize, targetFile);
        printf("Files are different.Updated target file: %s\n", targetFilePath);
        free(buffer);
        fclose(sourceFile);
        fclose(targetFile);
        return;
    } else {
        char sourceChar, targetChar;
        bool filesMatch = true;
        while ((sourceChar = fgetc(sourceFile)) != EOF && (targetChar = fgetc(targetFile)) != EOF) {
            if (sourceChar != targetChar) {
                filesMatch = false;
                break;
            }
        }
        if (!filesMatch) {
            fseek(sourceFile, 0, SEEK_SET);
            fseek(targetFile, 0, SEEK_SET);
            if(windwow(displayFile,root_window_file,texts)==false)
            {
            deleteExtraFiles(sourceFilePath,targetFilePath);
              printf("Suspicious files are deleted\n");
            return;
            }
            int ch;
            while ((ch = fgetc(sourceFile)) != EOF) {
                fputc(ch, targetFile);
            }
            printf("Files are different. Updated target file: %s\n", targetFilePath);
            fclose(sourceFile);
            fclose(targetFile);
            return;
        } else {
            printf("Files are identical. No update needed.\n");
            fclose(sourceFile);
            fclose(targetFile);
            return;
        }
    }
}
void dirwalkFile(char *path, bool l_opt, bool d_opt, bool f_opt, bool s_opt, FILE *outputFile) {
    struct dirent* dirEntry;
    struct stat sb;
    char fullpath[256];
    struct dirent **namelist;
    int numb = scandir(path, &namelist, NULL, s_opt ? locale_strcmp : NULL);
    if (numb < 0) {
        perror("scandir");
        return;
    }
    for (size_t k = 0; k < numb; k++) {
        dirEntry = namelist[k];
        if ((strcmp(dirEntry->d_name, ".") == 0) || (strcmp(dirEntry->d_name, "..") == 0)) {
            free(namelist[k]);
            continue;
        }
        memset(fullpath, '\0', 256);        
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, dirEntry->d_name);

        if (lstat(fullpath, &sb) == -1) {
            printf("ERROR");
            continue;
        }
        switch (__S_IFMT & sb.st_mode) {
            case __S_IFDIR:
                if (d_opt)
                    fprintf(outputFile, "\n%s", fullpath);
                dirwalkFile(fullpath, l_opt, d_opt, f_opt, s_opt, outputFile);
                break;
            case __S_IFLNK:
                if (l_opt)
                    fprintf(outputFile, "\n%s", fullpath);
                break;
            case __S_IFREG:
                if (f_opt)
                    fprintf(outputFile, "\n%s", fullpath);
                break;
            default: break;
        }
    }
}
