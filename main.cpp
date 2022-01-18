 #include "mbed.h"
#include "SpwfInterface.h"
#include "SDFileSystem.h"
#include "TCPSocket.h"
#include "Rtc_Ds1307.h"
#include <sys/time.h>

// D1->UART6_TX , D0->UART6_RX , 
SpwfSAInterface wifi(D1,D0,false);

TCPSocket socket,socket2;

//stm32f4 mosi=> PA7 , mosi=> PA6 , SCK => PA5
//TM_SPI_Init(SPI1, TM_SPI_PinsPack_1);

SDFileSystem sdc(D11, D12, D13, D9, "sd"); // MOSI, MISO, SCK, CS

AnalogIn analog_in(A0);

//timer
Rtc_Ds1307 rtc(PB_9, PB_8);


bool sdck=0;
char serverip[15];
char USER[15];
char PASSWORD[15];


/////////File path ///////////

char filepath1[100];


///////WIFI////////
int32_t connected;
char* ssid = "";
char* pass = "";
const char *ip;


////////Timer///////
int year;
int mon;
int date;
int hour;
int min;
int sec;


FILE *fp1;

//****************FIND IP FUNCTION**********************

void read_IP(){
    FILE *fd = fopen("/sd/address_ip.txt","r");// file where is saved the server_ip
    if(fd != NULL){
        fgets(serverip,15,fd);
        sdck=1;
    }

    else{
        printf("error reading server ip\n");
        sdck=0;
        }
    fclose(fd);
    }

//****************FIND data port**********************


int get_data_port(char *req){


    //The server send this port with cripted code
    //192,168,nnn,nnn,xxx,yyy and the solution is port=((xxx*256)+yyy)
   
    char xxx[4];
    char yyy[4];
    char *posstart;
    char *posend;
    int i;
    int pos;
    int pos2;
    int port;

    posstart=req; 
    for(i=0;i<4;i++) 
        posstart=strchr(posstart+1,','); // 4th ','
    if(posstart==NULL)
        return -1;
    printf("found");
    posstart++;
    posend=strchr(posstart,','); // 5th ',' 
    if(posend==NULL) 
       return -1;

    pos=(posend-posstart);      //number of character
    strncpy(xxx,posstart,pos);
    port=atoi(xxx)*256; //xxx*256
    posstart=posend+1;
    posend=strchr(posstart,'\r');
    if(posend==NULL)
     return -1;
    pos2=(posend-posstart);
    strncpy(yyy,posstart,pos2);
    port=port+atoi(yyy); //(xxx*256)+yyy

    return port;

    }



