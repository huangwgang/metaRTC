﻿//
// Copyright (c) 2019-2022 yanggaofeng
//
#include <yangp2p/YangP2pHandleImpl.h>
#include <yangutil/sys/YangLog.h>
#include <yangutil/yang_unistd.h>

#include <yangutil/sys/YangUrl.h>


void g_p2p_receive(char *data, int32_t nb_data,char* response,char* remoteIp, void *user) {
	if (user == NULL)
		return;
	YangP2pHandleImpl *p2p = (YangP2pHandleImpl*) user;

	p2p->startRtc(remoteIp, data,response);
}

YangP2pHandleImpl::YangP2pHandleImpl(bool phasAudio,YangContext* pcontext,YangSysMessageI* pmessage) {
	m_pub = NULL;
	m_decoder=NULL;
	m_videoState=Yang_VideoSrc_Camera;
	m_context = pcontext;
	m_message = pmessage;
	m_cap = new YangP2pPublish(m_context);
	m_cap->setCaptureType(m_videoState);
	m_hasAudio = phasAudio;
	m_isInit=false;
	m_isInitRtc=false;
	init();

	m_p2pServer=(YangP2pServer*)calloc(sizeof(YangP2pServer),1);
	yang_create_p2pserver(m_p2pServer,m_context->avinfo.sys.httpPort);
	m_p2pServer->receive=g_p2p_receive;
	m_p2pServer->user=this;
	yang_start_p2pserver(m_p2pServer);

	m_outVideoBuffer = NULL;
	m_outAudioBuffer = NULL;

}

YangP2pHandleImpl::~YangP2pHandleImpl() {
	if (m_pub)
		m_pub->stop();
	if (m_cap)
		m_cap->stopAll();
	 if(m_decoder) 		m_decoder->stopAll();
	yang_delete(m_pub);
	yang_delete(m_cap);
	yang_delete(m_decoder);
	yang_destroy_p2pserver(m_p2pServer);
	yang_free(m_p2pServer);

	yang_delete(m_outVideoBuffer);
	yang_delete(m_outAudioBuffer);
}
void YangP2pHandleImpl::disconnect() {
	if (m_cap) {
		if(m_hasAudio) m_cap->stopAudioCaptureState();
		m_cap->stopVideoCaptureState();

	}
	stopPublish();
}
void YangP2pHandleImpl::init() {
	if(m_isInit) return;
	m_videoState = Yang_VideoSrc_Camera;
	switchToCamera(true);

	m_isInit=true;
}
void YangP2pHandleImpl::startCapture() {

}
void YangP2pHandleImpl::removePlayBuffer(int32_t puid,int32_t playcount){
	if(playcount<1){
		if(m_decoder) m_decoder->stopAll();
		yang_delete(m_decoder);
		return;
	}
	if(m_decoder&&m_decoder->m_videoDec){
		m_decoder->m_videoDec->addVideoStream(NULL, puid, 0);
	}
	if(m_decoder&&m_decoder->m_audioDec){
			m_decoder->m_audioDec->removeAudioStream(puid);
	}
	int32_t ind=-1;
	if(m_context) ind=m_context->streams.getIndex(puid);
	if(ind>-1){
		yang_delete(m_context->streams.m_playBuffers->at(ind));
		m_context->streams.m_playBuffers->erase(m_context->streams.m_playBuffers->begin()+ind);
	}

}

void YangP2pHandleImpl::switchToCamera(bool pisinit) {
	m_videoState = Yang_VideoSrc_Camera;
	if(m_cap) m_cap->setCaptureType(m_videoState);
	if(m_cap) m_cap->setVideoInfo(&m_context->avinfo.video);


	startCamera();
}


