/*
create by lakeinchina
FFmpeg Documentation
https://www.ffmpeg.org/doxygen/3.2/index.html
*/
#include <stdio.h>
#include <time.h>
#include <jni.h>
#include "libavcodec/avcodec.h"
#include "libavutil/opt.h"
#include "log.h"


JNIEXPORT jlong JNICALL Java_me_lake_ffmpeg_FFmpeg_create
(JNIEnv *env, jobject obj) {
	AVFormatContex* avFormatCtx;
	AVCodec* avCodec;
	AVCodecContext* avCodecCtx;
	int videoTrackIndex=-1;

	char* inputPath = "/sdcard/ffmpeg.mp4";
	av_register_all();
	avFormatCtx = avformat_alloc_context();
	if(avformat_open_input(&avFormatCtx,inputPath,NULL,NULL)<0)
	{
		LOGD("ERROR!avformat_open_input()<0\n");
		return 0;
	}
	if(avformat_find_stream_info(avFormatCtx,NULL)<0)
	{
		LOGD("ERROR!avformat_find_stream_info()<0\n");
		return 0;
	}
	int ret = av_find_best_stream(avFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	if(ret<0)
	{
		LOGD("ERROR!av_find_best_stream()<0\n");
		return 0;
	}
	videoTrackIndex = ret;
	avCodecCtx = ;
	avCodec = avcodec_find_decoder(avFormatCtx->streams[videoTrackIndex]->codecpar->codec_id);
	if(!avCodec)
	{
		LOGD("ERROR!avcodec_find_decoder()==NULL\n");
		return 0;
	}
	avCodecCtx = avcodec_alloc_context3(avCodec);
	if(!avCodecCtx)
	{
		LOGD("ERROR!avcodec_alloc_context3()==NULL\n");
		return 0;
	}
	if(avcodec_parameters_to_context(avCodecCtx,avFormatCtx->streams[videoTrackIndex]->codecpar)<0)
	{
		LOGD("ERROR!avcodec_parameters_to_context()<0\n");
		return 0;
	}
	if(avcodec_open2(avCodecCtx,avCodec,NULL)<0)
	{
		LOGD("ERROR!avcodec_open2()<0\n");
		return 0;
	}
	//==================
	uint8_t *video_dst_data[4] = {NULL};
	int video_dst_linesize[4];
	int videoWidth,videoHeight;
	int video_dst_bufsize;
	enum AVPixelFormat pix_fmt;
	AVFrame *avFrame=NULL;
	AVPacket avPacket;
	avFrame = av_frame_alloc();
	if(!avFrame)
	{
		LOGD("ERROR!av_frame_alloc()==NULL\n");
		return 0;
	}
	av_init_packet(&avPacket);
	avPacket.data=NULL;
	avPacket.size=0;

	videoWidth = avCodecCtx->width;
	videoHeight = avCodecCtx->height;
	pix_fmt = avCodecCtx->pix_fmt;
	if(video_dst_bufsize=av_image_alloc(video_dst_data,video_dst_linesize,videoWidth,videoHeight,pix_fmt,1)<0)
	{
		LOGD("ERROR!av_image_alloc()==NULL\n");
		return 0;
	}
	int got_picture;
	while(av_read_frame(avFormatCtx,&avPacket))
	{
		if(avPacket->stream_index==videoTrackIndex)
		{
			continue;
		}
		if(avcodec_decode_video2(avCodecCtx,avFrame,&got_picture,avPacket)<0)
		{
			LOGD("ERROR!avcodec_decode_video2()<0\n");
		}
		if(got_picture)
		{
			av_image_copy(video_dst_data,video_dst_linesize,(const uint8_t **)(avFrame->data),avFrame->linesize,pix_fmt,videoWidth,videoHeight);
			LOGD("INFO!got_picture\n");
		}
	}
	//flush
	pkt.data=NULL;
	pkt.size=0;
	do{
		avcodec_decode_video2(avCodecCtx,avFrame,&got_picture,avPacket);
		if(got_picture)
		{
			av_image_copy(video_dst_data,video_dst_linesize,(const uint8_t **)(avFrame->data),avFrame->linesize,pix_fmt,videoWidth,videoHeight);
			LOGD("INFO!flush,got_picture\n");
		}
	}while(got_picture);


	avcodec_free_context(&avCodecCtx);
	avformat_close_input(&avFormatCtx);
	av_frame_free(&avFrame);
	av_free(video_dst_data[0]);
	return 0;
}






