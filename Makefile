CC=gcc
MQTT_DIR=mqtt
CFLAGS=-I$(MQTT_DIR) -g3 -Wall

TESTAPP = testapp
BRIDGE_APP = udp_tcp_mqtt_bridge
UDP_CLIENT = udpclient
TCP_SERVER = tcpserver

OBJS = $(MQTT_DIR)/MQTTConnectClient.o \
$(MQTT_DIR)/MQTTUnsubscribeServer.o \
$(MQTT_DIR)/MQTTSerializePublish.o \
$(MQTT_DIR)/MQTTSubscribeServer.o \
$(MQTT_DIR)/MQTTConnectServer.o \
$(MQTT_DIR)/MQTTDeserializePublish.o \
$(MQTT_DIR)/MQTTPacket.o \
$(MQTT_DIR)/MQTTUnsubscribeClient.o \
$(MQTT_DIR)/MQTTFormat.o \
$(MQTT_DIR)/MQTTSubscribeClient.o 

OBJS_BRIDGE = udp_tcp_mqtt_bridge.o

OBJS_UDP = udpclient.o
OBJS_TCP = tcpserver.o

DEPS = $(MQTT_DIR)/MQTTConnectClient.c \
$(MQTT_DIR)/MQTTUnsubscribeServer.c \
$(MQTT_DIR)/MQTTSerializePublish.c \
$(MQTT_DIR)/MQTTSubscribeServer.c \
$(MQTT_DIR)/MQTTConnectServer.c \
$(MQTT_DIR)/MQTTDeserializePublish.c \
$(MQTT_DIR)/MQTTPacket.c \
$(MQTT_DIR)/MQTTUnsubscribeClient.c \
$(MQTT_DIR)/MQTTFormat.o \
$(MQTT_DIR)/MQTTSubscribeClient.o \
test.o

network: $(BRIDGE_APP) $(UDP_CLIENT) $(TCP_SERVER)

$(TESTAPP): $(OBJS)
	gcc -o $@ $(OBJS) $(CFLAGS)

$(BRIDGE_APP): $(OBJS_BRIDGE) $(OBJS)
	gcc -o $@ $(OBJS_BRIDGE) $(OBJS) $(CFLAGS) -lpthread

$(UDP_CLIENT): $(OBJS_UDP) $(OBJS)

$(TCP_SERVER): $(OBJS_TCP)

.PHONY: clean

clean:
	-rm ./*.o
	-rm ./$(MQTT_DIR)/*.o
	-rm $(TESTAPP)
	-rm $(BRIDGE_APP)
	-rm $(UDP_CLIENT)
	-rm $(TCP_SERVER)
