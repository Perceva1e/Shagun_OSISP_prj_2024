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

bool windwow(Display *display, Window root_window,const char *texts) {
    int screen = DefaultScreen(display);
    // Создаем всплывающее окно
    Window window = XCreateSimpleWindow(display, root_window, 0, 0, 300, 200, 1,
                                        BlackPixel(display, screen), WhitePixel(display, screen));
    XSelectInput(display, window, ExposureMask | ButtonPressMask);

    // Устанавливаем заголовок окна
    XStoreName(display, window, "Alarm");

    // Создаем текстовую метку
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

    // Создаем кнопки "Да" и "Нет"
    const char *yes_label = "Yes";
    const char *no_label = "No";
    int button_width = 60;
    int button_height = 40;
    int button_x = 50;
    int button_y = 100;

    // Кнопка "Да"
    XDrawRectangle(display, window, gc, button_x, button_y, button_width, button_height);
    XDrawString(display, window, gc, button_x + 10, button_y + 25, yes_label, strlen(yes_label));

    // Кнопка "Нет"
    XDrawRectangle(display, window, gc, button_x + 100, button_y, button_width, button_height);
    XDrawString(display, window, gc, button_x + 110, button_y + 25, no_label, strlen(no_label));

    XMapWindow(display, window);

    XEvent event;
    while (1) {
        XNextEvent(display, &event);
        switch (event.type) {
            case Expose:
                // Перерисовываем окно
                XClearWindow(display, window);
                XDrawString(display, window, gc, 50, 50, text, strlen(text));
                XDrawRectangle(display, window, gc, button_x, button_y, button_width, button_height);
                XDrawRectangle(display, window, gc, button_x + 100, button_y, button_width, button_height);
                XDrawString(display, window, gc, button_x + 10, button_y + 25, yes_label, strlen(yes_label));
                XDrawString(display, window, gc, button_x + 110, button_y + 25, no_label, strlen(no_label));
                break;
            case ButtonPress:
                if (event.xbutton.button == Button1) {
                    // Проверяем, была ли нажата кнопка "Да" или "Нет"
                    if (event.xbutton.x >= button_x && event.xbutton.x <= button_x + button_width &&
                        event.xbutton.y >= button_y && event.xbutton.y <= button_y + button_height) {
                        // Нажата кнопка "Да"
                        printf("Да\n");
                        XCloseDisplay(display);
                        return(true);
                    } else if (event.xbutton.x >= button_x + 100 && event.xbutton.x <= button_x + 100 + button_width &&
                               event.xbutton.y >= button_y && event.xbutton.y <= button_y + button_height) {
                        // Нажата кнопка "Нет"
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
    // Открываем поток для выполнения команды
    FILE* fp = popen(command, "r");
    if (fp == NULL) {
        return NULL;
    }

    // Буфер для хранения вывода команды
    char buffer[1024];
    char* result = (char*)malloc(sizeof(char) * 1024);
    strcpy(result, "");

    // Читаем вывод команды
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        strcat(result, buffer);
    }

    // Закрываем поток
    pclose(fp);

    return result;
}

// Функция для нахождения PID процесса, использующего камеру
int find_camera_process_pid() {
    // Команда для поиска процесса, связанного с использованием камеры
    const char* command = "lsof /dev/video0";

    // Выполняем команду и получаем ее вывод
    char* output = execute_command(command);
    if (output == NULL) {
        printf("Ошибка выполнения команды\n");
        return -1;
    }

    // Парсим вывод команды для нахождения PID процесса
    char* pid_str = strtok(output, " ");
    while (pid_str != NULL) {
        // Проверяем, является ли строка числом (PID)
        int pid = atoi(pid_str);
        if (pid > 0) {
            free(output);
            return pid;
        }
        pid_str = strtok(NULL, " ");
    }

    free(output);
    return -1; // Если PID не найден
}

// Функция для поиска и завершения процессов, использующих микрофон
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
    int hasMicrophoneProcesses = 0; // Флаг для отслеживания наличия процессов, использующих микрофон

    // Открываем каталог /proc
    dir = opendir(PROC_DIRECTORY);
    if (dir == NULL) {
        perror("Ошибка при открытии каталога /proc");
        exit(1);
    }

    // Читаем содержимое каталога /proc
    while ((entry = readdir(dir)) != NULL) {
        // Проверяем, что это каталог и его имя является числом (PID)
        if (entry->d_type == DT_DIR && atoi(entry->d_name) != 0) {
            snprintf(path, sizeof(path), "%s/%s/status", PROC_DIRECTORY, entry->d_name);

            // Открываем файл /proc/<PID>/status
            FILE *fp = fopen(path, "r");
            if (fp != NULL) {
                // Ищем строку с именем процесса
                while (fgets(line, sizeof(line), fp) != NULL) {
                    if (strncmp(line, "Name:", 5) == 0) {
                        // Если процесс связан с микрофоном, выводим его
                        if (strstr(line, "audio") != NULL || strstr(line, "sound") != NULL) {
                            printf("Процесс с PID %s использует микрофон.\n", entry->d_name);
                            hasMicrophoneProcesses = 1;
                            // Записываем PID процесса в файл
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

// Функция для поиска и завершения процессов, использующих микрофон
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
        rewind(file2); // Сбрасываем указатель файла 2 на начало для каждой строки файла 1
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

void findAndKillMicrophoneProcesses(Display *display, Window root_window) {
    bool check = false;
     const char *texts = "Microfone is work.It is your activitity";
    // Выводим список процессов, использующих микрофон
    if (listMicrophoneProcesses()) {
        // Проверяем наличие лишних процессов
        check = compareFiles();
    }
    if(check)
    {   
       check = !(windwow(display,root_window,texts));
    }
    if (check) {   
        // Открываем файл с PID процессов
        FILE *file = fopen("microphone_processes.txt", "r");
        if (file == NULL) {
            perror("Ошибка при открытии файла с PID");
            exit(1);
        }

        char line[MAX_LINE_LENGTH];
        
        // Создаем временный файл для записи обновленного списка PID процессов
        FILE *tempFile = fopen("temp_microphone_processes.txt", "w");
        if (tempFile == NULL) {
            perror("Ошибка при создании временного файла");
            exit(1);
        }

        // Завершаем лишние процессы и удаляем соответствующие записи из файла
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
        
        // Заменяем исходный файл с PID процессов обновленным списком
        if (rename("temp_microphone_processes.txt", "microphone_processes.txt") != 0) {
            perror("Ошибка при замене файла");
            exit(1);
        }
    } else {
        printf("Нет лишних процессов для завершения.\n");
    }

    // Обновляем файл с предыдущими PID процессами
    system("cp microphone_processes.txt previous_microphone_processes.txt");
}


void deleteExtraFiles(const char *file1, const char *file2) {
    FILE *fp1, *fp2;
    char path1[1000], path2[1000];

    // Открываем файлы для чтения
    fp1 = fopen(file1, "r+");
    fp2 = fopen(file2, "r");

    if (fp1 == NULL || fp2 == NULL) {
        printf("Ошибка при открытии файлов.\n");
        return;
    }

    // Читаем файлы построчно и удаляем файлы и директории, которые есть в file1 и отсутствуют в file2
    while (fgets(path1, sizeof(path1), fp1) != NULL) {
        // Удаляем символ новой строки в конце пути
        path1[strcspn(path1, "\n")] = 0;

        int found = 0;
        // Перематываем файл2 к началу для каждой проверки
        rewind(fp2);
        while (fgets(path2, sizeof(path2), fp2) != NULL) {
            path2[strcspn(path2, "\n")] = 0;
            if (strcmp(path1, path2) == 0) {
                found = 1;
                break;
            }
        }
        // Если файл из file1 не найден в file2, удаляем его
        if (!found) {
            // Проверяем, является ли путь файлом или директорией
            if (remove(path1) == 0) {
                printf("Файл или директория %s удалены.\n", path1);
                // После успешного удаления пути из file1
                // перемещаем указатель файла на начало и удаляем строку с этим путем
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

    // Закрываем файлы
    fclose(fp1);
    fclose(fp2);
}

int locale_strcmp(const struct dirent **a, const struct dirent **b) {
    return strcoll((*a)->d_name, (*b)->d_name);
}
void compareAndUpdateFiles(const char *sourceFilePath, const char *targetFilePath, Display *display, Window root_window) {
    const char *texts = "Find suspicious files.It is your activitity";
    FILE *sourceFile = fopen(sourceFilePath, "r");
    FILE *targetFile = fopen(targetFilePath, "r+");

    if (sourceFile == NULL || targetFile == NULL) {
        perror("Error opening files");
        return;
    }

    // Get the size of the source file
    fseek(sourceFile, 0L, SEEK_END);
    long sourceFileSize = ftell(sourceFile);
    rewind(sourceFile);

    // Get the size of the target file
    fseek(targetFile, 0L, SEEK_END);
    long targetFileSize = ftell(targetFile);
    rewind(targetFile);

    // Compare file sizes
    if (sourceFileSize != targetFileSize) {
        // If file sizes are different, update the target file
        char *buffer = (char *)malloc(sourceFileSize + 1);
        fread(buffer, 1, sourceFileSize, sourceFile);
        buffer[sourceFileSize] = '\0';

        // Write the content of the source file to the target file
        fseek(targetFile, 0, SEEK_SET);
        if(windwow(display,root_window,texts)==false)
        {
            deleteExtraFiles(sourceFilePath,targetFilePath);
            printf("Suspicious files are deleted\n");
            return;
        }
        fwrite(buffer, 1, sourceFileSize, targetFile);
        // Print a message to the console
        printf("Files are different.Updated target file: %s\n", targetFilePath);

        free(buffer);
        fclose(sourceFile);
        fclose(targetFile);
        return;
    } else {
        // If file sizes are the same, compare contents
        char sourceChar, targetChar;
        bool filesMatch = true;
        while ((sourceChar = fgetc(sourceFile)) != EOF && (targetChar = fgetc(targetFile)) != EOF) {
            if (sourceChar != targetChar) {
                filesMatch = false;
                break;
            }
        }

        if (!filesMatch) {
            // If contents are different, update the target file
            fseek(sourceFile, 0, SEEK_SET);
            fseek(targetFile, 0, SEEK_SET);
            if(windwow(display,root_window,texts)==false)
            {
            deleteExtraFiles(sourceFilePath,targetFilePath);
              printf("Suspicious files are deleted\n");
            return;
            }
            int ch;
            while ((ch = fgetc(sourceFile)) != EOF) {
                fputc(ch, targetFile);
            }
            
            // Print a message to the console
            printf("Files are different. Updated target file: %s\n", targetFilePath);
            fclose(sourceFile);
            fclose(targetFile);
            return;
        } else {
            // If contents are the same, print a message to the console
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

int main() {
    bool check = false;
    Display *displayFile = XOpenDisplay(":0");
    if (displayFile == NULL) {
        fprintf(stderr, "Unable to open display.\n");
        return 1;
    }
    Display *displayAudio = XOpenDisplay(":0");
    if (displayAudio == NULL) {
        fprintf(stderr, "Unable to open display.\n");
        return 1;
    }
    Display *displayCamera = XOpenDisplay(":0");
    if (displayCamera == NULL) {
        fprintf(stderr, "Unable to open display.\n");
        return 1;
    }
    int screenCamera = DefaultScreen(displayCamera);
    Window root_window_camera = RootWindow(displayCamera, screenCamera);
    int screenFile = DefaultScreen(displayFile);
    Window root_window_file = RootWindow(displayFile, screenFile);
    int screenAudio = DefaultScreen(displayAudio);
    Window root_window_audio = RootWindow(displayAudio, screenAudio);
    bool l_opt = true;
    bool d_opt = true;
    bool f_opt = true;
    bool s_opt = true;
    const char targetFilePath[] = "target.txt";
    const char outputFilePath[] = "output.txt";
    while(1)
    {
    FILE *outputFile = fopen("output.txt", "w");
    if (outputFile == NULL) {
        perror("Error opening output file");
        return 1;
    }
    dirwalkFile("/home/denis/testfolder",l_opt,d_opt,f_opt,s_opt,outputFile);
    fclose(outputFile);
    compareAndUpdateFiles(outputFilePath,targetFilePath,displayFile,root_window_file);
    findAndKillMicrophoneProcesses(displayAudio,root_window_audio);
    int camera_pid = find_camera_process_pid();
    if (camera_pid == -1) {
        printf("Процесс, использующий камеру, не найден\n");
    }
    else
    {
    const char *texts = "Camera is work.It is your activitity";
    check = windwow(displayCamera,root_window_camera,texts);
    }
    if(check)
    {
    // Убиваем процесс
    char kill_command[256];
    sprintf(kill_command, "kill %d", camera_pid);
    system(kill_command);
    printf("Процесс с PID %d успешно завершен\n", camera_pid);
    }
    std::this_thread::sleep_for(std::chrono::seconds(10));
    }
    return EXIT_SUCCESS;
}