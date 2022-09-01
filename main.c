#include <unistd.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "main.h"

const char* command[] = {
    "{\"type\":\"auth\",\"data\":{\"user_id\":\"1426549\",\"token\":\"e5fa35886eb93205a75f36baeffaa5e6\"}}\n",        // authentication smile_board
    "{\"type\":\"get_channels_list\",\"data\":{\"start\":\"0\",\"count\":\"1\"}}\n"                                   // get channel list
};

const char* message[] = {
    ":gg:",
    ":fry:",
    ":whocares:",
    ":peka:",
    ":fp:",
    ":doggie:",
    ":myway:"
};

static int write_ws, read_ws;
static int status = 0;
static pid_t pid = 0;
static int fd;


/**
 * @brief SIGINT signal handler
 */
static void sigint_handler(int signo)
{
    // enable led indicating successful connection to the chat server
    ioctl(fd, CMD_CONTROL_LED, CMD_CONTROL_LED_ARG_OFF);

    close(fd);
    kill(pid, SIGKILL); //send SIGKILL signal to the child process
    waitpid(pid, &status, 0);
}


/**
 * @brief Main
 */
int main(void)
{
    int pipe_stdin[2];
    int pipe_stdout[2];

    // connect to smilebrd
    if((fd = open("/dev/smilebrd", 0)) == -1){
        perror("Fail to open device node smilebrd");
        return -1;
    }

    // disable led indicating successful connection to the chat server
    ioctl(fd, CMD_CONTROL_LED, CMD_CONTROL_LED_ARG_OFF);

    // create pipes to wscat
    pipe(pipe_stdin);
    pipe(pipe_stdout);

    pid = fork();

    // child process
    if(pid == 0) {
        // duplicate pipes
        dup2(pipe_stdin[0], STDIN_FILENO);
        dup2(pipe_stdout[1], STDOUT_FILENO);

        // close unused pipe ends
        close(pipe_stdin[1]);
        close(pipe_stdout[0]);

        // ask kernel to deliver SIGTERM in case the parent dies
        prctl(PR_SET_PDEATHSIG, SIGTERM);

        // start wscat and connect to goodgame chat server
        execlp("wscat", "wscat", "-c", "wss://chat.goodgame.ru/chat/websocket", NULL);

        exit(1);
    }

    char buffer[PARAM_BUFFER_SIZE];
    char ch_num[PARAM_CHANNEL_NUMBER_SIZE];

    // close unused pipe ends
    close(pipe_stdin[0]);
    close(pipe_stdout[1]);

    // Регистрация обработчика SIGINT
    signal(SIGINT, sigint_handler);

    write_ws = pipe_stdin[1];
    read_ws = pipe_stdout[0];

    // check answer from goodgame chat server
    read_from_wscat(buffer);

    // sing in
    get_com_const(buffer, command[COMMAND_AUTH]);
    send_to_wscat(buffer);

    // get top channel
    get_com_const(buffer, command[COMMAND_GET_CHANNELS]);
    send_to_wscat(buffer);

    // parse channel number
    parse_ch_num(buffer, ch_num);

    // connect to top channel
    get_com_conn_to_ch(buffer, ch_num);
    send_to_wscat(buffer);

    // enable led indicating successful connection to the chat server
    ioctl(fd, CMD_CONTROL_LED, CMD_CONTROL_LED_ARG_ON);

    // sending data to the chat in accordance with the touched button
    while(1) {
        unsigned int data = 0;

        // button touch detection
        read(fd, &data, sizeof(data));

        // send :gg: smile
        if(data & (1 << MESSAGE_GG)) {
            get_com_send_mes(buffer, ch_num, message[MESSAGE_GG]);
            send_to_wscat(buffer);
        }

        // send :fry: smile
        if(data & (1 << MESSAGE_FRY)) {
            get_com_send_mes(buffer, ch_num, message[MESSAGE_FRY]);
            send_to_wscat(buffer);
        }

        // send :whocares: smile
        if(data & (1 << MESSAGE_WHOCARES)) {
            get_com_send_mes(buffer, ch_num, message[MESSAGE_WHOCARES]);
            send_to_wscat(buffer);
        }

        // send :peka: smile
        if(data & (1 << MESSAGE_PEKA)) {
            get_com_send_mes(buffer, ch_num, message[MESSAGE_PEKA]);
            send_to_wscat(buffer);
        }

        // send :fp: smile
        if(data & (1 << MESSAGE_FP)) {
            get_com_send_mes(buffer, ch_num, message[MESSAGE_FP]);
            send_to_wscat(buffer);
        }

        // send :doggie: smile
        if(data & (1 << MESSAGE_DOGGIE)) {
            get_com_send_mes(buffer, ch_num, message[MESSAGE_DOGGIE]);
            send_to_wscat(buffer);
        }
    }

    exit(0);
}


