package me.lake.ffmpeg;

/**
 * Created by lake on 17/11/16.
 * Decoder_mp42YUV project.
 */
public class FFmpeg {
    static {
        System.loadLibrary("x264");
        System.loadLibrary("ffmpeg");
        System.loadLibrary("yuv2h264");
    }
    public static native long create();
}
