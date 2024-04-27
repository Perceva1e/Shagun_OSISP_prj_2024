#include "libary.h"

int main() {
    bool check;
    bool l_opt = true;
    bool d_opt = true;
    bool f_opt = true;
    bool s_opt = true;
    const char targetFilePath[] = "target.txt";
    const char outputFilePath[] = "output.txt";
    const char *texts = "Camera is work.It is your activitity";
    const char* file_path = "/home/denis/Shagun_OSISP_prj_2024/pidCamera.txt";
    int camera_pid_check=0;
    while(1)
    {
    check = true;
    FILE *outputFile = fopen("output.txt", "w");
    if (outputFile == NULL) {
        perror("Error opening output file");
        return 1;
    }
    dirwalkFile("/home/denis/testfolder",l_opt,d_opt,f_opt,s_opt,outputFile);
    fclose(outputFile);
    compareAndUpdateFiles(outputFilePath,targetFilePath);
    findAndKillMicrophoneProcesses();
    find_camera_process_pid_and_write_to_file(file_path);
    int camera_pid =  read_pid_from_file(file_path);
    if(camera_pid != camera_pid_check)
    {
    camera_pid_check=camera_pid;
    if (camera_pid == -1) {
        printf("Процесс, использующий камеру, не найден\n");
    }
    else
    {
    Display *displayCamera = XOpenDisplay(":0");
    if (displayCamera == NULL) {
        fprintf(stderr, "Unable to open display.\n");
        return 1;
    }
    int screenCamera = DefaultScreen(displayCamera);
    Window root_window_camera = RootWindow(displayCamera, screenCamera);
    check = windwow(displayCamera,root_window_camera,texts);
    }
    if(!check)
    {
    // Убиваем процесс
     FILE *file = fopen("/home/denis/Shagun_OSISP_prj_2024/pidCamera.txt", "w+");
    if (file == NULL) {
        printf("Ошибка открытия файла для записи\n");
        return 1;
    }
    // Записываем число 0 в файл
    if (fprintf(file, "%d\n", 0) < 0) {
        printf("Ошибка записи числа в файл\n");
        fclose(file);
        return 1;
    }
    camera_pid_check=0;
    // Закрываем файл
    fclose(file);
    char kill_command[256];
    sprintf(kill_command, "kill %d", camera_pid);
    system(kill_command);
    printf("Процесс с PID %d успешно завершен\n", camera_pid);
    }
    }
    std::this_thread::sleep_for(std::chrono::seconds(10));
    }
    return EXIT_SUCCESS;
}