/**
 * @brief Parse server answer to get channel number
 *
 * @param data - buffer containing server answer
 * @param ch_num - buffer for the received channel number
 */
void parse_ch_num(char* data, char* ch_num)
{
    const char pre_str[] = {"channel_id\":\""};
    const char post_str[] = {"\",\"channel_name"};
    int len = 0;
    int len_max = 10;

    char* begin = NULL;
    char* end = NULL;

    // find pre string in asnwer
    if((begin = strstr(data, pre_str)) == NULL) {
        perror("get channel number begin");
        return;
    }

    // find post string in answer
    if((end = strstr(data, post_str)) == NULL) {
        perror("get channel number end");
        return;
    }

    // get lingth of channel number string
    len = end - (begin + strlen(pre_str));

    if((len < 0) || (len > len_max)) {
        perror("get channel number length");
        return;
    }

    // clear channel number buffer
    memset(ch_num, 0, PARAM_CHANNEL_NUMBER_SIZE);

    // write channel number to buffer
    strncpy(ch_num, &begin[strlen(pre_str)], len);

    // terminate the string
    ch_num[len] = 0;
}


/**
 * @brief Generate a constant command
 *
 * @param buffer - buffer to generated command
 * @param data - constant command data
 */
void get_com_const(char* buffer, const char* data)
{
    // clear buffer
    memset(buffer, 0, PARAM_BUFFER_SIZE);

    strcpy(buffer, data);
}


/**
 * @brief Generate a command to connect to a specified channel
 *
 * @param data - buffer to generated command
 * @param ch_num - specified channel
 */
void get_com_conn_to_ch(char* data, char* ch_num)
{
    const char pre_str[] = {"{\"type\":\"join\",\"data\":{\"channel_id\":\""};
    const char post_str[] = {"\",\"hidden\":\"false\"}}\n"};

    if((data == NULL) || (ch_num == NULL)) {
        perror("generate connect command");
        return;
    }

    // clear buffer
    memset(data, 0, PARAM_BUFFER_SIZE);

    strcpy(data, pre_str);
    strcat(data, ch_num);
    strcat(data, post_str);
}


/**
 * @brief Generate a command to send message to a specified channel
 *
 * @param data - buffer to generated command
 * @param ch_num - specified channel
 * @param message - specified message
 */
void get_com_send_mes(char* data, char* ch_num, const char* message)
{
    const char pre_str[] = {"{\"type\":\"send_message\",\"data\":{\"channel_id\":\""};
    const char mid_str[] = {"\",\"text\":\""};
    const char post_str[] = {"\"}}\n"};

    if((data == NULL) || (ch_num == NULL) || (message == NULL)) {
        perror("generate send message command");
        return;
    }

    // clear buffer
    memset(data, 0, PARAM_BUFFER_SIZE);

    strcpy(data, pre_str);
    strcat(data, ch_num);
    strcat(data, mid_str);
    strcat(data, message);
    strcat(data, post_str);
}


/**
 * @brief Read data from pipe
 *
 * @param pipe - pipe to read
 * @param buffer - buffer to write data
 * @param b_len - buffer length
 * @param a_len - minimal read length value
 * @return - read bytes
 */
int read_pipe(int pipe, char* buffer, uint32_t b_len, uint32_t a_len)
{
    const uint32_t DELAY_US = 20000;        // delay in microseconds
    int len = 0;

    // read from pipe
    len = read(pipe, &buffer[len], (b_len - len));

    // error
    if(len < 0) {
        perror("read pipe");
        return -1;
    }

    // if only a piece of data is read, read all data after delay
    if(len < a_len) {
        usleep(DELAY_US);
        len += read(pipe, &buffer[len], (b_len - len));
    }

    // terminate the string
    buffer[len] = 0;

    return len;
}


/**
 * @brief Send data to wscat
 *
 * @param buffer - data to write
 */
void send_to_wscat(char* data)
{
    write(write_ws, data, strlen(data));
    read_from_wscat(data);
}


/**
 * @brief Read data from wscat and print to stdout
 */
void read_from_wscat(char* r_buffer)
{
    static int n = 0;

    read_pipe(read_ws, r_buffer, PARAM_BUFFER_SIZE, PARAM_ANSWER_MIN_SIZE);
    printf("%i. %s\n", n++, r_buffer);
}
