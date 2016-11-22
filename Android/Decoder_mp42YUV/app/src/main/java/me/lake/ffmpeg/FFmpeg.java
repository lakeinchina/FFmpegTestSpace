package me.lake.ffmpeg;

import android.view.Surface;

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

    public static native NativeMediaData open(String filePath);

    public static native long decodeNextFrame(long pointer,Surface surface);

    public static native long seekTo(long pointer,long timeSec);

    public static native long close(long pointer);
}
