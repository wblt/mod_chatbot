
#include <switch.h>


#include "NlsClient.h"

#include<sys/types.h>
#include<sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <time.h>




#define __SOCKET___

//#define __FILE_WRITE__

//#define __ASR_ALI__

SWITCH_MODULE_LOAD_FUNCTION(mod_chatbot_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_chatbot_shutdown);

extern "C" {
	SWITCH_MODULE_DEFINITION(mod_chatbot, mod_chatbot_load, mod_chatbot_shutdown, NULL);
}


#if defined(__ASR_ALI__)
NlsSpeechCallback* gCallback = NULL;
NlsClient* gNlc = NULL;
#endif


SWITCH_STANDARD_APP(start_chatbot_session_function)
{
	switch_status_t status;
	switch_frame_t *read_frame;
    char*	frame_data;
    int 	frame_len ;
    char *mydata, *argv[4] = { 0 };
    int    buf[1024]={0};
    int    nread =0;
    switch_codec_t*  codec=NULL;
    int sock=socket(AF_INET,SOCK_STREAM,0);




	switch_channel_t *channel = switch_core_session_get_channel(session);



	if (switch_channel_pre_answer(channel) != SWITCH_STATUS_SUCCESS) {
		return;
	}



	if (!zstr(data) && (mydata = switch_core_session_strdup(session, data))) {

		 switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "start_chatbot_session_function  args\r\n");

		switch_separate_string(mydata, ',', argv, (sizeof(argv) / sizeof(argv[0])));
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "start_chatbot_session:%s-%s-%s\r\n",mydata,argv[0],argv[1]);
	} else {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "No file specified.\n");
		return;
	}




#if defined(__FILE_WRITE__)
    char fname[200]={0};
    time_t seconds;

    seconds = time(NULL);
    sprintf(fname,"/home/chatbot%ld.pcm",seconds);
    FILE *fid = fopen(fname,"a+b");
#endif


#if defined(__SOCKET___)

	  if(sock<0)
	    {
	    	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "socket create failue.\n");
	        return;
	    }


	    struct sockaddr_in server;
	    server.sin_family=AF_INET;
	    server.sin_port=htons(atoi(argv[1]));
	    server.sin_addr.s_addr = inet_addr(argv[0]);


	    if(connect(sock,(struct sockaddr*)&server,(socklen_t)sizeof(server))<0)
	        {
	        	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "socket connect failue.\n");
	            return;
	        }



	    char idbuf[200]={0};

		codec = switch_core_session_get_read_codec(session);
		if(codec!=NULL)
		{
			 strcat(idbuf,codec->implementation->modname);
			 strcat(idbuf,":");
			 switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "start_chatbot_session_function:%s",codec->implementation->modname);
		}

		strcat(idbuf,switch_channel_get_name(channel));
	    write(sock,idbuf,strlen(idbuf));



	nread = read(sock,buf,sizeof(buf)-1);
	//switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_INFO, "socket read:%d,%d,%s.\n",errno,nread,buf);
#endif


#if defined(__ASR_ALI__)
        NlsRequest* request= gNlc->createRealTimeRequest(gCallback, "/etc/config-realtime.txt");
        if (request != NULL)
        {
        	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_INFO, "request create success");
            request->SetParam("Id", switch_channel_get_name(channel));
            request->Authorize("XX","XX");
            if (request->Start() < 0) {
            	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_INFO, "request start failue");
                delete request;
                request = NULL;
            }
        }
        else
        {
        	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_INFO, "request create failue");
        }

