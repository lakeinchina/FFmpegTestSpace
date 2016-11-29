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
#include <libavutil/timestamp.h>
#include "libswresample/swresample.h"
#include "libavutil/opt.h"
#include "log.h"

typedef struct {
	char* inputPath;
	AVFormatContext* avFormatCtx;
    AVCodec *avCodec;
    AVCodecContext *avCodecCtx;
    AVFrame *avFrame;
    int audioTrackIndex;
    int64_t duration;
    float frame_rate;
	enum AVPixelFormat pix_fmt;
	struct SwrContext *swrCtx;
	uint8_t *out_buffer;
} Decoder;
#define MAX_AUDIO_FRAME_SIZE 192000

void decoderCallback2Java(JNIEnv *env, jobject callbackobj, uint8_t *data, int size, long pts);

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
	int audioTrackIndex =-1;

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
	int ret = av_find_best_stream(avFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	if(ret<0)
	{
		LOGD("ERROR!av_find_best_stream()<0\n");
		return NULL;
	}
	audioTrackIndex = ret;
	avCodec = avcodec_find_decoder(avFormatCtx->streams[audioTrackIndex]->codecpar->codec_id);
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
	if(avcodec_parameters_to_context(avCodecCtx,avFormatCtx->streams[audioTrackIndex]->codecpar)<0)
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
	float fps =(float)avFormatCtx->streams[audioTrackIndex]->avg_frame_rate.num/avFormatCtx->streams[audioTrackIndex]->avg_frame_rate.den;
	AVFrame *avFrame=NULL;
	avFrame = av_frame_alloc();
	if(!avFrame)
	{
		LOGD("ERROR!av_frame_alloc()==NULL\n");
		return NULL;
	}
	struct SwrContext *swrCtx;
	swrCtx = swr_alloc();
	if(!swrCtx)
	{
		LOGD("ERROR!swr_alloc()==NULL\n");
		return 0;
	}
	int in_channel_layout =av_get_default_channel_layout(avCodecCtx->channels);
	av_opt_set_int(swrCtx,"in_channel_layout",in_channel_layout,0);
	av_opt_set_int(swrCtx,"in_sample_rate",avCodecCtx->sample_rate,0);
	av_opt_set_sample_fmt(swrCtx,"in_sample_fmt",avCodecCtx->sample_fmt,0);
	LOGD("avCodecCtx->sample_rate=%d,%d,%d",avCodecCtx->sample_rate,avCodecCtx->sample_fmt,in_channel_layout);

	int out_channel_layout = AV_CH_LAYOUT_MONO;
	int out_nb_samples=avCodecCtx->frame_size;
	enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
	int out_sample_rate=44100;
	int out_channels = av_get_channel_layout_nb_channels(out_channel_layout);
	int out_buffer_size = av_samples_get_buffer_size(NULL,out_channels,out_nb_samples,out_sample_fmt,1);
	uint8_t *out_buffer=(uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE*2);

	av_opt_set_int(swrCtx,"out_channel_layout",out_channel_layout,0);
	av_opt_set_int(swrCtx,"out_sample_rate",out_sample_rate,0);
	av_opt_set_sample_fmt(swrCtx,"out_sample_fmt",out_sample_fmt,0);

	if((ret=swr_init(swrCtx))<0)
	{
		LOGD("ERROR!swr_init()<0\n");
		return 0;
	}

	Decoder *decoder = (Decoder*)malloc(sizeof(Decoder));
	decoder->inputPath = inputPath;
	decoder->pix_fmt = avCodecCtx->pix_fmt;
	decoder->avCodec = avCodec;
    decoder->avCodecCtx = avCodecCtx;
    decoder->avFrame = avFrame;
    decoder->avFormatCtx = avFormatCtx;
    decoder->audioTrackIndex = audioTrackIndex;
    decoder->frame_rate = fps;
    decoder->duration = avFormatCtx->duration;
	decoder->swrCtx = swrCtx;
	decoder->out_buffer = out_buffer;
	return newNativeMediaDataByDecoder(env,decoder);
}

