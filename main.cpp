#define WINVER 0x0601
#include <iostream>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>

using namespace std;

int main()
{
    // 1. Создать два анонимных канала
    SECURITY_ATTRIBUTES attributes;
    attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    attributes.bInheritHandle = TRUE;
    attributes.lpSecurityDescriptor = NULL;

    HANDLE input_pipe_read_end;
    HANDLE input_pipe_write_end;
    CreatePipe(&input_pipe_read_end, &input_pipe_write_end, &attributes, 0);

    HANDLE output_pipe_read_end;
    HANDLE output_pipe_write_end;
    CreatePipe(&output_pipe_read_end, &output_pipe_write_end, &attributes, 0);

    SetHandleInformation(input_pipe_write_end, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(output_pipe_read_end, HANDLE_FLAG_INHERIT, 0);

    // 2. Запустить процесс cmd.exe
    TCHAR Cmdline[] = TEXT("cmd.exe");

    STARTUPINFO startup_info;
    ZeroMemory(&startup_info, sizeof(STARTUPINFO));
    startup_info.cb = sizeof(STARTUPINFO);
    startup_info.hStdInput = input_pipe_read_end;
    startup_info.hStdOutput = output_pipe_write_end;
    startup_info.hStdError = output_pipe_write_end;
    startup_info.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
    CreateProcess(
        NULL,
        Cmdline,
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &startup_info,
        &pi);

    // 3. Считать из канала все данные и вывести их на экран
    DWORD bytes_read;
    char buf[64];
    BOOL flag = TRUE;
    while (flag) {
        do {
            ReadFile(output_pipe_read_end, buf, sizeof(buf), &bytes_read, NULL);
            fwrite(buf, bytes_read, 1, stdout);
            //if (buf[bytes_read - 1] == '>'){
                //bytes_read = 0;
            //}
        } while (buf[bytes_read - 1] != '>');

        // 4. Запросить у пользователя полную строку-команду
        const char PLEASE[] = "please";
        char* input = NULL;
        char buffer[256];
        while (!input) {
            fgets(buffer, sizeof(buffer), stdin);

            // 6. Если введена строка «thanks», остановить дочерний процесс
            if (!strncmp(buffer, "thanks", 6)) {
                flag = FALSE;
                TerminateProcess(&pi,0);
                CloseHandle(pi.hProcess);
                CloseHandle(output_pipe_write_end);
                CloseHandle(input_pipe_read_end);
                CloseHandle(output_pipe_read_end);
                CloseHandle(input_pipe_write_end);
                break;
            }
            // 5. Введенная строка должна начинаться со слова «please»
            else if (strncmp(buffer, "please", 6)) {
                fprintf(stderr, "Please ask politely!\n> ");
            }
            // 7. Запись в канал часть команды (без please)
            else {
                input = buffer + sizeof(PLEASE);
                WriteFile(input_pipe_write_end, input, strlen(input), NULL, NULL);
                break;
            }
        }
    }
    return 0;
}
