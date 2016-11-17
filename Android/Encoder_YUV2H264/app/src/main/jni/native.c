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

typedef struct {
    AVCodec *avCodec;
    AVCodecContext *avCodecCtx;
    AVFrame *avFrame;
    int ysize;
    int usize;
} H264Encoder;

void encoderCallback2Java(JNIEnv *env, jobject callbackobj, uint8_t *data, int size, long pts,
                          long dts);

JNIEXPORT jlong JNICALL Java_me_lake_ffmpeg_FFmpeg_00024H264_create
        (JNIEnv *env, jobject obj, jint width, jint height, jint bitrate) {
    AVCodec *avCodec = NULL;
    AVCodecContext *avCodecCtx = NULL;
    AVFrame *avFrame;
    int videoWidth = width;
    int videoHeight = height;
    int videoBitrate = bitrate;
    avcodec_register_all();
    avCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!avCodec) {
        LOGD("ERROR!avcodec_find_encoder()=NULL\n");
        return 0;
    }
    avCodecCtx = avcodec_alloc_context3(avCodec);
    if (!avCodecCtx) {
        LOGD("ERROR!avcodec_alloc_context3()=NULL\n");
        return 0;
    }
    avCodecCtx->bit_rate = videoBitrate;
    avCodecCtx->width = videoWidth;
    avCodecCtx->height = videoHeight;
    avCodecCtx->gop_size = 15;
    avCodecCtx->max_b_frames = 0;
    avCodecCtx->b_frame_strategy = 0;
    avCodecCtx->time_base.num = 1;
    avCodecCtx->time_base.den = 30;
    avCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    avCodecCtx->profile = FF_PROFILE_H264_MAIN;
    //avCodecCtx->level = 31;
    //ultrafast,superfast, veryfast, faster, fast, medium, slow, slower, veryslow,placebo
    av_opt_set(avCodecCtx->priv_data, "preset", "veryfast", 0);

    if (avcodec_open2(avCodecCtx, avCodec, NULL) < 0) {
        LOGD("ERROR!avcodec_open2()<0\n");
        return 0;
    }

    avFrame = av_frame_alloc();
    if (!avFrame) {
        LOGD("ERROR!av_frame_alloc()=NULL\n");
        return 0;
    }
    avFrame->format = avCodecCtx->pix_fmt;
    avFrame->width = avCodecCtx->width;
    avFrame->height = avCodecCtx->height;

    if (av_image_alloc(avFrame->data, avFrame->linesize, avFrame->width, avFrame->height,
                       avFrame->format, 16) < 0) {
        LOGD("ERROR!av_image_alloc()=NULL\n");
        return 0;
    }
    H264Encoder *encoder = (H264Encoder *) malloc(sizeof(H264Encoder));
    encoder->avCodec = avCodec;
    encoder->avCodecCtx = avCodecCtx;
    encoder->avFrame = avFrame;
    encoder->ysize = videoWidth * videoHeight;
    encoder->usize = encoder->ysize / 4;
    return (jlong) encoder;
}

JNIEXPORT jint JNICALL Java_me_lake_ffmpeg_FFmpeg_00024H264_encode
        (JNIEnv *env, jobject obj, jlong pointer, jbyteArray pixdataArray, jlong pts,
         jobject callbackobj) {
    AVPacket avPacket;
    H264Encoder *encoder = (H264Encoder *) pointer;
    unsigned char *pixdata = (unsigned char *) (*env)->GetByteArrayElements(env, pixdataArray, 0);

    av_init_packet(&avPacket);
    avPacket.data = NULL;
    avPacket.size = 0;

    AVFrame *avFrame = encoder->avFrame;
    memcpy(avFrame->data[0], pixdata, encoder->ysize);
    memcpy(avFrame->data[1], pixdata + encoder->ysize, encoder->usize);
    memcpy(avFrame->data[2], pixdata + encoder->ysize + encoder->usize, encoder->usize);
    (*env)->ReleaseByteArrayElements(env, pixdataArray, (jbyte *) pixdata, JNI_ABORT);

    avFrame->pts = pts;
    int got_packet;
    int ret = avcodec_encode_video2(encoder->avCodecCtx, &avPacket, avFrame, &got_packet);
    if (ret < 0) {
        LOGD("ERROR!avcodec_encode_video2()=ret\n");
        return -1;
    }
    if (got_packet) {
        LOGD("INFO!avcodec_encode_video2(),size=%d,pts=%ld\n", avPacket.size, avPacket.pts);
        encoderCallback2Java(env, callbackobj, avPacket.data, avPacket.size, avPacket.pts,
                             avPacket.dts);
    }
    av_free_packet(&avPacket);
    return 0;
}