JNIEXPORT jlong JNICALL Java_me_lake_ffmpeg_FFmpeg_decodeNextFrame
(JNIEnv *env, jobject obj,jlong pointer,jobject callbackobj) {

	Decoder *decoder = (Decoder*)pointer;
	AVFormatContext* avFormatCtx = decoder->avFormatCtx;
	AVCodec* avCodec = decoder->avCodec;
	AVCodecContext* avCodecCtx = decoder->avCodecCtx;
	int audioTrackIndex = decoder->audioTrackIndex;
	AVFrame *avFrame=decoder->avFrame;
	struct SwrContext *swrCtx = decoder->swrCtx;
	uint8_t *out_buffer= decoder->out_buffer;

	
	AVPacket avPacket;
	av_init_packet(&avPacket);
	avPacket.data=NULL;
	avPacket.size=0;

	int got_frame=0;
	long long ret = -1;

	while(av_read_frame(avFormatCtx,&avPacket)>=0)
	{
		if(avPacket.stream_index!= audioTrackIndex)
		{
			continue;
		}else{
			if(avcodec_decode_audio4(avCodecCtx,avFrame,&got_frame,&avPacket)<0)
			{
				LOGD("ERROR!avcodec_decode_audio4()<0\n");
				ret = (long long)-1;
			}
			if(got_frame)
			{
				int64_t timeBase=((int64_t)(decoder->avFormatCtx->streams[decoder->audioTrackIndex]->time_base.den)) / (int64_t)(decoder->avFormatCtx->streams[decoder->audioTrackIndex]->time_base.num);
				LOGD("fmt=%d,name=%s",avFrame->format,av_get_sample_fmt_name(avFrame->format));
				ret = avFrame->pkt_pts/timeBase;
				LOGD("buf=%x,%x,%x,%x,%x,%x\n",avFrame->data[0][0],avFrame->data[0][1],avFrame->data[0][2],avFrame->data[0][3],avFrame->data[0][4],avFrame->data[0][5]);
				int samplesnb = swr_convert(swrCtx,&out_buffer,MAX_AUDIO_FRAME_SIZE,(const uint8_t**)avFrame->data,avFrame->nb_samples);
				decoderCallback2Java(env,callbackobj,out_buffer,samplesnb*2,ret);
				 size_t unpadded_linesize = avFrame->nb_samples * av_get_bytes_per_sample(avFrame->format);
				LOGD("audio_frame n: nb_samples:%d pts:%s\n", avFrame->nb_samples,
					   av_ts2timestr(avFrame->pts, &avCodecCtx->time_base));
				LOGD("unpadded_linesize=%d",unpadded_linesize);
				LOGD("INFO!got_frame,%ld,%ld,%ld\n",avFrame->pts,avFrame->pkt_pts,avFrame->pkt_dts);
				break;
			}
		}
		av_free_packet(&avPacket);
	}
	return (long long)ret;
}

JNIEXPORT jlong JNICALL Java_me_lake_ffmpeg_FFmpeg_seekTo
(JNIEnv *env, jobject obj,jlong pointer,long timeSec) {
	Decoder *decoder = (Decoder*)pointer;
	//AVSEEK_FLAG_BACKWARD  seek backward
	//AVSEEK_FLAG_BYTE      seeking based on position in bytes
	//AVSEEK_FLAG_ANY       seek to any frame, even non-keyframes
	//AVSEEK_FLAG_FRAME		seeking based on frame number
	int64_t timeBase=((int64_t)(decoder->avFormatCtx->streams[decoder->audioTrackIndex]->time_base.den)) / (int64_t)(decoder->avFormatCtx->streams[decoder->audioTrackIndex]->time_base.num);
	int ret = av_seek_frame(decoder->avFormatCtx,decoder->audioTrackIndex,timeSec*timeBase,AVSEEK_FLAG_BACKWARD);
	//LOGD("timeBase=%ld,%lld,%lld,%ld\n",timeSec,timeBase,timeSec*timeBase,AV_TIME_BASE);
	return 0;
}
JNIEXPORT jlong JNICALL Java_me_lake_ffmpeg_FFmpeg_close
(JNIEnv *env, jobject obj,jlong pointer) {
	Decoder *decoder = (Decoder*)pointer;
	avcodec_free_context(&(decoder->avCodecCtx));
	avformat_close_input(&(decoder->avFormatCtx));
	av_frame_free(&(decoder->avFrame));
	swr_free(&decoder->swrCtx);
	av_free(decoder->out_buffer);
	free(decoder->inputPath);
	free(decoder);
	return 0;
}

void decoderCallback2Java(JNIEnv *env, jobject callbackobj, uint8_t *data, int size, long pts) {
	jclass clazzCallback = (*env)->FindClass(env, "me/lake/ffmpeg/FFmpeg$DecodeCallback");
	jmethodID onDecodedDataCallback = (*env)->GetMethodID(env, clazzCallback, "onDecodedData",
														  "([BJ)V");
	jbyteArray jarray = (*env)->NewByteArray(env, size);
	(*env)->SetByteArrayRegion(env, jarray, 0, size, (jbyte *) data);
	(*env)->CallVoidMethod(env, callbackobj, onDecodedDataCallback, jarray, (long long) pts);
}