#endif

  // switch_channel_set_flag(channel, CF_TEXT_ECHO);

   switch_core_session_raw_read(session);
   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "start_chatbot_session_function  start\r\n");




   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "start_chatbot_session_function  request\r\n");

   //switch_core_media_bug_add(session, "chatbot", NULL,chatbot_callback, NULL, 0, SMBF_READ_REPLACE | SMBF_NO_PAUSE | SMBF_ONE_ONLY,NULL);


	while (switch_channel_ready(channel)) {


		//switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "chatbot Start Succeed channel:%s\n", switch_channel_get_name(channel));

		status = switch_core_session_read_frame(session, &read_frame, SWITCH_IO_FLAG_NONE, 0);

		if (!SWITCH_READ_ACCEPTABLE(status)) {
			break;
		}


        frame_len = read_frame->datalen;
        frame_data = (char*)read_frame->data;


#if defined(__ASR_ALI__)
        if (request != NULL)
        {
        	request->SendAudio(frame_data, frame_len);
        }
#endif

#if defined(__FILE_WRITE__)
       // if(frame_len>2)
        {
        	fwrite(frame_data,frame_len,1,fid);
        }
#endif




#if defined(__SOCKET___)
        write(sock,frame_data,frame_len);
    	nread = read(sock,buf,sizeof(buf)-1);
    	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_INFO, "socket read:%d,%d,%d,%d\n",errno,nread,read_frame->channels,frame_len);
#endif




    	if(nread>0)
    	{
        	read_frame->datalen=nread;
        	read_frame->data=buf;
    	}

    	switch_core_session_write_frame(session, read_frame, SWITCH_IO_FLAG_NONE, 0);


		if (switch_channel_test_flag(channel, CF_BREAK)) {
			switch_channel_clear_flag(channel, CF_BREAK);
			break;
		}
	}

	switch_core_session_reset(session, SWITCH_TRUE, SWITCH_TRUE);
	//switch_channel_clear_flag(channel, CF_TEXT_ECHO);

#if defined(__SOCKET___)
	close(sock);
#endif

#if defined(__FILE_WRITE__)
	fclose(fid);
#endif

#if defined(__ASR_ALI__)
        if (request != NULL)
        {
        	request->Stop();
            delete request;
            request = NULL;
        }
#endif


	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "start_chatbot_session_function  end\r\n");

}

#if defined(__ASR_ALI__)

void OnResultDataRecved(NlsEvent* str,void* para=NULL) {
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, " OnResultDataRecved:%s\n",str->getResponse().c_str());

   // sprintf(fname,"/home/chatbot%ld.txt");
    FILE *fid = fopen("/home/chatbot.txt","a");


    fputs(str->getResponse().c_str(),fid);
    fputs("\r\n",fid);
    fclose(fid);
}

void OnOperationFailed(NlsEvent* str, void* para=NULL) {
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, " OnOperationFailed:%s\n",str->getResponse().c_str());

}

void OnChannelCloseed(NlsEvent* str, void* para=NULL) {
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, " OnChannelCloseed:%s\n",str->getResponse().c_str());
}

#endif



SWITCH_MODULE_LOAD_FUNCTION(mod_chatbot_load)
{


	switch_application_interface_t *app_interface;
    *module_interface = switch_loadable_module_create_module_interface(pool, modname);
    SWITCH_ADD_APP(app_interface, "chatbot", "chatbot", "chatbot",start_chatbot_session_function, "", SAF_SUPPORT_NOMEDIA);

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, " chatbot_load\n");

#if defined(__ASR_ALI__)
	gNlc = NlsClient::getInstance();
	int ret = gNlc->setLogConfig("/etc/log-realtime.txt", 3);

	int id = 123434;
    gCallback = new NlsSpeechCallback();
    gCallback->setOnMessageReceiced(OnResultDataRecved, &id);
    gCallback->setOnOperationFailed(OnOperationFailed, &id);
    gCallback->setOnChannelClosed(OnChannelCloseed, &id);

#endif

    return SWITCH_STATUS_SUCCESS;
}




SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_chatbot_shutdown)
{
   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "chatbot_shutdown socket\n");

#if defined(__ASR_ALI__)
	if(gNlc != NULL) {
		delete gNlc;
		gNlc = NULL;
	}

	if(gCallback !=NULL)
	{
		delete gCallback;
	   gCallback = NULL;
	}
#endif


    return SWITCH_STATUS_SUCCESS;
}