JNIEXPORT jint JNICALL Java_me_lake_ffmpeg_FFmpeg_00024H264_flush
        (JNIEnv *env, jobject obj, jlong pointer, jobject callbackobj) {
    H264Encoder *encoder = (H264Encoder *) pointer;
    AVPacket avPacket;
    av_init_packet(&avPacket);
    avPacket.data = NULL;
    avPacket.size = 0;
    int got_packet;
    do {
        int ret = avcodec_encode_video2(encoder->avCodecCtx, &avPacket, NULL, &got_packet);
        if (ret < 0) {
            LOGD("ERROR!avcodec_encode_video2()=ret\n");
            return -1;
        }
        if (got_packet) {
            LOGD("INFO!avcodec_encode_video2(),packetsize=%ld\n", avPacket.size);
            encoderCallback2Java(env, callbackobj, avPacket.data, avPacket.size, avPacket.pts,
                                 avPacket.dts);
        }
        av_free_packet(&avPacket);
    } while (got_packet);
    return 0;
}

JNIEXPORT jint JNICALL Java_me_lake_ffmpeg_FFmpeg_00024H264_destroy
        (JNIEnv *env, jobject obj, jlong pointer) {
    H264Encoder *encoder = (H264Encoder *) pointer;
    avcodec_close(encoder->avCodecCtx);
    av_free(encoder->avCodecCtx);
    av_freep(&(encoder->avFrame)->data[0]);
    av_frame_free(&(encoder->avFrame));
    free(encoder);
    LOGD("INFO!ffmepg,freed\n");
    return 0;
}

JNIEXPORT void JNICALL Java_me_lake_ffmpeg_FFmpeg_00024H264_NV21TOYUV420P
        (JNIEnv *env, jobject thiz, jbyteArray srcarray, jbyteArray dstarray, jint ySize) {
    unsigned char *src = (unsigned char *) (*env)->GetByteArrayElements(env, srcarray, 0);
    unsigned char *dst = (unsigned char *) (*env)->GetByteArrayElements(env, dstarray, 0);
    memcpy(dst, src, ySize);
    int uSize = ySize >> 2;
    unsigned char *srcucur = src + ySize + 1;
    unsigned char *srcvcur = src + ySize;
    unsigned char *dstucur = dst + ySize;
    unsigned char *dstvcur = dst + ySize + uSize;
    int i = 0;
    while (i < uSize) {
        (*dstucur) = (*srcucur);
        (*dstvcur) = (*srcvcur);
        srcucur += 2;
        srcvcur += 2;
        ++dstucur;
        ++dstvcur;
        ++i;
    }
    (*env)->ReleaseByteArrayElements(env, srcarray, src, JNI_ABORT);
    (*env)->ReleaseByteArrayElements(env, dstarray, dst, JNI_ABORT);
    return;
}

void encoderCallback2Java(JNIEnv *env, jobject callbackobj, uint8_t *data, int size, long pts,
                          long dts) {
    jclass clazzCallback = (*env)->FindClass(env, "me/lake/ffmpeg/FFmpeg$EncodeCallback");
    jmethodID onEncodedDataCallback = (*env)->GetMethodID(env, clazzCallback, "onEncodedData",
                                                          "([BJJ)V");
    jbyteArray jarray = (*env)->NewByteArray(env, size);
    (*env)->SetByteArrayRegion(env, jarray, 0, size, (jbyte *) data);
    (*env)->CallVoidMethod(env, callbackobj, onEncodedDataCallback, jarray, (long long) pts,
                           (long long) dts);
}






