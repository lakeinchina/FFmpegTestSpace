/*
create by lakeinchina
FFmpeg Documentation
https://www.ffmpeg.org/doxygen/3.2/index.html
*/
#include <stdio.h>
#include <time.h>
#include <jni.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
#include "log.h"
#include <android/native_window_jni.h>

typedef struct {
	char* inputPath;
	AVFormatContext* avFormatCtx;
    AVCodec *avCodec;
    AVCodecContext *avCodecCtx;
    AVFrame *avFrame;
    int videoTrackIndex;
    int width;
    int height;
    int64_t duration;
    float frame_rate;
    uint8_t *video_dst_data[4];
	int video_dst_linesize[4];
	int video_dst_bufsize;
	enum AVPixelFormat pix_fmt;
} Decoder;

void  renderingSurface(JNIEnv * env,jobject javaSurface,uint8_t* pixelsData[],jint w,jint h);

enum AVPixelFormat get_format(struct AVCodecContext *s, const enum AVPixelFormat * fmt)
{
	const enum AVPixelFormat *head = fmt;
	while(*fmt != AV_PIX_FMT_NONE)
	{
		//AV_PIX_FMT_YUV420P
		LOGD("type2=%s\n",av_get_pix_fmt_name(*fmt));
		fmt++;
	}
	return head[0];
}

jobject newNativeMediaDataByDecoder(JNIEnv *env,Decoder* decoder)
{
	jclass cls = (*env)->FindClass(env,"me/lake/ffmpeg/NativeMediaData");
    cls = (*env)->NewGlobalRef(env, cls);
    jmethodID constructor = (*env)->GetMethodID(env,cls,"<init>", "()V");
    jobject obj = (*env)->NewObject(env,cls, constructor);
    jmethodID setNativePointerID = (*env)->GetMethodID(env,cls, "setNativePointer", "(J)V");
    (*env)->CallVoidMethod(env,obj, setNativePointerID, (long long)decoder);
    jmethodID setWidthID = (*env)->GetMethodID(env,cls, "setWidth", "(I)V");
    (*env)->CallVoidMethod(env,obj, setWidthID, decoder->width);
    jmethodID setHeightID = (*env)->GetMethodID(env,cls, "setHeight", "(I)V");
    (*env)->CallVoidMethod(env,obj, setHeightID, decoder->height);
    jmethodID setDurationID = (*env)->GetMethodID(env,cls, "setDuration", "(J)V");
    (*env)->CallVoidMethod(env,obj, setDurationID, (long long)decoder->duration);
    jmethodID setFrameRateID = (*env)->GetMethodID(env,cls, "setFrameRate", "(F)V");
    (*env)->CallVoidMethod(env,obj, setFrameRateID, decoder->frame_rate);
    (*env)->DeleteGlobalRef(env, cls);cls = NULL;
    return obj;
}


