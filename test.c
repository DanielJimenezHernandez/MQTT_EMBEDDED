#include<stdio.h> //printf
#include<string.h>    //strlen
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
/* MQTT PAHO test implementation for later porting to FRDM-K64F with Thread*/
/*MQTT INCLUDES*/
#include "MQTTPacket.h"

#define BUFF_SIZE 1024
#define TOPIC "a/b"
#define PORT 1883
#define ADDR "127.0.0.1"

/* connack*/
char testbuffer[4] = {0x20,0x02,0x00,0x00};

int MQTTdeserializePacket(unsigned char * buffer);
const char * MQTT_MsgType2Str(unsigned char packetType);

void printSerializedPacket(int len, unsigned char *buffer){
        int i;
        for (i = 0; i < len; i++){
                printf("0x%02x ",*buffer);
                buffer++;
        }
        printf("\n");

}

MQTTPacket_connectData *connect_options;
MQTTString topicString = MQTTString_initializer;
MQTTPacket_publishData *publish_options;
unsigned char* PAYLOAD = (unsigned char *)"Estoy Hablando desde la pc";
/* wrapper for a no QoS no retain, and no dup serialized publish packet */
int main(int argc, char const *argv[]) {
        /* Publish packet info */
        unsigned char connect_packet[BUFF_SIZE];
        unsigned char subscribe_packet[BUFF_SIZE];
        unsigned char publish_packet[BUFF_SIZE];
        int len_connect,len_subscribe,len_publish;
        char clientID[256];
        int req_qos = 0;
        /* Sockets variables */
        int sock;
        struct sockaddr_in server;
        char message[1000] , server_reply[2000];
        /*Connect packet serialization example */
        strcpy(clientID,"FRDM-K64F");
        MQTTPacket_connectData default_options_connect = MQTTPacket_connectData_initializer;
        default_options_connect.MQTTVersion = 4;
        default_options_connect.clientID.cstring = &clientID[0];
        default_options_connect.keepAliveInterval = 10;
        connect_options = &default_options_connect;
        len_connect = MQTTSerialize_connect(connect_packet,BUFF_SIZE,connect_options);
        /*Subscribe packet serialization example */
        topicString.cstring = TOPIC;
        len_subscribe = MQTTSerialize_subscribe(subscribe_packet,BUFF_SIZE,0,1,1,&topicString,&req_qos);
        /*publish packet serialization example */
        MQTTPacket_publishData default_options_publish = MQTTPacket_publishData_initializer;
        publish_options = &default_options_publish;
        publish_options->topicName.cstring = TOPIC;
        publish_options->payload = PAYLOAD;
        publish_options->payloadlen = strlen((const char *)PAYLOAD);

        len_publish = MQTTSerialize_publish_opt(publish_packet,BUFF_SIZE,publish_options);

        //Create socket
        sock = socket(AF_INET , SOCK_STREAM , 0);
        if (sock == -1)
        {
            printf("Could not create socket");
        }
        puts("Socket created");

        server.sin_addr.s_addr = inet_addr(ADDR);
        server.sin_family = AF_INET;
        server.sin_port = htons( PORT );

        //Connect to remote server
        if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
        {
            perror("connect failed. Error");
            return 1;
        }

        puts("Connected\n");

        //keep communicating with server
        while(1)
        {


            //Send some data
            if( send(sock , connect_packet , len_connect , 0) < 0)
            {
                puts("Send failed");
                return 1;
            }

            //Receive a reply from the server
            if( recv(sock , server_reply , 2000 , 0) < 0)
            {
                puts("recv failed");
                break;
            }

            if(MQTTdeserializePacket(testbuffer)){
                    if( send(sock , publish_packet , len_connect , 0) < 0)
                    {
                        puts("Send failed");
                        return 1;
                    }
            }
        }

        close(sock);
        return 0;
}

typedef struct
{
        unsigned char packetType : 4;
        unsigned char packetFlags : 4;
        unsigned char remainingLen;

}MQTTFixedHeader;

const char * MQTT_MsgType2Str(unsigned char packetType){

        switch (packetType){
                case CONNECT:
                        return "CONNECT";
                case CONNACK:
                        return "CONNACK";
                case PUBLISH:
                        return "PUBLISH";
                case PUBACK:
                        return "PUBACK";
                case PUBREC:
                        return "PUBREC";
                case PUBREL:
                        return "PUBREL";
                case PUBCOMP:
                        return "PUBCOMP";
                case SUBSCRIBE:
                        return "SUBSCRIBE";
                case SUBACK:
                        return "SUBACK";
                case UNSUBSCRIBE:
                        return "UNSUBSCRIBE";
                case UNSUBACK:
                        return "UNSUBACK";
                case PINGREQ:
                        return "PINGREQ";
                case PINGRESP:
                        return "PINGRESP";
                case DISCONNECT:
                        return "DISCONNECT";
                default:
                        return "UNKNOWN";

        }
}

int MQTTdeserializePacket(unsigned char * buffer){
        unsigned char *tmpBuffer;
        const char * tempMSG;
        tmpBuffer = buffer;
        MQTTFixedHeader header;
        header.packetType = (*tmpBuffer >> 4);
        header.packetFlags = (*tmpBuffer & 0x0F);
        tmpBuffer++;
        header.remainingLen = (*tmpBuffer);
        tempMSG = MQTT_MsgType2Str(header.packetType);
        printf("Server Response:%s, Message Length:%d\n",tempMSG,header.remainingLen);
        switch (header.packetType){
                case CONNACK:
                case PUBACK:
                case SUBACK:

        }

        return 0;
}
