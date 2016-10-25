#include "../cetcd.h"

int main(int argc, char *argv[]) {
    cetcd_client cli;
    cetcd_response *resp;
    cetcd_array addrs;

    cetcd_array_init(&addrs, 3);
    cetcd_array_append(&addrs, "http://127.0.0.1:2379");

    cetcd_client_init(&cli, &addrs);

    resp = cetcd_lsdir(&cli, "/chassis/045f3a11-a485-465a-ba33-16fb5c4a1420", 1, 1);
    printf("err=%d\n",resp->err);
    if(resp->err) {
        printf("error :%d, %s (%s)\n", resp->err->ecode, resp->err->message, resp->err->cause);
    }
    cetcd_response_print(resp);
    cetcd_response_release(resp);

    cetcd_array_destroy(&addrs);
    cetcd_client_destroy(&cli);
    return 0;
}