void send_data_to_server(char *path){

    int open ;
    char buffer[256];
    int port;
    int rec ;
    FILE *fp ; 
    int size=0;
    int res;
    int len=0;
    char str[4096];

    socket2.set_timeout(3000);
    open= socket2.open(&wifi); //socket for data transmission

             if(!open){
                  
                 rec=socket2.recv(buffer,sizeof(buffer));

                 if(rec != 0 && rec!= 1 ){
                    port=get_data_port(buffer);
                    printf("port : %d \n",port);

                    socket2.bind(ip,port);
                    socket.recv(buffer,sizeof(buffer));
                    //printf("buffer=%s\n",buffer);
                    socket2.connect(serverip,port);
                    socket.recv(buffer,sizeof(buffer));
                    //printf("buffer=%s\n",buffer);

                     fp = fopen(path,"r");
                     if (fp != NULL)
                     {
                         // dimension of file
                         seek(fp, 0, SEEK_END);
                         size = ftell(fp);
                         fseek(fp, 0, SEEK_SET);
                         printf("size= %d\n",size);

                         int n=0;
                         while(1){
                         n++;
                         res=fread(&str,1,4094,fp);
                         if(n*4094<=size){
                            printf("send packet of  4096 byte\n");
                            socket2.send(str,strlen(str));      //send  4094 byts
                          }
                         else{
                            len = strlen(str)-(n*4094-size); //rest of byte to send
                            char log[len];
                            for(int i=0;i<len;i++){
                                log[i]=str[i];
                            }

                            log[len]='\0';  //terminate the string with '\0'
                            socket2.send(log,len);
                            printf("send packet of %d byte\n",len);
                            break;
                           }

                     }
                      fclose(fp);
        }

    socket2.close();
    
}


void connect_to_server(){


    char buffer[1024];
    int len;
    char* user ="";
    char* pwd ="";

    socket.set_timeout(3000);
    printf("connection to the server\n");
    int open=socket.open(&wifi);

    //binding a socket specifies the address and port on which to recieve data 
    socket.bind(ip,22);

            if(!open){

                    socket.connect(serverip,21);
                    len=socket.recv(buffer,sizeof(buffer));
                 
                    if(len != 0 && len!= 1 ){
                        printf("connected on port 21\n");
                        //printf("buffer=%s\n",buffer);
                        socket.send(user,strlen(user));
                        socket.recv(buffer,sizeof(buffer));
                        //printf("buffer=%s\n",buffer);
                        socket.send(pwd,strlen(pwd));
                        socket.recv(buffer,sizeof(buffer));
                        //printf("buffer=%s\n",buffer);
                        sended=send_data_to_server(filepath1) ;
                       

                    }
                    else { 
                        printf("Error connecting to server\n");
                    }
            }
    socket.close();
}


void init_wifi(){
  
  bool sd=0 ;
  ip = wifi.get_ip_address();
  sd=sdc.card_present();

    if(sd) {
       FILE *fd = fopen("/sd/address_ip.txt","w");
        fprintf(fd,"%s", ip);
    }
 
}

void init_time(){

    //INIT TIME

    Rtc_Ds1307::Time_rtc tm = {};

    year=tm.year=2021;
    mon=tm.mon=1;
    date=tm.date=1;
    hour=tm.hour=0;
    min=tm.min=0;
    sec=tm.sec=0;
    rtc.setTime(tm,true,false);//rtc initialization

    rtc.startClock();

    rtc.getTime(tm);
    printf("02%d/02%d/%04d/%02d.%02d.%02d",tm.date, tm.mon, tm.year, tm.hour, tm.min, tm.sec);

}


  ////////  SD  //////////////

void boot(){

    bool sd=0 ;
    init_wifi();  
    sd=sdc.card_present();
    if(sd) {

        read_IP(); 
        if(sdck){
            printf("succes boot\n");
         
         }
        else
            printf("Error\n");

         }
}

int main() {

      struct itimerval timer;
      signal(SIGALRM, (void (*)(int)) doStuff);
      double val ;

      // Configure the timer to expire after 500 ms
      // the time until the timer next expires 
      timer.it_value.tv_sec = 3600; 
      // the value that timer will be reset to after it expires
      timer.it_interval.tv_sec = 3600; 

     
    Configure_sensor();

    init_time(); 
    boot();

    mkdir("/sd/data", 0777);

    filepath1="/sd/data.txt" ;
    fp1=fopen(filepath1,"a");
    
    while (1) {
             // Start a virtual timer. It counts down whenever this process is executing. 
         // When the timer code is ITIMER_VIRTUAL, the process is sent a SIGVTALRM signal after the process has executed for the specified time
   

           setitimer (ITIMER_REAL, &timer, NULL);
             
            do {
 
              printf("timer starting\n");
              val=analog_in.read();
              //val=
              if (fp1)
                fprintf(fp1,"value : %d , date : 02%d/02%d/%04d/%02d:%02d:%02d",val,date, mon, year, hour, min, sec);
           
               } while(timer_getoverrun(*timer) == 0) ;

            connected=wifi.connect(ssid, pass);
            init_wifi(); 
            ip = wifi.get_ip_address();

              if(connected){
                connect_to_server();
              }
               else
               {
                   printf("error \n") ;
                   break;
               }   
               }                
            fp1.close(); 
}