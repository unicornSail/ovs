#include "../cetcd.h"
#include <stdio.h>
#include<stdlib.h>
#include<string.h>
#include <time.h>  
#include <sys/time.h>  
#include <stdlib.h>  
#include <signal.h>  
static char string[6];
  
static int count = 15;  
static struct itimerval oldtv;  
  
void set_timer()  
{  
    struct itimerval itv;  
    itv.it_interval.tv_sec = 1;  
    itv.it_interval.tv_usec = 0;  
    itv.it_value.tv_sec = 1;  
    itv.it_value.tv_usec = 0;  
    setitimer(ITIMER_REAL, &itv, &oldtv);  
}  
  
void signal_handler(int m)  
{  
    count --;  
    printf("%d\n", count);  
    cetcd_client cli;
    cetcd_response *resp;
    cetcd_array addrs;
    cetcd_array_init(&addrs, 3);
    cetcd_array_append(&addrs, "http://127.0.0.1:2379");
    cetcd_client_init(&cli, &addrs);	            
    resp=cetcd_update(&cli,string,NULL,3,1);
    cetcd_response_print(resp);
    cetcd_response_release(resp);
    cetcd_array_destroy(&addrs);
    cetcd_client_destroy(&cli);
}  
  

int main(int argc, char *argv[]) {
    
    
    cetcd_client cli;
    cetcd_response *resp;
    cetcd_array addrs;

    cetcd_array_init(&addrs, 3);
    cetcd_array_append(&addrs, "http://10.0.0.11:2379");

    cetcd_client_init(&cli, &addrs);

    // resp = cetcd_get(&cli, "/radar/service");
    //resp = cetcd_get(&cli, "/Logical_Router/d39fccce-12c3-408d-9845-c1f65d38f28e");
    //if(resp->err) {
      //  printf("error :%d, %s (%s)\n", resp->err->ecode, resp->err->message, resp->err->cause);
    //}
   
    char result[50];char * encap_type ="encap_type";
	strcpy(result,encap_type);//encaps
   	strcat(result,"encap_ip");
   	
	//strcat(reslut,iface_types_str);
	//strcat(reslut ,bridge_mappings);
	//strcat(reslut ,hostname);//hostname
	strcat(result ,"[]");//vtep_logical_switches
        printf("%s\n",result);
        //char * chassis_id ="abc";
        if(!strcmp(string,"/test")){
           printf("repeat:%d\n",10);
           resp = cetcd_update(&cli,"/test","",2,1);
          }
        else {
           resp = cetcd_set(&cli,"/test",result , 2 );
                strcpy(string,"/test");
                signal(SIGALRM, signal_handler);  
                         set_timer(); 
          }
        //cetcd_setdir(&cli, "chassis_id", 0);
        //	resp = cetcd_set(&cli,"/chassis_id",reslut ,0);
    if(resp->err) {
        printf("error :%d, %s (%s)\n", resp->err->ecode, resp->err->message, resp->err->cause);
    }
    cetcd_response_print(resp);
    cetcd_response_release(resp);
  
    cetcd_array_destroy(&addrs);
    cetcd_client_destroy(&cli);
    while(count>0);

    return 0;
}
