#ifndef HTTPS_SERVER_H
#define HTTPS_SERVER_H

#define OTA_UPDATE_PENDING 		0
#define OTA_UPDATE_SUCCESSFUL	1
#define OTA_UPDATE_FAILED		-1

void start_server(void);
void stop_server(void);


#endif