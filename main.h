#ifndef _MAIN_H_
#define _MAIN_H_

typedef enum
{
    COMMAND_AUTH,
    COMMAND_GET_CHANNELS,
} command_num_t;

typedef enum
{
    MESSAGE_GG,
    MESSAGE_FRY,
    MESSAGE_WHOCARES,
    MESSAGE_PEKA,
    MESSAGE_FP,
    MESSAGE_DOGGIE,
    MESSAGE_MYWAY,
} message_num_t;

typedef enum
{
    PARAM_BUFFER_SIZE = 2048,
    PARAM_CHANNEL_NUMBER_SIZE = 11,
    PARAM_ANSWER_MIN_SIZE = 25,
} prog_param_t;

typedef enum
{
    CMD_CONTROL_LED = 0x10,
    CMD_CONTROL_LED_ARG_OFF = 0x00,
    CMD_CONTROL_LED_ARG_ON = 0x01,
} device_cmd_t;


void parse_ch_num(char* data, char* ch_num);
void get_com_const(char* buffer, const char* data);
void get_com_conn_to_ch(char* data, char* ch_num);
void get_com_send_mes(char* data, char* ch_num, const char* message);
int read_pipe(int pipe, char* buffer, uint32_t b_len, uint32_t a_len);
void send_to_wscat(char* data);
void read_from_wscat(char* r_buffer);

#endif  // _MAIN_H_