JNIEXPORT jobject JNICALL Java_me_lake_ffmpeg_FFmpeg_open
(JNIEnv *env, jobject obj,jstring filePath) {
	AVFormatContext* avFormatCtx;
	AVCodec* avCodec;
	AVCodecContext* avCodecCtx;
	int videoTrackIndex=-1;

	const char *inputPathJava= (*env)->GetStringUTFChars(env,filePath, 0);
	char* inputPath = (char*)malloc(strlen(inputPathJava));
	strcpy(inputPath,inputPathJava);
 	(*env)->ReleaseStringUTFChars(env,filePath, inputPathJava);

	av_register_all();
	avFormatCtx = avformat_alloc_context();
	if(avformat_open_input(&avFormatCtx,inputPath,NULL,NULL)<0)
	{
		LOGD("ERROR!avformat_open_input()<0\n");
		return NULL;
	}
	if(avformat_find_stream_info(avFormatCtx,NULL)<0)
	{
		LOGD("ERROR!avformat_find_stream_info()<0\n");
		return NULL;
	}
	int ret = av_find_best_stream(avFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	if(ret<0)
	{
		LOGD("ERROR!av_find_best_stream()<0\n");
		return NULL;
	}
	videoTrackIndex = ret;
	avCodec = avcodec_find_decoder(avFormatCtx->streams[videoTrackIndex]->codecpar->codec_id);
	if(!avCodec)
	{
		LOGD("ERROR!avcodec_find_decoder()==NULL\n");
		return 0;
	}
	avCodecCtx = avcodec_alloc_context3(avCodec);
	avCodecCtx->get_format = get_format;
	if(!avCodecCtx)
	{
		LOGD("ERROR!avcodec_alloc_context3()==NULL\n");
		return NULL;
	}
	if(avcodec_parameters_to_context(avCodecCtx,avFormatCtx->streams[videoTrackIndex]->codecpar)<0)
	{
		LOGD("ERROR!avcodec_parameters_to_context()<0\n");
		return NULL;
	}
	if(avcodec_open2(avCodecCtx,avCodec,NULL)<0)
	{
		LOGD("ERROR!avcodec_open2()<0\n");
		return NULL;
	}
	//==================
	float fps =(float)avFormatCtx->streams[videoTrackIndex]->avg_frame_rate.num/avFormatCtx->streams[videoTrackIndex]->avg_frame_rate.den;
	int videoWidth,videoHeight;
	AVFrame *avFrame=NULL;
	avFrame = av_frame_alloc();
	if(!avFrame)
	{
		LOGD("ERROR!av_frame_alloc()==NULL\n");
		return NULL;
	}

	videoWidth = avCodecCtx->width;
	videoHeight = avCodecCtx->height;
	Decoder *decoder = (Decoder*)malloc(sizeof(Decoder));
	if(decoder->video_dst_bufsize=av_image_alloc(decoder->video_dst_data,decoder->video_dst_linesize,videoWidth,videoHeight,avCodecCtx->pix_fmt,1)<0)
	{
		LOGD("ERROR!av_image_alloc()==NULL\n");
		return NULL;
	}
	decoder->inputPath = inputPath;
	decoder->pix_fmt = avCodecCtx->pix_fmt;
	decoder->avCodec = avCodec;
    decoder->avCodecCtx = avCodecCtx;
    decoder->avFrame = avFrame;
    decoder->avFormatCtx = avFormatCtx;
    decoder->videoTrackIndex = videoTrackIndex;
    decoder->width = videoWidth;
    decoder->height = videoHeight;
    decoder->frame_rate = fps;
    decoder->duration = avFormatCtx->duration;
	return newNativeMediaDataByDecoder(env,decoder);
}

JNIEXPORT jlong JNICALL Java_me_lake_ffmpeg_FFmpeg_decodeNextFrame
(JNIEnv *env, jobject obj,jlong pointer,jobject surface) {
	Decoder *decoder = (Decoder*)pointer;
	AVFormatContext* avFormatCtx = decoder->avFormatCtx;
	AVCodec* avCodec = decoder->avCodec;
	AVCodecContext* avCodecCtx = decoder->avCodecCtx;
	int videoTrackIndex = decoder->videoTrackIndex;
	AVFrame *avFrame=decoder->avFrame;
	
	AVPacket avPacket;
	av_init_packet(&avPacket);
	avPacket.data=NULL;
	avPacket.size=0;

	int got_frame=0;
	int ret;
	while(av_read_frame(avFormatCtx,&avPacket)>=0)
	{
		if(avPacket.stream_index!=videoTrackIndex)
		{
			continue;
		}else{
			if(avcodec_decode_video2(avCodecCtx,avFrame,&got_frame,&avPacket)<0)
			{
				LOGD("ERROR!avcodec_decode_video2()<0\n");
				ret = 1;
			}else{
				LOGD("w=%d,h=%d,fmt=%d,name=%s",avFrame->width,avFrame->height,avFrame->format,av_get_pix_fmt_name(avFrame->format));
				ret =0;
			}
			if(got_frame)
			{
				if(avFrame->format==AV_PIX_FMT_YUV420P)
				{
					renderingSurface(env,surface,(const uint8_t **)(avFrame->data),decoder->width,decoder->height);
				}
				//av_image_copy(video_dst_data,video_dst_linesize,(const uint8_t **)(avFrame->data),avFrame->linesize,pix_fmt,videoWidth,videoHeight);
				LOGD("INFO!got_frame\n");
				break;
			}
		}
	}
	return ret;
}

JNIEXPORT jlong JNICALL Java_me_lake_ffmpeg_FFmpeg_seekTo
(JNIEnv *env, jobject obj,jlong pointer) {
	return 0;
}

JNIEXPORT jlong JNICALL Java_me_lake_ffmpeg_FFmpeg_close
(JNIEnv *env, jobject obj,jlong pointer) {
	Decoder *decoder = (Decoder*)pointer;
	avcodec_free_context(&(decoder->avCodecCtx));
	avformat_close_input(&(decoder->avFormatCtx));
	av_frame_free(&(decoder->avFrame));
	av_free(decoder->video_dst_data[0]);
	free(decoder->inputPath);
	free(decoder);
	return 0;
}

void  renderingSurface(JNIEnv * env,jobject javaSurface,uint8_t* pixelsData[],jint w,jint h) {
	ANativeWindow* window = ANativeWindow_fromSurface(env, javaSurface);
	if(window!=NULL)
	{
		ANativeWindow_setBuffersGeometry(window,w,h,842094169);
		ANativeWindow_Buffer buffer;
		if (ANativeWindow_lock(window, &buffer, NULL) == 0) {
			if(buffer.width==buffer.stride){
				memcpy(buffer.bits, pixelsData[0],  w*h);
				memcpy(buffer.bits+w*h, pixelsData[2],  w*h/4);
				memcpy(buffer.bits+w*h*5/4, pixelsData[1],  w*h/4);
			}else{
			}
			ANativeWindow_unlockAndPost(window);
		}
		ANativeWindow_release(window);
	}
	return;
}