void YangP2pHandleImpl::stopPublish() {
	if (m_pub) {
		m_pub->disConnectMediaServer();
	}
	yang_stop(m_pub);
	if(m_decoder) m_decoder->stopAll();
	yang_stop_thread(m_pub);
	yang_delete(m_pub);
	if(m_cap) m_cap->deleteVideoEncoding();
	yang_delete(m_decoder);
}
YangVideoBuffer* YangP2pHandleImpl::getPreVideoBuffer() {
	if (m_cap)	return m_cap->getPreVideoBuffer();
	return NULL;
}
vector<YangVideoBuffer*>* YangP2pHandleImpl::getPlayVideoBuffer(){
	if(m_decoder) return m_decoder->getOutVideoBuffer();
	return NULL;
}
int32_t YangP2pHandleImpl::initRtc(){
	if (m_pub == NULL) {
		m_pub = new YangP2pRtc(m_context);
		m_pub->m_playremove=this;
	}
	if(m_hasAudio) {
        m_hasAudio=bool(m_cap->startAudioCapture()==Yang_Ok);
	}
	if (m_hasAudio) {

		m_cap->initAudioEncoding();
	}

	m_cap->initVideoEncoding();
	m_cap->setNetBuffer(m_pub);

	if (m_hasAudio) 	m_cap->startAudioEncoding();
	m_cap->startVideoEncoding();
	return Yang_Ok;
}

int32_t YangP2pHandleImpl::initRtc2(bool hasPlay){
	if(!m_isInitRtc){
		m_pub->start();
		if (m_hasAudio)
			m_cap->startAudioCaptureState();

		m_cap->startVideoCaptureState();
		m_isInitRtc=true;
	}
	if(hasPlay){
		if(m_decoder==NULL) 		{
			initPlayList();
			m_decoder=new YangP2pDecoder(m_context);

			m_decoder->initAudioDecoder();
			m_decoder->initVideoDecoder();
			m_decoder->setInAudioBuffer(m_outAudioBuffer);
			m_decoder->setInVideoBuffer(m_outVideoBuffer);
			m_pub->setOutAudioList(m_outAudioBuffer);
			m_pub->setOutVideoList(m_outVideoBuffer);

		}
		if(m_decoder){
			m_decoder->startAudioDecoder();
			m_decoder->startVideoDecoder();
		}
	}
	return Yang_Ok;
}

int32_t YangP2pHandleImpl::connectRtc(char* url) {
	int32_t localPort=m_context->avinfo.sys.rtcLocalPort++;
	int err = Yang_Ok;
	memset(&m_url,0,sizeof(m_url));
	if (yang_srs_url_parse(url, &m_url))	return 1;
	stopPublish();
	m_isInitRtc=false;
	yang_trace("\nnetType==%d,server=%s,port=%d,app=%s,stream=%s\n",
			m_url.netType, m_url.server, m_url.port, m_url.app,
			m_url.stream);
	initRtc();
	if ((err = m_pub->init(m_url.netType, m_url.server,  localPort,
			m_url.port, m_url.app, m_url.stream)) != Yang_Ok) {
		return yang_error_wrap(err, " connect server failure!");
	}

	initRtc2(true);
	return err;

}
int YangP2pHandleImpl::startRtc(char* remoteIp,char* sdp,char* response ) {
	int32_t localPort=m_context->avinfo.sys.rtcLocalPort++;
	int err = Yang_Ok;

	if(!m_isInitRtc) initRtc();
	int hasplay=0;
	if ((err = m_pub->addPeer(sdp, response,remoteIp,localPort,&hasplay)) != Yang_Ok) {
		return yang_error_wrap(err, " connect server failure!");
	}
	initRtc2(hasplay?true:false);
	return err;

}

void YangP2pHandleImpl::startCamera() {
	if(m_cap) m_cap->startCamera();
}


void YangP2pHandleImpl::stopCamera() {
	if(m_cap) m_cap->stopCamera();
}

void YangP2pHandleImpl::initPlayList() {
	if (m_outAudioBuffer == NULL) {
		m_outAudioBuffer = new YangAudioEncoderBuffer(10);
	}
	if (m_outVideoBuffer == NULL)
		m_outVideoBuffer = new YangVideoDecoderBuffer();

}
