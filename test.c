#include <stdio.h>
#include <string.h>

/*MQTT INCLUDES*/
#include "MQTTPacket.h"

#define BUFF_SIZE 1024

MQTTPacket_connectData *connect_options;
/* wrapper for a no QoS no retain, and no dup serialized publish packet */
int main(int argc, char const *argv[]) {
        /* Publish packet info */
        char connect_packet[BUFF_SIZE];
        int len,i;
        char clientID[256];
        strcpy(clientID,"mosqpub/24279-debian");
        MQTTPacket_connectData default_options = MQTTPacket_connectData_initializer;
        default_options.MQTTVersion = 3;
        default_options.clientID.cstring = &clientID[0],
        connect_options = &default_options;
        len = MQTTSerialize_connect(connect_packet,BUFF_SIZE,connect_options);
        for (i = 0; i <= len; i++){
          printf("0x%02x\n",connect_packet[i]);
        }
        for (i = 0; i <= len; i++){
          printf("%c",connect_packet[i]);
        }

        return 0;
}
